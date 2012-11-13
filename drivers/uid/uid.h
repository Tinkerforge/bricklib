/* master-brick
 * Copyright (C) 2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * uid.h: Read UID from unique identifier register
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

#ifndef UID_H
#define UID_H

#include <stdint.h>

#define UID_CHARACTER_SET_LENGTH 60
#define MAX_BASE58_STR_SIZE 8

__attribute__ ((section (".ramfunc")))
uint32_t uid_get_uid32(void);

char uid_get_serial_char_from_num(uint8_t num);
void uid_to_serial_number(uint32_t value, char *str);
#endif
