/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * crc.h: Implementation of DMA CRC calculation
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

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define CRCCU_TIMEOUT    0xFFFFFFFF

typedef struct {
    uint32_t TR_ADDR;
    uint32_t TR_CTRL;
} CCRCDescriptor;

uint16_t crc16_compute(uint8_t *buffer, const uint16_t length);
uint32_t crc32_compute(uint8_t *buffer, const uint16_t length);
uint32_t crc_compute(uint8_t *buffer, const uint16_t length, const uint32_t polynom_type);

#endif
