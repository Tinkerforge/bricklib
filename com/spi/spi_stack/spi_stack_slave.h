/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_slave.h: SPI stack slave functionality
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

#ifndef SPI_SLAVE_H
#define SPI_SLAVE_H

#include <stdint.h>

#include "bricklib/com/com_messages.h"

void SPI_IrqHandler(void);

bool spi_stack_slave_is_selected(void);
void spi_stack_slave_init(void);
void spi_stack_slave_message_loop(void *parameters);
void spi_stack_slave_message_loop_return(char *data, const uint16_t length);

#endif
