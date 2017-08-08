/* master-brick
 * Copyright (C) 2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * uid.c: Read UID from unique identifier register
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

#include "uid.h"

#include <stddef.h>
#include <stdio.h>
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/efc/efc.h"

const char BASE58_STR[] = \
	"123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

__attribute__ ((noinline)) __attribute__ ((section (".ramfunc")))
uint32_t uid_get_uid32(void) {
	// disable sequential-code-optimization, because the Atmel example code does
	// it and reading the unique identifier might trigger a hardfault otherwise
	EFC->EEFC_FMR |= (0x1u << 16);

    // Write "Start Read" unique identifier command (STUI)
	EFC->EEFC_FCR = (0x5A << 24) | EFC_FCMD_STUI;
	 // If STUI is performed, FRDY is cleared
	while ((EFC->EEFC_FSR & EEFC_FSR_FRDY) == EEFC_FSR_FRDY);

    // The Unique Identifier has 128 bits.
	// We only use the second last 32 bits.
    const uint32_t value1 = *(((uint32_t *)(IFLASH_ADDR + 8)));
    const uint32_t value2 = *(((uint32_t *)(IFLASH_ADDR + 12)));

    // Write "Stop Read" unique identifier command (SPUI)
    EFC->EEFC_FCR = (0x5A << 24) | EFC_FCMD_SPUI;
    // If SPUI is performed, FRDY is set
    while ((EFC->EEFC_FSR & EEFC_FSR_FRDY) != EEFC_FSR_FRDY);

	// reenable sequential-code-optimization
	EFC->EEFC_FMR &= ~(0x1u << 16);

    uint32_t uid = 0;

    uid  = (value1 & 0x00000FFF);
    uid |= (value1 & 0x0F000000) >> 12;
    uid |= (value2 & 0x0000003F) << 16;
    uid |= (value2 & 0x000F0000) << 6;
    uid |= (value2 & 0x3F000000) << 2;

    return uid;
}

char uid_get_serial_char_from_num(uint8_t num) {
	if(num < 25) {
		return 'a' + num;
	} else if(num < 50) {
		return 'A' + (num-25);
	} else if(num < 60) {
		return '0' + (num-50);
	}

	return '?';
}

void uid_to_serial_number(uint32_t value, char *str) {
	char reverse_str[MAX_BASE58_STR_SIZE] = {'\0'};
	uint8_t i = 0;
	while(value >= 58) {
		uint32_t mod = value % 58;
		reverse_str[i] = BASE58_STR[mod];
		value = value/58;
		i++;
	}

	reverse_str[i] = BASE58_STR[value];

	uint8_t j = 0;
	for(j = 0; j <= i; j++) {
		str[j] = reverse_str[i-j];
	}
	for(; j < MAX_BASE58_STR_SIZE; j++) {
		str[j] = '\0';
	}
}
