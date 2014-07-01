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

extern uint8_t reset_counter;

extern const USBDDriverDescriptors driver_descriptors;
static USBDDriver usbd_driver;
static uint32_t usb_send_transferred = 0;
uint32_t usb_recv_transferred = 0;

bool usb_startup_connected = false;
static uint8_t usb_detect_task_counter = 0;
static uint8_t receive_status = 0;
static uint8_t send_status = 0;

uint32_t usb_num_send_tries = NUM_SEND_TRIES;

char usb_recv_buffer[DEFAULT_EP_SIZE];
char usb_send_buffer[DEFAULT_EP_SIZE];

static Pin pin_usb_detect = PIN_USB_DETECT;

uint32_t usb_sequence_number = 0;

void usb_send_callback(void *arg,
                       uint8_t status,
                       uint32_t transferred,
                       uint32_t remaining) {
	static int32_t higher_priority_task_woken;

	usb_send_transferred = transferred;

	if((uint32_t)arg == usb_sequence_number) {
		send_status |= USB_CALLBACK;
	}
	portEND_SWITCHING_ISR(higher_priority_task_woken);
}

inline uint16_t usb_send(const void *data, const uint16_t length, uint32_t *options) {
	if((send_status & (USB_IN_FUNCTION))) {
		return 0;
	}

	if(!(send_status & USB_CALLBACK)) {
		send_status |= USB_IN_FUNCTION;
		if(USBD_Write(IN_EP, data, length, usb_send_callback, (void*)usb_sequence_number) != USBD_STATUS_SUCCESS) {
			send_status &= ~USB_IN_FUNCTION;
			return 0;
		}
	} else {
		return 0;
	}

	uint32_t num_tries = 0;
	while(~send_status & USB_CALLBACK) {
		taskYIELD();
		num_tries++;
		// USBD_Write does not always call callback when USBD_STATUS_SUCCESS
		// Wait for NUM_SEND_TRIES
		if(num_tries > usb_num_send_tries) {
			usb_sequence_number++;
			send_status = 0;

			// We sometimes come to this state if the PC/laptop is on suspend to disk.
			// In that case it seems that the USB is in LOCKED state forever.
			// So we are cautionary and cancel all IO.

			// TODO: Fixme: Seems to make problems on some Windows versions!
			//USBD_HAL_CancelIo(0xFFFF);
			return 0;
		}
	}

	send_status = 0;

	// We assume that the sending worked out OK if usb_send_transferred is
	// not equal 0. This is necessary, since USBD_HAL sometimes returns
	// UINT32_MAX for unknown reasons.
	if(usb_send_transferred == 0) {
		return 0;
	}

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
	return adc_channel_get_data(USB_VOLTAGE_CHANNEL) > VOLTAGE_MAX_VALUE*2/3;
#else
	return PIO_Get(&pin_usb_detect);
#endif
}

void usb_detect_configure(void) {
	 PIO_Configure(&pin_usb_detect, 1);
	 pin_usb_detect.attribute = PIO_PULLDOWN;
	 PIO_Configure(&pin_usb_detect, 1);
	 // We use usb_detect_task instead of interrupt
}

void usb_detect_task(const uint8_t tick_type) {
	if(tick_type == TICK_TASK_TYPE_CALCULATION) {
		// Reset through usb resume
		if(reset_counter > 0) {
			reset_counter++;
			if(reset_counter == 255) {
				brick_reset();
			}
		}

// Only use USB detect reset on Master Brick
// It makes problems on other Bricks and has little use
#ifdef BRICK_CAN_BE_MASTER
		// Reset through usb detect
		if(usb_startup_connected ^ usb_is_connected()) {
			usb_detect_task_counter++;
			if(usb_detect_task_counter >= 250) {
				logi("USB detect: %d\n\r", adc_channel_get_data(USB_VOLTAGE_CHANNEL));
				brick_reset();
			}
		} else {
			usb_detect_task_counter = 0;
		}
#endif
	}
}

bool usb_init() {
    if(!usb_is_connected()) {
    	usb_startup_connected = false;
    	return false;
    }

    usb_startup_connected = true;

	send_status = 0;
	receive_status = 0;

	usb_configure_clock_48mhz();

    USBDDriver_Initialize(&usbd_driver, &driver_descriptors, 0);
    USBD_Init();

    // Check current level on VBus
    if(usb_is_connected()) {
        USBD_Connect();
    } else {
        USBD_Disconnect();
        return false;
    }

	return true;
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
