/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_select_dma.c: Functions to select participant in stack (with DMA)
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

#include "spi_stack_select.h"
#include "spi_stack_common.h"

#include "bricklib/drivers/pio/pio.h"

#include <stdint.h>

static uint8_t spi_stack_select_last_num = 0;

Pin spi_select_master[SPI_ADDRESS_MAX] = {PINS_SPI_SELECT_MASTER};

extern int32_t spi_stack_deselect_time;

void spi_stack_select(const uint8_t num) {
	if(num >= SPI_ADDRESS_MIN && num <= SPI_ADDRESS_MAX) {
		PIO_Clear(&spi_select_master[num-1]);
		spi_stack_select_last_num = num;
	}
}

void spi_stack_deselect() {
	PIO_Set(&spi_select_master[spi_stack_select_last_num-1]);
	spi_stack_select_last_num = 0;

	spi_stack_deselect_time = SysTick->VAL;
}
