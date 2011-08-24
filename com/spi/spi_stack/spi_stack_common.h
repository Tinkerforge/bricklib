/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_common.h: SPI stack functions common to master and slave
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

#ifndef SPI_GENERAL_H
#define SPI_GENERAL_H

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


#define SPI_STACK_BUFFER_SIZE 64

// SPI delays in ns, 0 = half a clock
#define SPI_DELAY_BETWEEN_CHIP_SELECT 0
#define SPI_DELAY_BETWEEN_TRANSFER 0
#define SPI_DELAY_BEFORE_FIRST_SPI_CLOCK 0

// Speed of SPI clock
#define SPI_CLOCK 8000000 // 8 Mhz

uint16_t spi_stack_send(const void *data, const uint16_t length);
uint16_t spi_stack_recv(void *data, const uint16_t length);

#endif
