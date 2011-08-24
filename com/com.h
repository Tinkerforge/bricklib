/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * com.h: Definition of communication interfaces
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

#ifndef COM_H
#define COM_H

#include <stdint.h>
#include <stdbool.h>

// Communication definitions
typedef enum {
	COM_NONE      = 0,
	COM_USB       = 1,
	COM_SPI_STACK = 2,
	COM_CHIBI     = 3
} ComType;

#define SEND_BLOCKING_TRIALS 1000000

typedef struct Com Com;

// define main communication functions
#define INIT(com) com_list[com]->init()
#define SEND(data, length, com)	com_list[com].send(data, length)
#define RECV(data, length, com) com_list[com].recv(data, length)

typedef bool (*function_init_t)();
typedef uint16_t (*function_send_t)(const void *data, const uint16_t length);
typedef uint16_t (*function_recv_t)(void *data, const uint16_t length);

struct Com {
	ComType type;

	function_init_t init;
	function_send_t send;
	function_recv_t recv;
};

extern Com com_list[];

#endif
