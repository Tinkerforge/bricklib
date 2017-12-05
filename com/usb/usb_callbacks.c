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
#include "bricklib/drivers/usb/USBD.h"
#include "bricklib/logging/logging.h"
#include "bricklib/utility/init.h"

#include <stdbool.h>

extern bool usb_first_connection;
extern uint8_t usb_wakeup_counter;
extern bool usbd_hal_configuration_seen;

void USBDDriverCallbacks_ConfigurationChanged(uint8_t cfgnum) {
	logi("USBDDriverCallbacks_ConfigurationChanged: %d\n\r", cfgnum);
	usbd_hal_configuration_seen = true;
}

// Invoked after the USB driver has been initialized.
// Configures the UDP/UDPHS interrupt.
void USBDCallbacks_Initialized(void) {
    NVIC_EnableIRQ(UDP_IRQn);
	logi("USBDCallbacks_Initialized\n\r");
}

void USBDCallbacks_Resumed(void) {
	logi("USBDCallbacks_Resumed\n\r");

	// Send initial enumeration again
	usb_first_connection = true;
}

void USBDCallbacks_Suspended(void) {
	logi("USBDCallbacks_Suspended: %d\n\r", usb_first_connection);

	// Start wakeup counter.
	// It is possible that the Brick gets suspended because of an EMI event.
	// In this case we don't actually want to go to sleep. So we start a
	// wake-up sequence to show the PC that we are still alive.
	// The wakeup will be triggered within 255ms. If the PC itself issues
	// a resume within this time, the wakeup will be cancelled.
	usb_wakeup_counter = 1;
}
