/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_common_dma.h: SPI stack DMA functions common to master and slave
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

#ifndef SPI_STACK_COMMON_H
#define SPI_STACK_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "bricklib/logging/logging.h"
#include "config.h"

#if(DEBUG_SPI_STACK)
#define logspisd(str, ...) do{logd("spis: " str, ##__VA_ARGS__);}while(0)
#define logspisi(str, ...) do{logi("spis: " str, ##__VA_ARGS__);}while(0)
#define logspisw(str, ...) do{logw("spis: " str, ##__VA_ARGS__);}while(0)
#define logspise(str, ...) do{loge("spis: " str, ##__VA_ARGS__);}while(0)
#define logspisf(str, ...) do{logf("spis: " str, ##__VA_ARGS__);}while(0)
#else
#define logspisd(str, ...) {}
#define logspisi(str, ...) {}
#define logspisw(str, ...) {}
#define logspise(str, ...) {}
#define logspisf(str, ...) {}
#endif


#define SPI_STACK_BUFFER_SIZE 80

#define SPI_ADDRESS_MIN 1
#define SPI_ADDRESS_MAX 8

// SPI delays in ns, 0 = half a clock
#define SPI_DELAY_BETWEEN_CHIP_SELECT 0
#define SPI_DELAY_BETWEEN_TRANSFER 0
#define SPI_DELAY_BEFORE_FIRST_SPI_CLOCK 0


// Speed of SPI clock
#define SPI_CLOCK 9000000 // 9 Mhz

#define SPI_STACK_EMPTY_MESSAGE_LENGTH 4
#define SPI_STACK_MAX_MESSAGE_LENGTH   84

#define SPI_STACK_MESSAGE_LENGTH_MIN   12
#define SPI_STACK_PREAMBLE_VALUE       0xAA

#define SPI_STACK_PREAMBLE                  0
#define SPI_STACK_LENGTH                    1
#define SPI_STACK_INFO(length)              ((length) -2)
#define SPI_STACK_CHECKSUM(length)          ((length) -1)

#define SPI_STACK_INFO_BUSY                 (1 << 6)
#define SPI_STACK_INFO_SEQUENCE_MASTER_MASK (0x7)
#define SPI_STACK_INFO_SEQUENCE_SLAVE_MASK  (0x38)

void spi_stack_slave_irq(void);

void spi_stack_increase_slave_seq(uint8_t *seq);
void spi_stack_increase_master_seq(uint8_t *seq);

uint8_t spi_stack_calculate_pearson(const uint8_t *data, const uint8_t length);
uint16_t spi_stack_send(const void *data, const uint16_t length, uint32_t *options);
uint16_t spi_stack_recv(void *data, const uint16_t length, uint32_t *options);

#endif
