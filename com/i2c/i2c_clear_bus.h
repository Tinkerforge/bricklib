/* bricklib
 * Copyright (C) 2011 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_clear_bus.h: Clear Bus implementation (3.16 I2C specification)
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

#ifndef I2C_CLEAR_BUS_H
#define I2C_CLEAR_BUS_H

#include "bricklib/drivers/pio/pio.h"

void i2c_clear_bus_delay(void);
void i2c_clear_bus_9clock(Pin *sda, Pin *scl);
void i2c_clear_bus_stop(Pin *sda, Pin *scl);
void i2c_clear_bus_start(Pin *sda, Pin *scl);
void i2c_clear_bus(Pin *sda, Pin *scl);

#endif
