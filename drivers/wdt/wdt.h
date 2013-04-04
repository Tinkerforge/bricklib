/* bricklib
 * Copyright (C) 2013 Olaf Lüke <olaf@tinkerforge.com>
 *
 * wdt.h: Simple watchdog timer implementation
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

#ifndef WDT_H
#define WDT_H

#include <stdint.h>

#define WDT_TIMEOUT_16S 0xFFF

void wdt_start(void);
void wdt_stop(void);
void wdt_restart(void);
bool wdt_has_error(void);
uint32_t wdt_get_counter(void);

#endif
