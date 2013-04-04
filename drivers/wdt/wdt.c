/* bricklib
 * Copyright (C) 2013 Olaf Lüke <olaf@tinkerforge.com>
 *
 * wdt.c: Simple watchdog timer implementation
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

#include "config.h"
#include "wdt.h"

#include <stdint.h>

uint32_t wdt_counter = 0;

// Configures the watchdog timer to 16s (can only be called once).
void wdt_start(void) {
	if(wdt_has_error()) {
		// If there is an error in the wdt status register the watchdog
		// timer reached 0 since last call of start
		wdt_counter++;
	}

	WDT->WDT_MR = WDT_MR_WDV(WDT_TIMEOUT_16S) |
	              WDT_MR_WDD(WDT_TIMEOUT_16S) |
	              WDT_MR_WDFIEN |
	              WDT_MR_WDRSTEN |
	              WDT_MR_WDDBGHLT |
	              WDT_MR_WDIDLEHLT;
}

// Stops the watchdog timer (can only be called once).
void wdt_stop(void) {
	WDT->WDT_MR = WDT_MR_WDDIS;
}

// Restarts the watchdog timer, has to be called at least every 16s.
void wdt_restart(void) {
	WDT->WDT_CR = 0xA5000001;
}

bool wdt_has_error(void) {
	return (WDT->WDT_SR & (WDT_SR_WDERR | WDT_SR_WDUNF)) != 0;
}

uint32_t wdt_get_counter(void) {
	return wdt_counter;
}
