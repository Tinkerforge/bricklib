/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
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
#include "bricklib/com/com.h"

#define PLUGIN_CHUNK_SIZE 32
#define PLUGIN_CHUNK_SIZE_STARTUP (128*4)

#define FID_WRITE_BRICKLET_NAME 244
#define FID_READ_BRICKLET_NAME 245
#define FID_WRITE_BRICKLET_PLUGIN 246
#define FID_READ_BRICKLET_PLUGIN 247
#define FID_WRITE_BRICKLET_UID 248
#define FID_READ_BRICKLET_UID 249

#define COM_MESSAGES_BRICKLET \
	{FID_WRITE_BRICKLET_NAME, (message_handler_func_t)NULL}, /* Not used anymore */ \
	{FID_READ_BRICKLET_NAME, (message_handler_func_t)NULL}, /* Not used anymore */ \
	{FID_WRITE_BRICKLET_PLUGIN, (message_handler_func_t)write_bricklet_plugin}, \
	{FID_READ_BRICKLET_PLUGIN, (message_handler_func_t)read_bricklet_plugin}, \
	{FID_WRITE_BRICKLET_UID, (message_handler_func_t)write_bricklet_uid}, \
	{FID_READ_BRICKLET_UID, (message_handler_func_t)read_bricklet_uid},

typedef struct {
	MessageHeader header;
	char port;
	uint8_t position;
	char plugin[PLUGIN_CHUNK_SIZE];
} __attribute__((__packed__)) WriteBrickletPlugin;

typedef struct {
	MessageHeader header;
	char port;
	uint8_t position;
} __attribute__((__packed__)) ReadBrickletPlugin;

typedef struct {
	MessageHeader header;
	char plugin[PLUGIN_CHUNK_SIZE];
} __attribute__((__packed__)) ReadBrickletPluginReturn;

typedef struct {
	MessageHeader header;
	char port;
	uint32_t uid;
} __attribute__((__packed__)) WriteBrickletUID;

typedef struct {
	MessageHeader header;
	char port;
} __attribute__((__packed__)) ReadBrickletUID;

typedef struct {
	MessageHeader header;
	uint32_t uid;
} __attribute__((__packed__)) ReadBrickletUIDReturn;

void write_bricklet_plugin(const ComType com, const WriteBrickletPlugin *data);
void read_bricklet_plugin(const ComType com, const ReadBrickletPlugin *data);
void write_bricklet_uid(const ComType com, const WriteBrickletUID *data);
void read_bricklet_uid(const ComType com, const ReadBrickletUID *data);

#endif
