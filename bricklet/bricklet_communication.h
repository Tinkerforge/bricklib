/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_communication.h: implementation of bricklet specific messages
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

#ifndef BRICKLET_COMMUNICATION_H
#define BRICKLET_COMMUNICATION_H

#include <stdint.h>
#include "bricklib/com/com_messages.h"

#define PLUGIN_CHUNK_SIZE 32

#define TYPE_WRITE_BRICKLET_NAME 244
#define TYPE_READ_BRICKLET_NAME 245
#define TYPE_WRITE_BRICKLET_PLUGIN 246
#define TYPE_READ_BRICKLET_PLUGIN 247
#define TYPE_WRITE_BRICKLET_UID 248
#define TYPE_READ_BRICKLET_UID 249

#define COM_MESSAGES_BRICKLET \
	{TYPE_WRITE_BRICKLET_NAME, (message_handler_func_t)write_bricklet_name}, \
	{TYPE_READ_BRICKLET_NAME, (message_handler_func_t)read_bricklet_name}, \
	{TYPE_WRITE_BRICKLET_PLUGIN, (message_handler_func_t)write_bricklet_plugin}, \
	{TYPE_READ_BRICKLET_PLUGIN, (message_handler_func_t)read_bricklet_plugin}, \
	{TYPE_WRITE_BRICKLET_UID, (message_handler_func_t)write_bricklet_uid}, \
	{TYPE_READ_BRICKLET_UID, (message_handler_func_t)read_bricklet_uid},

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char port;
	char name[MAX_LENGTH_NAME];
} __attribute__((__packed__)) WriteBrickletName;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char port;
} __attribute__((__packed__)) ReadBrickletName;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char name[MAX_LENGTH_NAME];
} __attribute__((__packed__)) ReadBrickletNameReturn;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char port;
	uint8_t position;
	char plugin[PLUGIN_CHUNK_SIZE];
} __attribute__((__packed__)) WriteBrickletPlugin;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char port;
	uint8_t position;
} __attribute__((__packed__)) ReadBrickletPlugin;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char plugin[PLUGIN_CHUNK_SIZE];
} __attribute__((__packed__)) ReadBrickletPluginReturn;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char port;
	uint64_t uid;
} __attribute__((__packed__)) WriteBrickletUID;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	char port;
} __attribute__((__packed__)) ReadBrickletUID;

typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint16_t length;
	uint64_t uid;
} __attribute__((__packed__)) ReadBrickletUIDReturn;

void write_bricklet_name(uint8_t com, const WriteBrickletName *data);
void read_bricklet_name(uint8_t com, const ReadBrickletName *data);
void write_bricklet_plugin(uint8_t com, const WriteBrickletPlugin *data);
void read_bricklet_plugin(uint8_t com, const ReadBrickletPlugin *data);
void write_bricklet_uid(uint8_t com, const WriteBrickletUID *data);
void read_bricklet_uid(uint8_t com, const ReadBrickletUID *data);

#endif
