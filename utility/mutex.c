/* bricklib
 * Copyright (C) 2011 Olaf Lüke <olaf@tinkerforge.com>
 *
 * mutex.c: Mutex functionality
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

#include "mutex.h"

#include "config.h"

#include <stdint.h>
#include <stdbool.h>


Mutex mutex_twi_bricklet;

void mutex_init() {
	mutex_twi_bricklet = xSemaphoreCreateMutex();
}

Mutex mutex_create(void) {
	return xSemaphoreCreateMutex();
}

bool mutex_take(Mutex mutex, uint32_t time) {
	return xSemaphoreTake(mutex, time);
}

bool mutex_give(Mutex mutex) {
	return xSemaphoreGive(mutex);
}
