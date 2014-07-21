/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_common_dma.c: SPI stack DMA functions common to master and slave
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

#include "spi_stack_common_dma.h"

#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bricklib/drivers/spi/spi.h"

#include "bricklib/com/com_messages.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/pearson_hash.h"
#include "bricklib/drivers/crc/crc.h"

#include "spi_stack_select.h"
#include "spi_stack_slave_dma.h"
#include "spi_stack_master_dma.h"
#include "config.h"

// Recv and send buffer for SPI stack (written by spi_stack_send/recv)
uint8_t spi_stack_buffer_recv[SPI_STACK_MAX_MESSAGE_LENGTH] = {0};
uint8_t spi_stack_buffer_send[SPI_STACK_MAX_MESSAGE_LENGTH] = {0};

// Recv and send buffer size for SPI stack (written by spi_stack_send/recv)
uint16_t spi_stack_buffer_size_send = 0;
uint16_t spi_stack_buffer_size_recv = 0;



#ifdef BRICK_CAN_BE_MASTER
extern uint8_t master_mode;
#endif

void SPI_IrqHandler(void) {
#ifdef BRICK_CAN_BE_MASTER
	if(master_mode & MASTER_MODE_MASTER) {
		spi_stack_master_irq();
	} else if(master_mode & MASTER_MODE_SLAVE) {
		spi_stack_slave_irq();
	}
#else
	spi_stack_slave_irq();
#endif
}

uint8_t spi_stack_calculate_pearson(const uint8_t *data, const uint8_t length) {
	uint8_t checksum = 0;
	for(uint8_t i = 0; i < length; i++) {
		PEARSON(checksum, data[i]);
	}

	return checksum;
}

uint16_t spi_stack_send(const void *data, const uint16_t length, uint32_t *options) {
#ifdef BRICK_CAN_BE_MASTER
	if(master_mode & MASTER_MODE_MASTER) {
		return spi_stack_master_send(data, length, options);
	} else if(master_mode & MASTER_MODE_SLAVE) {
		return spi_stack_slave_send(data, length, options);
	} else {
		return 0;
	}
#else
	return spi_stack_slave_send(data, length, options);
#endif
}

uint16_t spi_stack_recv(void *data, const uint16_t length, uint32_t *options) {
#ifdef BRICK_CAN_BE_MASTER
	if(master_mode & MASTER_MODE_MASTER) {
		return spi_stack_master_recv(data, length, options);
	} else if(master_mode & MASTER_MODE_SLAVE) {
		return spi_stack_slave_recv(data, length, options);
	} else {
		return 0;
	}
#else
	return spi_stack_slave_recv(data, length, options);
#endif
}
