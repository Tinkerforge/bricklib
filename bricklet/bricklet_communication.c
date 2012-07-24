/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
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

#include <twi/twi.h>
#include <cmsis/core_cm3.h>

#include "bricklib/bricklet/bricklet_config.h"
#include "bricklib/bricklet/bricklet_init.h"
#include "bricklib/com/com_common.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"

#include "config.h"

extern const BrickletAddress baddr[BRICKLET_NUM];

void write_bricklet_name(uint8_t com, const WriteBrickletName *data) {
	uint8_t port = tolower(data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (write name)\n\r", port);
		return;
	}

	bricklet_select(port);
	i2c_eeprom_master_write_name(TWI_BRICKLET, data->name);
	bricklet_deselect(port);

	logbletd("Write Bricklet Name [%s] (port %c)\n\r", data->name, data->port);
}

void read_bricklet_name(uint8_t com, const ReadBrickletName *data) {
	uint8_t port = tolower(data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (read name)\n\r", port);
		return;
	}

	ReadBrickletNameReturn rbnr;
	rbnr.stack_id = data->stack_id;
	rbnr.type = TYPE_READ_BRICKLET_NAME;
	rbnr.length = sizeof(ReadBrickletNameReturn);

	bricklet_select(port);
	i2c_eeprom_master_read_name(TWI_BRICKLET, rbnr.name);
	bricklet_deselect(port);

	send_blocking_with_timeout(&rbnr, sizeof(ReadBrickletNameReturn), com);
	logbletd("Read Bricklet Name [%s] (port %c)\n\r", rbnr.name, data->port);
}

void write_bricklet_plugin(uint8_t com, const WriteBrickletPlugin *data) {
	uint8_t port = tolower(data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (write plugin)\n\r", port);
		return;
	}

	bricklet_select(port);
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
}

void read_bricklet_plugin(uint8_t com, const ReadBrickletPlugin *data) {
	uint8_t port = tolower(data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (read plugin)\n\r", port);
		return;
	}

	ReadBrickletPluginReturn rbpr;
	rbpr.stack_id = data->stack_id;
	rbpr.type = TYPE_READ_BRICKLET_PLUGIN;
	rbpr.length = sizeof(ReadBrickletPluginReturn);

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

void write_bricklet_uid(uint8_t com, const WriteBrickletUID *data) {
	uint8_t port = tolower(data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (write uid)\n\r", port);
		return;
	}

	bricklet_select(port);
	i2c_eeprom_master_write_uid(TWI_BRICKLET, data->uid);
	bricklet_deselect(port);
	logbletd("Write Bricklet UID %d\n\r", (uint32_t)data->uid);
}

void read_bricklet_uid(uint8_t com, const ReadBrickletUID *data) {
	uint8_t port = tolower(data->port) - 'a';
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (read uid)\n\r", port);
		return;
	}

	ReadBrickletUIDReturn rbuidr;

	rbuidr.stack_id = data->stack_id;
	rbuidr.type = TYPE_READ_BRICKLET_UID;
	rbuidr.length = sizeof(ReadBrickletUIDReturn);
	bricklet_select(port);
	rbuidr.uid = i2c_eeprom_master_read_uid(TWI_BRICKLET);
	bricklet_deselect(port);

	send_blocking_with_timeout(&rbuidr, sizeof(ReadBrickletUIDReturn), com);
	logbletd("Read Bricklet UID %d\n\r", (uint32_t)rbuidr.uid);
}
