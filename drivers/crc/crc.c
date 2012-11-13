/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * crc.c: Implementation of DMA CRC calculation
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

#include "crc.h"

#include <string.h>
#include "config.h"

inline uint16_t crc16_compute(uint8_t *buffer, const uint16_t length) {
	return crc_compute(buffer,
	                   length,
	                   CRCCU_MR_PTYPE_CCIT16) &  0xFFFF;
}

inline uint32_t crc32_compute(uint8_t *buffer, const uint16_t length) {
	return crc_compute(buffer,
	                   length,
	                   CRCCU_MR_PTYPE_CCIT8023);
}

uint32_t crc_compute(uint8_t *buffer, const uint16_t length, const uint32_t polynom_type) {
	CCRCDescriptor __attribute__ ((aligned(512))) crcd;
	// Reset CRC value
	CRCCU->CRCCU_CR = CRCCU_CR_RESET;
    memset(&crcd, 0, sizeof(CCRCDescriptor));

    crcd.TR_ADDR = (uint32_t)buffer;
    //           transfer width | buffer length
    crcd.TR_CTRL = (0 << 24) | length;

    // Configure CRCCU
    CRCCU->CRCCU_DSCR = (uint32_t)&crcd;
    CRCCU->CRCCU_MR = CRCCU_MR_ENABLE | polynom_type;

    // Compute CRC
    uint32_t timeout = 0;

    CRCCU->CRCCU_DMA_EN = CRCCU_DMA_EN_DMAEN;
    while(((CRCCU->CRCCU_DMA_SR & CRCCU_DMA_SR_DMASR) == CRCCU_DMA_SR_DMASR)
          && (timeout++ < CRCCU_TIMEOUT)) {
    	// TODO: yield?
    }

    return CRCCU->CRCCU_SR;
}
