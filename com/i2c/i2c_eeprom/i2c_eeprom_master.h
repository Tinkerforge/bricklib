/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_eeprom_master.h: i2c eeprom master functionality (for M24C64)
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

#ifndef I2C_EEPROM_MASTER_H
#define I2C_EEPROM_MASTER_H

#include <stdint.h>
#include <stdbool.h>

#include "config.h"


bool i2c_eeprom_master_read_name(Twi *twi, char *name);
bool i2c_eeprom_master_write_name(Twi *twi, const char *name);

bool i2c_eeprom_master_read(Twi *twi,
                            const uint16_t internal_address,
                            char *data,
                            const uint16_t length);

bool i2c_eeprom_master_write(Twi *twi,
                             const uint16_t internal_address,
                             const char *data,
                             const uint16_t length);

void i2c_eeprom_master_init(Twi *twi);
uint64_t i2c_eeprom_master_read_uid(Twi *twi);
bool i2c_eeprom_master_write_uid(Twi *twi, uint64_t uid);

bool i2c_eeprom_master_read_plugin(Twi *twi,
                                   char *plugin,
                                   const uint8_t position);
bool i2c_eeprom_master_write_plugin(Twi *twi,
                                    const char *plugin,
                                    const uint8_t position);




#endif
