/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_eeprom_slave.h: i2c eeprom slave functionality (for M24C64)
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

#ifndef I2C_STACK_SLAVE_H
#define I2C_STACK_SLAVE_H

#include <stdint.h>

#define I2C_EEPROM_SLAVE_ADDRESS 85

void i2c_eeprom_slave_init(void);
char i2c_eeprom_slave_get_memory(uint16_t address);
void i2c_eeprom_slave_set_memory(uint16_t address, char data);
#endif
