/* bricklib
 * Copyright (C) 2009-2013 Olaf Lüke <olaf@tinkerforge.com>
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

#ifdef BRICK_HAS_CO_MCU_SUPPORT
#include "bricklib/bricklet/bricklet_co_mcu.h"
#endif

#include "bricklib/drivers/wdt/wdt.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/led.h"
#include "bricklib/utility/system_timer.h"

#ifdef BRICK_CAN_BE_MASTER
#ifndef BRICK_EXCLUDE_BRICKD
#include "routing.h"
#endif
#endif

extern uint32_t led_rxtx;
extern uint32_t com_blocking_timeout[];

extern ComInfo com_info;
extern const BrickletAddress baddr[];
extern BrickletSettings bs[];
extern uint8_t bricklet_attached[];

void send_blocking_options(const void *data,
                           const uint16_t length,
                           const ComType com,
                           uint32_t *options) {
	uint16_t bytes_send = 0;

	while(length - bytes_send != 0) {
		bytes_send += SEND(data + bytes_send, length - bytes_send, com, options);
		taskYIELD();
	}

	led_rxtx++;
}

void send_blocking(const void *data,
                   const uint16_t length,
                   const ComType com) {
	return send_blocking_options(data, length, com, NULL);
}

uint16_t send_blocking_with_timeout_options(const void *data,
                                            const uint16_t length,
                                            const ComType com,
                                            uint32_t *options) {
	uint16_t bytes_send = 0;
	uint32_t time_start = system_timer_get_ms();

	while(length - bytes_send != 0) {
		if(system_timer_is_time_elapsed_ms(time_start, com_blocking_timeout[com])) {
			com_timeout_count[com]++;
			break;
		}
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
		taskYIELD();
	}
}

void com_route_message_from_pc(const char *data, const uint16_t length, const ComType com) {
	if(!com_route_message_brick(data, length, com)) {
#ifdef BRICK_CAN_BE_MASTER
#ifndef BRICK_EXCLUDE_BRICKD
		routing_master_from_pc(data, length, com);
#endif
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

#ifdef BRICK_HAS_CO_MCU_SUPPORT
		for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
			if(bricklet_attached[i] == BRICKLET_INIT_CO_MCU) {
				bricklet_co_mcu_send(i, (void*)data, length);
			}
		}
#endif

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
		// Check if UID belongs to Bricklet or to Isolator Bricklet connected
		// between Brick and Bricklet
		if((bs[i].uid == header->uid) || (bs[i].uid_isolator == header->uid)) {
#ifdef BRICK_HAS_CO_MCU_SUPPORT
			if(bricklet_attached[i] == BRICKLET_INIT_CO_MCU) {
				bricklet_co_mcu_send(i, (void*)data, length);
			} else 
#endif
				
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

void com_forward_message(const ComType com, const MessageHeader *data) {
	logd("com forward from %d (fid %d)\n\r", com, data->fid);
	if(com == COM_WIFI2) {
		send_blocking_with_timeout(data, data->length, COM_USB);
	} else {
		send_blocking_with_timeout(data, data->length, COM_WIFI2);
	}
}

void com_debug_message(const MessageHeader *header) {
#if LOGGING_LEVEL == LOGGING_DEBUG
	logwohd("Message UID: %lu", header->uid);
	logwohd(", length: %d", header->length);
	logwohd(", fid: %d", header->fid);
	logwohd(", (seq#, r, a, oo): (%d, %d, %d, %d)", header->sequence_num, header->return_expected, header->authentication, header->other_options);
	logwohd(", (e, fu): (%d, %d)\n\r", header->error, header->future_use);

	logwohd("Message data: [");

	uint8_t *data = (uint8_t*)header;
	for(uint8_t i = 0; i < header->length - 8; i++) {
		logwohd("%d ", data[i+8]);
	}
	logwohd("]\n\r");
#endif
}
