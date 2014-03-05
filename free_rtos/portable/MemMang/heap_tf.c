/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * heap_tf.c: Super simple heap implementation (we don't use free)
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

#include "bricklib/utility/syscalls.h"

// Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
// all the API functions to use the MPU wrappers.  That should only be done when
// task.h is included from an application file.
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

void *pvPortMalloc(size_t incr) {
	vTaskSuspendAll();
	void *ret = _sbrk(incr);
	xTaskResumeAll();

	return ret;
}

// We don't use free
void vPortFree(void *pv) {}



