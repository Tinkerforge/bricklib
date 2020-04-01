/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_eeprom_master.c: i2c eeprom master functionality (for M24C64)
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

#include "i2c_eeprom_master.h"

#include "bricklib/drivers/cmsis/core_cm3.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/twi/twi.h"
#include "bricklib/drivers/twi/twid.h"
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "bricklib/drivers/uid/uid.h"
#include "bricklib/utility/mutex.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_common.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/bricklet/bricklet_communication.h"
#include "bricklib/bricklet/bricklet_config.h"

#include "config.h"

uint8_t bricklet_eeprom_address = 84;

extern Mutex mutex_twi_bricklet;
Twid twid0 = {TWI0, NULL};
Twid twid1 = {TWI1, NULL};

void TWI0_IrqHandler(void) {
    TWID_Handler(&twid0);
}

void TWI1_IrqHandler(void) {
    TWID_Handler(&twid1);
}

void i2c_eeprom_master_init(Twi *twi) {
	const Pin twi_pins[] = {PINS_TWI_BRICKLET};

	// Configure TWI pins
	PIO_Configure(twi_pins, PIO_LISTSIZE(twi_pins));

	// Enable TWI peripheral clock
	PMC->PMC_PCER0 = 1 << ID_TWI0;

	// Configure TWI interrupts
	NVIC_DisableIRQ(TWI0_IRQn);
	NVIC_ClearPendingIRQ(TWI0_IRQn);
	NVIC_SetPriority(TWI0_IRQn, PRIORITY_EEPROM_MASTER_TWI0);
	NVIC_EnableIRQ(TWI0_IRQn);

    // Configure TWI as master
    TWI_ConfigureMaster(twi, I2C_EEPROM_CLOCK, BOARD_MCK);
}

// i2c_eeprom_master_read/write are based on twid.c from atmels at91lib and
// adapted for better handling when there is no eeprom present.
// This handling is needed for the bricklet initialization
bool i2c_eeprom_master_read(Twi *twi,
                            const uint16_t internal_address,
                            char *data,
                            const uint16_t length) {
	uint32_t timeout;

	mutex_take(mutex_twi_bricklet, MUTEX_BLOCKING);

	// Start read
	TWI_StartRead(twi,
	              bricklet_eeprom_address,
	              internal_address,
	              I2C_EEPROM_INTERNAL_ADDRESS_BYTES);

	for(uint16_t i = 0; i < length; i++) {
		// If last Byte -> send STOP
		if(i == length-1) {
			TWI_Stop(twi);
		}

		uint32_t timeout = 0;
		// Wait until byte is received, otherwise return false
		while(!TWI_ByteReceived(twi) && (++timeout < I2C_EEPROM_TIMEOUT));

		if(timeout == I2C_EEPROM_TIMEOUT) {
			logieew("read timeout (nothing received)\n\r");
			mutex_give(mutex_twi_bricklet);
			return false;
		}

		data[i] = TWI_ReadByte(twi);
	}

	timeout = 0;
	// Wait for transfer to be complete
	while(!TWI_TransferComplete(twi) && (++timeout < I2C_EEPROM_TIMEOUT));
	if (timeout == I2C_EEPROM_TIMEOUT) {
		logieew("read timeout (transfer incomplete)\n\r");
		mutex_give(mutex_twi_bricklet);
		return false;
	}

	mutex_give(mutex_twi_bricklet);
    return true;
}

bool i2c_eeprom_master_write(Twi *twi,
                             const uint16_t internal_address,
                             const char *data,
                             const uint16_t length) {
	uint32_t timeout;

	mutex_take(mutex_twi_bricklet, MUTEX_BLOCKING);

    // Start write
	TWI_StartWrite(twi,
	               bricklet_eeprom_address,
	               internal_address,
	               I2C_EEPROM_INTERNAL_ADDRESS_BYTES,
	               data[0]);

    for(uint16_t i = 1; i < length; i++) {
    	timeout = 0;
    	// Wait until byte is sent, otherwise return false
		while(!TWI_ByteSent(twi) && (++timeout < I2C_EEPROM_TIMEOUT)) {}

		if(timeout == I2C_EEPROM_TIMEOUT) {
			logieew("write timeout (nothing sent)\n\r");
			mutex_give(mutex_twi_bricklet);
			return false;
		}
        TWI_WriteByte(twi, data[i]);
    }

    // Send STOP
    TWI_SendSTOPCondition(twi);

    timeout = 0;
	// Wait for transfer to be complete
	while(!TWI_TransferComplete(twi) && (++timeout < I2C_EEPROM_TIMEOUT)) {}

	if (timeout == I2C_EEPROM_TIMEOUT) {
		logieew("write timeout (transfer incomplete)\n\r");
		mutex_give(mutex_twi_bricklet);
		return false;
	}

	// Wait at least 5ms between writes (see m24128-bw.pdf)
	SLEEP_MS(5);

	mutex_give(mutex_twi_bricklet);
	return true;
}

