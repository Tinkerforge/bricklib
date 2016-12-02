/* bricklib
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * system_timer.c: Simple system tick counter
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

#include "system_timer.h"

#include "config.h"

volatile uint32_t system_timer_tick;

uint32_t system_timer_get_ms(void) {
	return system_timer_tick;
}

uint32_t system_timer_get_us(void) {
#ifdef __XMC1__
	return system_timer_tick*1000 + (((SystemCoreClock/1000) - SysTick->VAL)*1000)/(SystemCoreClock/1000);
#else
	return 0; // Implement for other MCUs here
#endif
}

// This will work even with wrap-around up to UIN32_MAX/2 difference.
// E.g.: end - start = 0x00000010 - 0xfffffff = 0x00000011 etc
inline bool system_timer_is_time_elapsed_ms(const uint32_t start_measurement, const uint32_t time_to_be_elapsed) {
	return (system_timer_get_ms() - start_measurement) >= time_to_be_elapsed;
}

inline bool system_timer_is_time_elapsed_us(const uint32_t start_measurement, const uint32_t time_to_be_elapsed) {
	return (system_timer_get_us() - start_measurement) >= time_to_be_elapsed;
}

void system_timer_sleep_ms(const uint32_t sleep) {
	const uint32_t time = system_timer_get_ms();
	while(!system_timer_is_time_elapsed_ms(time, sleep));
}

void system_timer_sleep_us(const uint32_t sleep) {
	const uint32_t time = system_timer_get_us();
	while(!system_timer_is_time_elapsed_us(time, sleep));
}
