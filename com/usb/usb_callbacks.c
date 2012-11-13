/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
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

// Invoked after the USB driver has been initialized.
// Configures the UDP/UDPHS interrupt.
void USBDCallbacks_Initialized(void) {
    NVIC_EnableIRQ(UDP_IRQn);
}

void USBDCallbacks_Resumed(void) {
	logi("UDP_ENDPOINT_IDLE\n\r");
}

void USBDCallbacks_Suspended(void) {
	logi("USBDCallbacks_Suspended\n\r");
}
