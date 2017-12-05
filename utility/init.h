/* bricklib
 * Copyright (C) 2010-2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * init.h: Implementation of initialization valid for all bricks
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

#ifndef INIT_H
#define INIT_H

#include <stdint.h>
#include <stdbool.h>
#include "bricklib/com/com.h"

#define TICK_TASK_TYPE_MESSAGE 1
#define TICK_TASK_TYPE_CALCULATION 2

void SUPC_IrqHandler(void);

void brick_init(void);
void brick_init_new_connection(void);
void brick_enable_brownout_detection(void);
void brick_tick_task(void *parameters);
void brick_init_start_tick_task(void);
void brick_reset(void);
void brick_init_handle_bricklet_enumeration(void);

#endif
