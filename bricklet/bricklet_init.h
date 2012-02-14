/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_init.h: functions for bricklet initialization
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

#ifndef BRICKLET_INIT_H
#define BRICKLET_INIT_H

#include <pio/pio.h>
#include <stdint.h>
#include <stdbool.h>

void bricklet_write_asc_to_flash(const uint8_t bricklet);
void bricklet_write_plugin_to_flash(const char *plugin,
                                    const uint8_t position,
                                    const uint8_t bricklet);
bool bricklet_init_plugin(const uint8_t bricklet);
void bricklet_select(const uint8_t bricklet);
void bricklet_deselect(const uint8_t bricklet);
void bricklet_init(void);
void bricklet_clear_eeproms(void);
void bricklet_try_connection(const uint8_t bricklet);
void bricklet_tick_task(uint8_t tick_type);

#endif
