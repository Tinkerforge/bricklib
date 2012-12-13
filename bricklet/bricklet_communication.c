/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_communication.c: implementation of bricklet specific messages
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

#include "bricklet_communication.h"

#include "bricklib/drivers/twi/twi.h"
#include "bricklib/drivers/cmsis/core_cm3.h"

#include "bricklib/bricklet/bricklet_config.h"
#include "bricklib/bricklet/bricklet_init.h"
#include "bricklib/com/com_common.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"

#include "config.h"

void write_bricklet_plugin(const ComType com, const WriteBrickletPlugin *data) {
	uint8_t port = tolower((uint8_t)data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		com_return_error(data, sizeof(MessageHeader), MESSAGE_ERROR_CODE_INVALID_PARAMETER, com);
		logblete("Bricklet Port %d does not exist (write plugin)\n\r", port);
		return;
	}

	bricklet_select(port);
/*	if(data->position == 0) {
		i2c_eeprom_master_write_magic_number(TWI_BRICKLET);
	}*/

	i2c_eeprom_master_write_plugin(TWI_BRICKLET,
	                               data->plugin,
	                               data->position);
	bricklet_deselect(port);
	logbletd("Write Bricklet Plugin [%d %d %d %d %d %d] (port %c, pos %d)\n\r",
	         data->plugin[0],
	         data->plugin[1],
	         data->plugin[2],
	         data->plugin[3],
	         data->plugin[4],
	         data->plugin[5],
	         data->port,
	         data->position);

	com_return_setter(com, data);
}

void read_bricklet_plugin(const ComType com, const ReadBrickletPlugin *data) {
	uint8_t port = tolower((uint8_t)data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		com_return_error(data, sizeof(ReadBrickletPluginReturn), MESSAGE_ERROR_CODE_INVALID_PARAMETER, com);
		logblete("Bricklet Port %d does not exist (read plugin)\n\r", port);
		return;
	}

	ReadBrickletPluginReturn rbpr;
	rbpr.header = data->header;
	rbpr.header.length = sizeof(ReadBrickletPluginReturn);

	bricklet_select(port);
	i2c_eeprom_master_read_plugin(TWI_BRICKLET,
	                              rbpr.plugin,
	                              data->position,
	                              PLUGIN_CHUNK_SIZE);
	bricklet_deselect(port);

	send_blocking_with_timeout(&rbpr, sizeof(ReadBrickletPluginReturn), com);
	logbletd("Read Bricklet Plugin [%d %d %d %d %d %d] (port %c, pos %d)\n\r",
	         rbpr.plugin[0],
	         rbpr.plugin[1],
	         rbpr.plugin[2],
	         rbpr.plugin[3],
	         rbpr.plugin[4],
	         rbpr.plugin[5],
	         data->port,
	         data->position);
}

void write_bricklet_uid(const ComType com, const WriteBrickletUID *data) {
	uint8_t port = tolower((uint8_t)data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		com_return_error(data, sizeof(MessageHeader), MESSAGE_ERROR_CODE_INVALID_PARAMETER, com);
		logblete("Bricklet Port %d does not exist (write uid)\n\r", port);
		return;
	}

	bricklet_select(port);
	i2c_eeprom_master_write_uid(TWI_BRICKLET, data->uid);
	bricklet_deselect(port);
	logbletd("Write Bricklet UID %lu\n\r", data->uid);

	com_return_setter(com, data);
}

void read_bricklet_uid(const ComType com, const ReadBrickletUID *data) {
	uint8_t port = tolower((uint8_t)data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		com_return_error(data, sizeof(ReadBrickletUIDReturn), MESSAGE_ERROR_CODE_INVALID_PARAMETER, com);
		logblete("Bricklet Port %d does not exist (read uid)\n\r", port);
		return;
	}

	ReadBrickletUIDReturn rbuidr;
	rbuidr.header = data->header;
	rbuidr.header.length = sizeof(ReadBrickletUIDReturn);

	bricklet_select(port);
	rbuidr.uid = i2c_eeprom_master_read_uid(TWI_BRICKLET);
	bricklet_deselect(port);

	send_blocking_with_timeout(&rbuidr, sizeof(ReadBrickletUIDReturn), com);
	logbletd("Read Bricklet UID %lu\n\r", rbuidr.uid);
}
