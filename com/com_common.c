/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * com_common.c: functions common to all communication protocols
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

#include "com_common.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/led.h"

extern uint32_t led_rxtx;
extern uint32_t com_blocking_trials[];

uint16_t send_blocking_with_timeout(const void *data,
                                    const uint16_t length,
                                    ComType com) {
	uint16_t bytes_send = 0;
	uint32_t trials = com_blocking_trials[com];

	while(length - bytes_send != 0 && trials--) {
		bytes_send += SEND(data + bytes_send, length - bytes_send, com);
		taskYIELD();
	}

	led_rxtx++;
	return bytes_send;
}

void com_message_loop(void *parameters) {
	MessageLoopParameter *mlp = (MessageLoopParameter*)parameters;

	char data[mlp->buffer_size];
	int32_t length;
	int32_t received;
	int32_t message_length;

	while(true) {
		length = 0;

		while(length < SIZE_OF_MESSAGE_TYPE) {
			received = RECV(data + length,
			                mlp->buffer_size - length,
			                mlp->com_type);
			if(received == 0) {
				taskYIELD();
			} else {
				length += received;
			}
		}
		led_rxtx++;

		message_length = get_length_from_data(data);

		if(length == message_length) {
			mlp->return_func(data, message_length);
		} else {
			char long_data[message_length];
			memcpy(long_data, data, length);

			while(length < message_length) {
				received = RECV(long_data + length,
				                message_length - length,
								mlp->com_type);

				if(received == 0) {
					taskYIELD();
				} else {
					length += received;
				}
			}
			mlp->return_func(long_data, message_length);
		}

		led_rxtx++;
	}
}
