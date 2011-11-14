/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * com.c: Definition of communication interfaces
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

#include "com.h"

#include "bricklib/com/none/none.h"
#include "bricklib/com/usb/usb.h"
#include "bricklib/com/spi/spi_stack/spi_stack_common.h"
#include "config.h"

#ifndef COM_EXTENSIONS
#define COM_EXTENSIONS
#endif

uint8_t com_last_stack_address = 0;
uint8_t com_last_spi_stack_id = 1;
uint8_t com_last_ext_id[2] = {1, 1};
ComType com_current = COM_NONE;
ComType com_ext[2] = {COM_NONE, COM_NONE};
uint8_t com_stack_id = 1;
uint64_t com_brick_uid = 62;

Com com_list[] = {
	{COM_NONE, no_init, no_send, no_recv},
	{COM_USB, usb_init, usb_send, usb_recv},
	{COM_SPI_STACK, NULL, spi_stack_send, spi_stack_recv},
	COM_EXTENSIONS
};
