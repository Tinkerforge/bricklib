/* bricklib
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * system_timer.h: Simple system tick counter
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
**/

#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H

#include <stdint.h>
#include <stdbool.h>

uint32_t system_timer_get_ms(void);
uint32_t system_timer_get_us(void);
bool system_timer_is_time_elapsed_ms(const uint32_t start_measurement, const uint32_t time_to_be_elapsed);
void system_timer_sleep_ms(const uint32_t sleep);

#endif
