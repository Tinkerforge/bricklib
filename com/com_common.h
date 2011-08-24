/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * com_common.h: functions common to all communication protocols
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

#ifndef COM_COMMON_H
#define COM_COMMON_H

#include <pio/pio.h>
#include <stdbool.h>
#include <stdint.h>

#include "com_messages.h"

typedef void (*function_message_loop_return_t)(char *, uint16_t);

typedef struct {
	uint16_t buffer_size;
	ComType com_type;
	function_message_loop_return_t return_func;
} MessageLoopParameter;

void com_message_loop(void *parameters);
uint16_t send_blocking_with_timeout(const void *data,
                                    const uint16_t length,
                                    ComType com);

#endif
