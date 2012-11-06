/* bricklib
 * Copyright (C) 2009-2012 Olaf Lüke <olaf@tinkerforge.com>
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

#include <stdbool.h>
#include <stdint.h>

#include "com.h"

#define MESSAGE_LOOP_SIZE 550

#define MESSAGE_EMPTY_INITIALIZER {{0}}

typedef void (*function_message_loop_return_t)(char *, uint16_t);

typedef struct {
	uint16_t buffer_size;
	ComType com_type;
	function_message_loop_return_t return_func;
} MessageLoopParameter;

typedef struct {
	uint32_t uid;
	uint8_t length;
	uint8_t fid;
	uint8_t sequence_num:4,
	        return_expected:1,
	        authentication:1,
	        other_options:2;
	uint8_t error:1,
	        future_use:7;
} __attribute__((__packed__)) MessageHeader;

typedef struct {
	uint32_t uid;
	uint32_t data;
} __attribute__((__packed__)) MessageHeaderSimple;

void com_handle_setter(uint8_t com, void *message);
void com_message_loop(void *parameters);
void com_make_default_header(void *message,
                             uint32_t uid,
                             uint8_t length,
                             uint8_t fid);
uint16_t send_blocking_with_timeout(const void *data,
                                    const uint16_t length,
                                    ComType com);
bool com_route_message_brick(char *data, uint16_t length, ComType com);
void com_route_message_from_pc(char *data, uint16_t length, ComType com);
void com_return_error(const void *data, const uint8_t ret_length, ComType com);
void com_return_setter(ComType com, const void *data);
void com_debug_message(MessageHeader *header);

#endif
