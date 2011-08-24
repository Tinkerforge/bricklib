/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_pca9549.c: Functionality to read and write from pca9549 bus switch
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

#include "i2c_pca9549.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <twi/twi.h>

bool i2c_pca9549_write(uint8_t address, uint8_t data) {
	//printf("write %d %d\n\r", address, data);
	// Set address
	// No internal address for bus switch PCA9549
    TWI_BRICKLET->TWI_MMR = 0;
    TWI_BRICKLET->TWI_MMR = (address << 16);

    // Send data
    TWI_WriteByte(TWI_BRICKLET, data);
    uint16_t timeout = 0;
    while(!TWI_ByteSent(TWI_BRICKLET) &&
          (++timeout < BUS_SWITCH_MAX_TIMEOUT));
    if(timeout == BUS_SWITCH_MAX_TIMEOUT) {
    	//printf("SPI stack write timeout bs\n\r");
    	return false;
    }

    // Send stop
    TWI_SendSTOPCondition(TWI_BRICKLET);
    timeout = 0;
    while(!TWI_TransferComplete(TWI_BRICKLET) &&
          (++timeout < BUS_SWITCH_MAX_TIMEOUT));
    if(timeout == BUS_SWITCH_MAX_TIMEOUT) {
    	//printf("SPI stack write timeout tc\n\r");
    	return false;
    }

    return true;
}

bool i2c_pca9549_read(uint8_t address, uint8_t *data) {
	// Set address
	// No internal address for bus switch PCA9549
	TWI_BRICKLET->TWI_MMR = 0;
	TWI_BRICKLET->TWI_MMR = TWI_MMR_MREAD | (address << 16);

	TWI_BRICKLET->TWI_CR = TWI_CR_START;

	// Set stop bit (we only read one byte)
	TWI_Stop(TWI_BRICKLET);
	uint16_t timeout = 0;

	// Receive one byte
    while(!TWI_ByteReceived(TWI_BRICKLET) &&
          (++timeout < BUS_SWITCH_MAX_TIMEOUT));
    if(timeout == BUS_SWITCH_MAX_TIMEOUT) {
    	//printf("SPI stack read timeout br\n\r");
    	return false;
    }
	*data = TWI_ReadByte(TWI_BRICKLET);

	// Write stop bit
    while(!TWI_TransferComplete(TWI_BRICKLET) &&
          (++timeout < BUS_SWITCH_MAX_TIMEOUT));
    if(timeout == BUS_SWITCH_MAX_TIMEOUT) {
    	//printf("SPI stack read timeout tc\n\r");
    	return false;
    }

	return true;

}
