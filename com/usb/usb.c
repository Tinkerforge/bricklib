/* bricklib
 * Copyright (C) 2009-2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb.c: Communication interface implementation for USB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "usb.h"

#include <string.h>

#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/pio/pio_it.h"
#include "bricklib/drivers/pmc/pmc.h"
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include "bricklib/free_rtos/include/semphr.h"
#include "bricklib/drivers/usb/USBDescriptors.h"
#include "bricklib/drivers/usb/USBDDriver.h"
#include "bricklib/drivers/usb/USBD.h"
#include "bricklib/drivers/usb/USBD_HAL.h"

#include "bricklib/drivers/adc/adc.h"

#include "bricklib/com/com_messages.h"
#include "bricklib/com/com_common.h"
#include "bricklib/com/com.h"
#include "bricklib/utility/init.h"
#include "bricklib/com/com_common.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/logging/logging.h"
#include "bricklib/com/spi/spi_stack/spi_stack_master.h"
#include "bricklib/utility/led.h"
#include "bricklib/bricklet/bricklet_config.h"

#include "usb_descriptors.h"
#include "config.h"

#define MAX_USB_MESSAGE_SIZE 80

#define USB_IN_FUNCTION 1
#define USB_CALLBACK 2

extern const USBDDriverDescriptors driver_descriptors;
extern ComInfo com_info;

static USBDDriver usbd_driver;
bool usb_first_connection = false;
uint8_t usb_wakeup_counter = 0;
uint32_t usb_recv_transferred = 0;

bool usb_startup_connected = false;
static uint8_t receive_status = 0;
static uint8_t send_status = 0;

char usb_recv_buffer[DEFAULT_EP_SIZE];
char usb_send_buffer[DEFAULT_EP_SIZE];
uint16_t usb_send_buffer_length = 0;

#ifdef PIN_USB_DETECT
static Pin pin_usb_detect = PIN_USB_DETECT;
#endif

uint32_t usb_sequence_number = 0;

typedef enum {
	USB_SEND_STATE_IDLE,
	USB_SEND_STATE_WAIT_FOR_CALLBACK,
} USBSendState;

volatile USBSendState usb_send_state = USB_SEND_STATE_IDLE;

void usb_send_callback(void *arg,
                       uint8_t status,
                       uint32_t transferred,
                       uint32_t remaining) {
	static int32_t higher_priority_task_woken;

	usb_send_state = USB_SEND_STATE_IDLE;

	// If data was not transferred we go back to idle, but re-send the data
	if(transferred != 0) {
		usb_send_buffer_length = 0;
	}

	portEND_SWITCHING_ISR(higher_priority_task_woken);
}

bool is_endpoint_idle(const uint8_t ep);
void usb_handle_send(void) {
	if(usb_send_buffer_length == 0) {
		return;
	}

	switch(usb_send_state) {
		case USB_SEND_STATE_IDLE: {
			usb_send_state = USB_SEND_STATE_WAIT_FOR_CALLBACK;
			if(!(USBD_Write(IN_EP, usb_send_buffer, usb_send_buffer_length, usb_send_callback, (void*)usb_sequence_number) == USBD_STATUS_SUCCESS)) {
				usb_send_state = USB_SEND_STATE_IDLE;
			} else {

			}

			break;
		}

		case USB_SEND_STATE_WAIT_FOR_CALLBACK: {
			// If we reach this state, we did a write and there was no callback within one ms.
			// Normally this does not happen.
			// We disable the irq and check if the endpoint is idling. If it is we have to assume
			// that the message was lost and we have to send it again.
			// This can for example happen if a PC goes into suspend during a usb write (before
			// the message reached the PC).
			__disable_irq();
			if(usb_send_state == USB_SEND_STATE_WAIT_FOR_CALLBACK) { // Check again if we got the callback between the switch and the irq disable...
				if(is_endpoint_idle(IN_EP)) {
					USBD_Write(IN_EP, usb_send_buffer, usb_send_buffer_length, usb_send_callback, (void*)usb_sequence_number);
				}
			}
			__enable_irq();

			break;
		}
	}
}

inline uint16_t usb_send(const void *data, const uint16_t length, uint32_t *options) {
	if((usb_send_buffer_length != 0) || length > DEFAULT_EP_SIZE) {
		return 0;
	}

	// Copy to send buffer and call usb_handle_send once.
	// Normally the send will be handled in the first try and a callback
	// will be issued after the send finished (and allow for a new send).

	// But in cases of usb errors or suspend/resume we may need to call the
	// usb write function again. Because of this we have to copy the buffer first.
	// Otherwise the data may be lost in case of an error.
	memcpy(usb_send_buffer, data, length);
	usb_send_buffer_length = length;
	usb_handle_send();

	return length;
}

// usb receive is implemented with a hook in the usb protocol state machine
// from atmel (see USBD_HAL.c). Unfortunately the USBD_Read is not reliable,
// it does not always invoke the callback function. Since i could not fix this,
// we write the usb_recv_buffer directly to the endpoint whenever the pc
// polls and there is data available. This is a little bit faster and seems
// to work very reliable.
inline uint16_t usb_recv(void *data, const uint16_t length, uint32_t *options) {
	if(usb_recv_transferred == 0) {
		usb_set_read_endpoint_state_to_receiving();
		return 0;
	}

	memcpy(data, usb_recv_buffer, usb_recv_transferred);
	usb_set_read_endpoint_state_to_receiving();
	uint16_t tmp = usb_recv_transferred;
	usb_recv_transferred = 0;

	return tmp;
}

bool usb_is_connected(void) {
#ifdef BRICK_CAN_BE_MASTER
#ifdef BRICK_IS_BRIDGE
	return true;
#else
    if(master_get_hardware_version() > 20) {
        // Master V2.1 detects usb with GND
        return adc_channel_get_data(USB_VOLTAGE_CHANNEL) < VOLTAGE_MAX_VALUE*1/3;
    } else {
        return adc_channel_get_data(USB_VOLTAGE_CHANNEL) > VOLTAGE_MAX_VALUE*2/3;
    }
#endif
#else
	return PIO_Get(&pin_usb_detect);
#endif
}

bool usb_add_enumerate_connected_request(void) {
	__disable_irq();
	if(usb_recv_transferred != 0) {
		__enable_irq();
		return false;
	}

	com_make_default_header(usb_recv_buffer, 0, sizeof(Enumerate), FID_CREATE_ENUMERATE_CONNECTED);
	usb_recv_transferred = sizeof(Enumerate);
	__enable_irq();

	usb_handle_send();

	return true;
}

void usb_detect_configure(void) {
#ifdef PIN_USB_DETECT
	 PIO_Configure(&pin_usb_detect, 1);
	 pin_usb_detect.attribute = PIO_PULLDOWN;
	 PIO_Configure(&pin_usb_detect, 1);
	 // We use usb_detect_task instead of interrupt
#endif
}


void usb_tick_task(const uint8_t tick_type) {
	if(tick_type == TICK_TASK_TYPE_CALCULATION) {
		if(usb_wakeup_counter > 0) {
			usb_wakeup_counter++;
			if(usb_wakeup_counter == 255) {
				logi("USB Remote Wakeup\n\r");
				USBD_RemoteWakeUp();
			}
		}
	} else if(tick_type == TICK_TASK_TYPE_MESSAGE) {
		usb_handle_send();

		// Periodically look if Bricklets need re-enumeration.
		// We run this here, since the usb tick task runs on every brick.
		brick_init_handle_bricklet_enumeration();

		// Handle initial USB enumeration
		if(usb_first_connection && !usbd_hal_is_disabled(IN_EP)) {
			if(usb_add_enumerate_connected_request()) {
				usb_first_connection = false;
				com_info.current = COM_USB;
			}
		}
	}
}

bool usb_init() {
    usb_startup_connected = true;

	send_status = 0;
	receive_status = 0;

	usb_configure_clock_48mhz();

    USBDDriver_Initialize(&usbd_driver, &driver_descriptors, 0);
    USBD_Init();

    USBD_Connect();

    // Check current level on VBus
    if(usb_is_connected()) {
    	return true;
    }

	return false;
}

void usb_message_loop_return(const char *data, const uint16_t length) {
	com_route_message_from_pc(data, length, COM_USB);
}

void usb_message_loop(void *parameters) {
	MessageLoopParameter mlp;
	mlp.buffer_size = MAX_USB_MESSAGE_SIZE;
	mlp.com_type    = COM_USB;
	mlp.return_func = usb_message_loop_return;
	com_message_loop(&mlp);
}

// Configure 48MHz Clock for USB
void usb_configure_clock_48mhz(void) {
    // Use PLLB for USB

#if (BOARD_MAINOSC == 16000000)
	PMC->CKGR_PLLBR = CKGR_PLLBR_DIVB(1) |
                      CKGR_PLLBR_MULB(5) |
                      CKGR_PLLBR_PLLBCOUNT_Msk;
    while((PMC->PMC_SR & PMC_SR_LOCKB) == 0);

	//               divide by 2         use PLLB
	PMC->PMC_USB = PMC_USB_USBDIV(1) | PMC_USB_USBS;
#elif (BOARD_MAINOSC == 12000000)
	PMC->CKGR_PLLBR = CKGR_PLLBR_DIVB(1) |
                      CKGR_PLLBR_MULB(7) |
                      CKGR_PLLBR_PLLBCOUNT_Msk;
    while((PMC->PMC_SR & PMC_SR_LOCKB) == 0);

	//               divide by 2         use PLLB
	PMC->PMC_USB = PMC_USB_USBDIV(1) | PMC_USB_USBS;
#endif
}


void USBDCallbacks_RequestReceived(const USBGenericRequest *request) {
    USBDDriver_RequestHandler(&usbd_driver, request);
}
