/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_master_dma.h: SPI stack DMA master functionality
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

#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <stdint.h>

#include "bricklib/com/com_common.h"

typedef enum {
	SLAVE_STATUS_ABSENT = 0,
	SLAVE_STATUS_AVAILABLE,
} SPIStackMasterSlaveStatus;

typedef enum {
	TRANSCEIVE_STATE_MESSAGE_EMPTY = 0,
	TRANSCEIVE_STATE_MESSAGE_READY,
	TRANSCEIVE_STATE_BUSY
} SPIStackMasterTransceiveState;

typedef enum {
	TRANSCEIVE_INFO_SEND_EMPTY_MESSAGE = 0,
	TRANSCEIVE_INFO_SEND_NOTHING_BUSY,
	TRANSCEIVE_INFO_SEND_OK,
	TRANSCEIVE_INFO_SEND_ERROR
} SPIStackMasterTransceiveInfo;

void spi_stack_master_irq(void);
void spi_stack_master_reset_recv_dma_buffer(void);
void spi_stack_master_reset_send_dma_buffer(void);
void spi_stack_master_enable_dma(void);
void spi_stack_master_disable_dma(void);
SPIStackMasterTransceiveInfo spi_stack_master_start_transceive(const uint8_t *data, const uint8_t length, const uint8_t stack_address);

bool spi_stack_master_transceive(void);

void spi_master_state_machine(void);
void spi_master_reset_state_machine(void);
void spi_stack_master_init(void);
void spi_stack_master_insert_position(void* data, const uint8_t position);
void spi_stack_master_update_routing_table(void* data, const uint8_t position);

void spi_stack_master_state_machine_loop(void *arg);
void spi_stack_master_message_loop(void *parameters);

void spi_stack_master_message_loop_return(const char *data, const uint16_t length);

uint16_t spi_stack_master_send(const void *data, const uint16_t length, uint32_t *options);
uint16_t spi_stack_master_recv(void *data, const uint16_t length, uint32_t *options);

#endif
