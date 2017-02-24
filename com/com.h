/* bricklib
 * Copyright (C) 2009-2012 Olaf Lüke <olaf@tinkerforge.com>
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
	COM_CHIBI     = 3,
	COM_RS485     = 4,
	COM_WIFI      = 5,
	COM_ETHERNET  = 6,
	COM_WIFI2     = 7,
} ComType;

typedef enum {
	COM_TYPE_NONE   = 0,
	COM_TYPE_MASTER = 1,
	COM_TYPE_SLAVE  = 2
} ComExtType;

typedef struct {
	uint32_t uid;
	ComType current;
	uint8_t last_stack_address;
	ComType ext[2];
	ComExtType ext_type[2];
} ComInfo;

#define SEND_BLOCKING_TIMEOUT            100  // Set default to 100ms

#define SEND_BLOCKING_TIMEOUT_NONE       1
#define SEND_BLOCKING_TIMEOUT_USB        SEND_BLOCKING_TIMEOUT
#define SEND_BLOCKING_TIMEOUT_SPI_STACK  SEND_BLOCKING_TIMEOUT
#define SEND_BLOCKING_TIMEOUT_RS485      SEND_BLOCKING_TIMEOUT
#define SEND_BLOCKING_TIMEOUT_WIFI       (SEND_BLOCKING_TIMEOUT*10)
#define SEND_BLOCKING_TIMEOUT_ETHERNET   SEND_BLOCKING_TIMEOUT
#define SEND_BLOCKING_TIMEOUT_WIFI2      (SEND_BLOCKING_TIMEOUT*10)

// Not being able to send over chibi is expected (communication partner out
// of range). Thus we don't want to wait too long.
#define SEND_BLOCKING_TIMEOUT_CHIBI      10

typedef struct Com Com;

// define main communication functions
#define INIT(com) com_list[com]->init()
#define SEND(data, length, com, options) com_list[com].send(data, length, options)
#define RECV(data, length, com, options) com_list[com].recv(data, length, options)

typedef bool (*function_init_t)();
typedef uint16_t (*function_send_t)(const void *data, const uint16_t length, uint32_t *options);
typedef uint16_t (*function_recv_t)(void *data, const uint16_t length, uint32_t *options);

struct Com {
	ComType type;

	function_init_t init;
	function_send_t send;
	function_recv_t recv;
};

extern Com com_list[];

#endif