uint32_t i2c_eeprom_master_read_uid(Twi *twi) {
	uint32_t uid;
	if(i2c_eeprom_master_read(twi,
	                          I2C_EEPROM_INTERNAL_ADDRESS_UID,
	                          (char*)&uid,
	                          I2C_EEPROM_UID_LENGTH)) {
		logieei("read uid %lu\n\r", uid);
		return uid;
	}

	logieew("could not read uid\n\r");
	return 0;
}

bool i2c_eeprom_master_write_uid(Twi *twi, const uint32_t uid) {
	if(i2c_eeprom_master_write(twi,
	                           I2C_EEPROM_INTERNAL_ADDRESS_UID,
	                           (char*)&uid,
	                           I2C_EEPROM_UID_LENGTH)) {
		logieei("wrote uid %lu\n\r", (uint32_t)uid);
	}

	logieew("could not write uid\n\r");
	return false;
}

uint32_t i2c_eeprom_master_read_magic_number(Twi *twi) {
	uint32_t magic_number;
	if(i2c_eeprom_master_read(twi,
	                          I2C_EEPROM_INTERNAL_ADDRESS_MAGIC_NUMBER,
	                          (char*)&magic_number,
	                          I2C_EEPROM_MAGIC_NUMBER_LENGTH)) {
		logieei("read magic number %lu\n\r", magic_number);
		return magic_number;
	}

	logieew("could not read magic number\n\r");
	return 0;
}

bool i2c_eeprom_master_write_magic_number(Twi *twi) {
	uint32_t magic_number = BRICKLET_MAGIC_NUMBER;
	if(i2c_eeprom_master_write(twi,
	                           I2C_EEPROM_INTERNAL_ADDRESS_MAGIC_NUMBER,
	                           (char*)&magic_number,
	                           I2C_EEPROM_MAGIC_NUMBER_LENGTH)) {
		logieei("wrote magic number %lu\n\r", (uint32_t)magic_number);
	}

	logieew("could not write magic number\n\r");
	return false;
}

bool i2c_eeprom_master_read_plugin(Twi *twi,
                                   char *plugin,
                                   const uint8_t position,
                                   const uint16_t chunk_size) {
	uint16_t add = chunk_size*position;

	if(i2c_eeprom_master_read(twi,
	                          I2C_EEPROM_INTERNAL_ADDRESS_PLUGIN + add,
	                          plugin,
	                          chunk_size)) {
		logieei("read plugin [%d %d %d %d %d %d...]\n\r", plugin[0],
		                                                  plugin[1],
		                                                  plugin[2],
		                                                  plugin[3],
		                                                  plugin[4],
		                                                  plugin[5]);
		return true;
	}

	logieew("could not read plugin\n\r");
	return false;
}

bool i2c_eeprom_master_write_plugin(Twi *twi,
                                   const char *plugin,
                                   const uint8_t position) {
	const uint16_t add = PLUGIN_CHUNK_SIZE*position;

	if(!i2c_eeprom_master_write(twi,
	                            I2C_EEPROM_INTERNAL_ADDRESS_PLUGIN + add,
	                            plugin,
	                            PLUGIN_CHUNK_SIZE)) {
		logieew("could not write plugin\n\r");
		return false;
	}

	logieei("wrote plugin [%d %d %d %d %d %d...]\n\r", plugin[0],
	                                                   plugin[1],
	                                                   plugin[2],
	                                                   plugin[3],
	                                                   plugin[4],
	                                                   plugin[5]);

    return true;
}
