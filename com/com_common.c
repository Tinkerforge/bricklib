/* bricklib
 * Copyright (C) 2009-2012 Olaf Lüke <olaf@tinkerforge.com>
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

#include "com_messages.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"

#include "bricklib/bricklet/bricklet_config.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/led.h"

#ifdef BRICK_CAN_BE_MASTER
#include "routing.h"
#endif

extern uint32_t led_rxtx;
extern uint32_t com_blocking_trials[];

extern ComInfo com_info;
extern const BrickletAddress baddr[];
extern BrickletSettings bs[];

uint16_t send_blocking_with_timeout_options(const void *data,
                                            const uint16_t length,
                                            const ComType com,
                                            uint32_t *options) {
	uint16_t bytes_send = 0;
	uint32_t trials = com_blocking_trials[com];

	while(length - bytes_send != 0 && trials--) {
		bytes_send += SEND(data + bytes_send, length - bytes_send, com, options);
		taskYIELD();
	}

	led_rxtx++;
	return bytes_send;
}

uint16_t send_blocking_with_timeout(const void *data,
                                    const uint16_t length,
                                    const ComType com) {
	return send_blocking_with_timeout_options(data, length, com, NULL);
}

void com_handle_setter(const ComType com, void *message) {
	MessageHeader *header = message;
	if(header->return_expected) {
		send_blocking_with_timeout(message, header->length, com);
	}
}

void com_make_default_header(void *message,
                             const uint32_t uid,
                             const uint8_t length,
                             const uint8_t fid) {
	MessageHeader *header = message;

	header->uid              = uid;
	header->length           = length;
	header->fid              = fid;
	header->sequence_num     = 0; // Sequence number for callback is 0
	header->return_expected  = 1;
	header->authentication   = 0; // TODO
	header->other_options    = 0;
	header->error            = 0;
	header->future_use       = 0;
}

void com_message_loop(void *parameters) {
	MessageLoopParameter *mlp = (MessageLoopParameter*)parameters;

	char data[mlp->buffer_size];
	int32_t length;
	int32_t received;

	while(true) {
		length = 0;

		while(length < SIZE_OF_MESSAGE_HEADER) {
			received = RECV(data + length,
			                mlp->buffer_size - length,
			                mlp->com_type,
			                NULL);
			if(received == 0) {
				taskYIELD();
			} else {
				length += received;
			}
		}

		MessageHeader *header = (MessageHeader*)data;

		// TODO: If header->length > 80: Out-Of-Sync-Handling

		while(length < header->length) {
			received = RECV(data + length,
			                header->length - length,
			                mlp->com_type,
			                NULL);
			if(received == 0) {
				taskYIELD();
			} else {
				length += received;
			}
		}

		// com_debug_message(header);

		led_rxtx++;
		mlp->return_func(data, header->length);
	}
}

void com_route_message_from_pc(const char *data, const uint16_t length, const ComType com) {
	if(!com_route_message_brick(data, length, com)) {
#ifdef BRICK_CAN_BE_MASTER
		routing_master_from_pc(data, length);
#endif
	}
}

bool com_route_message_brick(const char *data, const uint16_t length, const ComType com) {
	com_info.current = com;

	MessageHeader *header = (MessageHeader*)data;
	if(header->uid == 0) {
		const ComMessage *com_message = get_com_from_header(header);
		if(com_message != NULL && com_message->reply_func != NULL) {
			com_message->reply_func(com, (void*)data);
		}

		return false;
	} else if(header->uid == com_info.uid) {
		const ComMessage *com_message = get_com_from_header(header);
		if(com_message != NULL && com_message->reply_func != NULL) {
			com_message->reply_func(com, (void*)data);
			return true;
		} else {
			com_return_error(data, sizeof(MessageHeader), MESSAGE_ERROR_CODE_NOT_SUPPORTED, com);
		}

		return false;
	}

	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].uid == header->uid) {
			if(header->fid == FID_GET_IDENTITY) {
				const ComMessage *com_message = get_com_from_header(header);
				if(com_message != NULL && com_message->reply_func != NULL) {
					com_message->reply_func(com, (void*)data);
					return true;
				} else {
					com_return_error(data, sizeof(MessageHeader), MESSAGE_ERROR_CODE_NOT_SUPPORTED, com);
				}
			} else {
				baddr[i].entry(BRICKLET_TYPE_INVOCATION, com, (void*)data);
				return true;
			}
		}
	}

	return false;
}

void com_return_error(const void *data, const uint8_t ret_length, const uint8_t error_code, const ComType com) {
	MessageHeader *message = (MessageHeader*)data;

	if(!message->return_expected) {
		return;
	}

	uint8_t ret_data[ret_length];
	MessageHeader *ret_message = (MessageHeader*)&ret_data;

	memset(ret_data, 0, ret_length);
	*ret_message = *message;
	ret_message->length = ret_length;
	ret_message->error = error_code;

	send_blocking_with_timeout(ret_data, ret_length, com);
}

void com_return_setter(const ComType com, const void *data) {
	if(((MessageHeader*)data)->return_expected) {
		MessageHeader ret = *((MessageHeader*)data);
		ret.length = sizeof(MessageHeader);
		send_blocking_with_timeout(&ret, sizeof(MessageHeader), com);
	}
}

void com_debug_message(const MessageHeader *header) {
	printf("Message UID: %lu", header->uid);
	printf(", length: %d", header->length);
	printf(", fid: %d", header->fid);
	printf(", (seq#, r, a, oo): (%d, %d, %d, %d)", header->sequence_num, header->return_expected, header->authentication, header->other_options);
	printf(", (e, fu): (%d, %d)\n\r", header->error, header->future_use);

	printf("Message data: [");

	uint8_t *data = (uint8_t*)header;
	for(uint8_t i = 0; i < header->length - 8; i++) {
		printf("%d ", data[i+8]);
	}
	printf("]\n\r");
}
