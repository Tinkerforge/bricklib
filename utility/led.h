/* bricklib
 * Copyright (C) 2011 Olaf Lüke <olaf@tinkerforge.com>
 *
 * led.c: led controlling functions
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

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

// Blink for 30ms and next blinking after at least 100ms
#define LED_RXTX_OFF     35
#define LED_RXTX_RESTART 200

// Num rx or tx messages before blinking
#define LED_RXTX_NUM     20

void led_init();
void led_on(const uint8_t led_num);
void led_off(const uint8_t led_num);
void led_toggle(const uint8_t led_num);
bool led_is_on(const uint8_t led_num);
void led_blink(const uint8_t led_num, const uint32_t cycles);
void led_tick_task(void);

#endif
