/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_eeprom_common.h: functions common to i2c eeprom master and slave
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

#ifndef I2C_EEPROM_COMMON_H
#define I2C_EEPROM_COMMON_H

#include "config.h"
#include "bricklib/logging/logging.h"

#define I2C_EEPROM_CLOCK 400000 // 100000 and 400000 allowed

#define I2C_EEPROM_INTERNAL_ADDRESS_UID 0
#define I2C_EEPROM_INTERNAL_ADDRESS_NAME 32
#define I2C_EEPROM_INTERNAL_ADDRESS_PLUGIN 96

#define I2C_EEPROM_INTERNAL_ADDRESS_BYTES 2

#define I2C_EEPROM_UID_LENGTH sizeof(uint64_t)
#define I2C_EEPROM_NAME_LENGTH MAX_LENGTH_NAME

#define I2C_EEPROM_MEMORY_SIZE 512
#define I2C_EEPROM_PAGE_SIZE 32

#define I2C_EEPROM_TIMEOUT 50000

// logging for i2c eeprom
#if(DEBUG_I2C_EEPROM)
#define logieed(str, ...) do{logd("iee: " str, ##__VA_ARGS__);}while(0)
#define logieei(str, ...) do{logi("iee: " str, ##__VA_ARGS__);}while(0)
#define logieew(str, ...) do{logw("iee: " str, ##__VA_ARGS__);}while(0)
#define logieee(str, ...) do{loge("iee: " str, ##__VA_ARGS__);}while(0)
#define logieef(str, ...) do{logf("iee: " str, ##__VA_ARGS__);}while(0)
#else
#define logieed(str, ...) {}
#define logieei(str, ...) {}
#define logieew(str, ...) {}
#define logieee(str, ...) {}
#define logieef(str, ...) {}
#endif
#endif
