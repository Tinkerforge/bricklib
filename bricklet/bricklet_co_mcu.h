/* bricklib
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_co_mcu.h: Functions for Bricklets with co-processor
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

#ifndef BRICKLET_CO_MCU_H
#define BRICKLET_CO_MCU_H

#include <stdint.h>
#include "bricklib/utility/ringbuffer.h"

#define CO_MCU_BUFFER_SIZE_SEND 80
#define CO_MCU_BUFFER_SIZE_RECV 140
#define CO_MCU_DEFAULT_BAUDRATE 1400000

typedef enum {
	STATE_START,
	STATE_ACK_SEQUENCE_NUMBER,
	STATE_ACK_CHECKSUM,
	STATE_MESSAGE_SEQUENCE_NUMBER,
	STATE_MESSAGE_DATA,
	STATE_MESSAGE_CHECKSUM
} CoMCURecvState;

typedef union {
	struct {
		uint8_t got_message:1;
		uint8_t tries:7;
	} access;
	uint8_t data;
} CoMCURecvAvailability;

typedef struct {
	uint32_t error_count_ack_checksum;
	uint32_t error_count_message_checksum;
	uint32_t error_count_frame;
} CoMCUSPITFPErrorCount;

typedef struct {
	CoMCUSPITFPErrorCount error_count;
	CoMCURecvAvailability availability;
	uint8_t buffer_send_length;
	int16_t buffer_send_ack_timeout;
	uint8_t current_sequence_number;
	uint8_t last_sequence_number_seen;
	bool first_enumerate_send;
	uint8_t buffer_send[CO_MCU_BUFFER_SIZE_SEND];
	uint8_t buffer_recv[CO_MCU_BUFFER_SIZE_RECV];
	Ringbuffer ringbuffer_recv;
} CoMCUData;

#define CO_MCU_DATA(i) ((CoMCUData*)(bc[i]))

void bricklet_co_mcu_poll(const uint8_t bricklet_num);
void bricklet_co_mcu_send(const uint8_t bricklet_num, uint8_t *data, const uint8_t length);
void bricklet_co_mcu_init(const uint8_t bricklet_num);

#endif
