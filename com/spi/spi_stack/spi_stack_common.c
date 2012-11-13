/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_common.c: SPI stack functions common to master and slave
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

#include "spi_stack_common.h"

#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bricklib/drivers/spi/spi.h"

#include "bricklib/com/com_messages.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/drivers/crc/crc.h"

#include "spi_stack_select.h"
#include "config.h"

extern uint32_t led_rxtx;

// Recv and send buffer for SPI stack (written by spi_stack_send/recv)
uint8_t spi_stack_buffer_recv[SPI_STACK_BUFFER_SIZE] = {0};
uint8_t spi_stack_buffer_send[SPI_STACK_BUFFER_SIZE] = {0};

// Recv and send buffer size for SPI stack (written by spi_stack_send/recv)
uint16_t spi_stack_buffer_size_send = 0;
uint16_t spi_stack_buffer_size_recv = 0;

int8_t spi_stack_send_to = -1;

extern uint8_t com_last_stack_address;

uint16_t spi_stack_send(const void *data, const uint16_t length, uint32_t *options) {
	if(spi_stack_buffer_size_send > 0) {
		return 0;
	}

	if(options && *options >= SPI_ADDRESS_MIN && *options <= com_last_stack_address) {
		spi_stack_send_to = *options;
	} else {
		spi_stack_send_to = -1;
	}

	led_rxtx++;

	uint16_t send_length = MIN(length, SPI_STACK_BUFFER_SIZE);

	memcpy(spi_stack_buffer_send, data, send_length);
	spi_stack_buffer_size_send = send_length;

	return send_length;
}

uint16_t spi_stack_recv(void *data, const uint16_t length, uint32_t *options) {
	if(spi_stack_buffer_size_recv == 0) {
		return 0;
	}

	led_rxtx++;

	static uint16_t recv_pointer = 0;

	uint16_t recv_length = MIN(length, spi_stack_buffer_size_recv);

	memcpy(data, spi_stack_buffer_recv + recv_pointer, recv_length);

	if(spi_stack_buffer_size_recv - recv_length == 0) {
		recv_pointer = 0;
	} else {
		recv_pointer += recv_length;
	}

	spi_stack_buffer_size_recv -= recv_length;

	return recv_length;
}
