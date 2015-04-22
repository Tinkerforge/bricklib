/* bricklib
 * Copyright (C) 2010-2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_callbacks.c: USB callback functions
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

#include "usb_callbacks.h"

#include "config.h"
#include "bricklib/drivers/cmsis/core_cm3.h"
#include "bricklib/logging/logging.h"
#include "bricklib/utility/init.h"

bool usb_first_connection = true;

uint8_t reset_counter = 0;

// Invoked after the USB driver has been initialized.
// Configures the UDP/UDPHS interrupt.
void USBDCallbacks_Initialized(void) {
    NVIC_EnableIRQ(UDP_IRQn);
	logi("USBDCallbacks_Initialized\n\r");
}

void USBDCallbacks_Resumed(void) {
	logi("USBDCallbacks_Resumed\n\r");
	if(!usb_first_connection) {
		logi("Brick will reset in 255ms\n\r");
		reset_counter = 1;
	}
}

void USBDCallbacks_Suspended(void) {
	logi("USBDCallbacks_Suspended\n\r");
	if(!usb_first_connection) {
		logi("Brick will reset in 255ms\n\r");
		reset_counter = 1;
	}
}
