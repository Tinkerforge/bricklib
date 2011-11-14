/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
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

#include <pio/pio.h>
#include <pio/pio_it.h>
#include <pmc/pmc.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <string.h>
#include <usb/USBDescriptors.h>
#include <usb/USBDDriver.h>
#include <usb/USBD.h>
#include <usb/USBD_HAL.h>

#include "bricklib/com/com_messages.h"
#include "bricklib/com/com.h"
#include "bricklib/com/com_common.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/logging/logging.h"
#include "bricklib/com/spi/spi_stack/spi_stack_master.h"
#include "bricklib/utility/led.h"
#include "bricklib/bricklet/bricklet_config.h"

#include "usb_descriptors.h"
#include "config.h"

#define MAX_USB_MESSAGE_SIZE 64

#define USB_IN_FUNCTION 1
#define USB_CALLBACK 2

extern const USBDDriverDescriptors driver_descriptors;

extern ComType com_ext[];
extern uint8_t master_routing_table[];
extern uint8_t com_last_spi_stack_id;
extern uint8_t com_last_ext_id[];
extern uint8_t com_last_stack_address;
extern uint8_t com_stack_id;
extern const BrickletAddress baddr[];
extern BrickletSettings bs[];

static USBDDriver usbd_driver;
static const Pin pin_usb_detect = PIN_USB_DETECT;

static unsigned int usb_send_transferred = 0;
unsigned int usb_recv_transferred = 0;

static uint8_t receive_status = 0;
static uint8_t send_status = 0;

char usb_recv_buffer[DEFAULT_EP_SIZE];
char usb_send_buffer[DEFAULT_EP_SIZE];

void usb_send_callback(void *arg,
                       uint8_t status,
                       uint32_t transferred,
                       uint32_t remaining) {
	static int32_t higher_priority_task_woken;

	usb_send_transferred = transferred;

	send_status |= USB_CALLBACK;
	portEND_SWITCHING_ISR(higher_priority_task_woken);
}

inline uint16_t usb_send(const void *data, const uint16_t length) {
/*	if(USBD_HAL_Write(ep, data, length) != USBD_STATUS_SUCCESS) {
		return 0;
	}

	return length;*/

	if((send_status & (USB_IN_FUNCTION))) {
		return 0;
	}

	if(!(send_status & USB_CALLBACK)) {
		send_status |= USB_IN_FUNCTION;
		if(USBD_Write(IN_EP, data, length, usb_send_callback, 0) != USBD_STATUS_SUCCESS) {
			send_status &= ~USB_IN_FUNCTION;
			return 0;
		}
	} else {
		return 0;
	}

	uint16_t num_tries = 0;
	while(~send_status & USB_CALLBACK) {
		taskYIELD();
		num_tries++;
		// USBD_Write does not always call callback when USBD_STATUS_SUCCESS
		// Wait for NUM_RECEIVE_TRIES
		if(num_tries > NUM_SEND_TRIES) {
			send_status = 0;
			return 0;
		}
	}

	send_status = 0;

	return usb_send_transferred;
}

// usb receive is implemented with a hook in the usb protocol state machine
// from atmel (see USBD_HAL.c). Unfortunately the USBD_Read is not reliable,
// it does not always invoke the callback function. Since i could not fix this,
// we write the usb_recv_buffer directly to the endpoint whenever the pc
// polls and there is data available. This is a little bit faster and seems
// to work very reliable.
inline uint16_t usb_recv(void *data, const uint16_t length) {
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

void usb_isr_vbus(const Pin *pin) {
    // Check current level on VBus
    if (PIO_Get(&pin_usb_detect)) {
        USBD_Connect();
    } else {
        USBD_Disconnect();
    }
}

bool usb_is_connected(void) {
	return PIO_Get(&pin_usb_detect);
}

void usb_detect_configure(void) {
    // Configure PIO
    PIO_Configure(&pin_usb_detect, 1);
    PIO_ConfigureIt(&pin_usb_detect, usb_isr_vbus);
    PIO_EnableIt(&pin_usb_detect);
}

bool usb_init() {
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

void usb_message_loop_return(char *data, uint16_t length) {
	const uint8_t stack_id = get_stack_id_from_data(data);

	if(stack_id == com_stack_id || stack_id == 0) {
		const ComMessage *com_message = get_com_from_data(data);
		if(com_message->reply_func != NULL) {
			com_message->reply_func(COM_USB, (void*)data);
			return;
		}
	}
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].stack_id == stack_id) {
			baddr[i].entry(BRICKLET_TYPE_INVOCATION, COM_USB, (void*)data);
			return;
		}
	}

	if(stack_id <= com_last_spi_stack_id) {
		send_blocking_with_timeout(data, length, COM_SPI_STACK);
		return;
	}

	if(stack_id <= com_last_ext_id[0]) {
		send_blocking_with_timeout(data, length, com_ext[0]);
		return;
	}
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
