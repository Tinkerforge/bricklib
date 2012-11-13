/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_eeprom_slave.c: i2c eeprom slave functionality (for M24C64)
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

#include "i2c_eeprom_slave.h"

#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bricklib/drivers/cmsis/core_cm3.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/twi/twi.h"
#include "bricklib/drivers/twi/twid.h"

#include "bricklib/drivers/uid/uid.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_common.h"
#include "bricklib/com/spi/spi_stack/spi_stack_slave.h"


// EEPROM emulation for M24C64
// It is based on twi-slave example of ATMEL Softpack for sam3s-ek.

typedef struct {
    uint8_t page;
    uint8_t offset;
    uint8_t acquire;
    char memory[sizeof(uint32_t)];
} EEPROMDriver;

static EEPROMDriver eeprom;

void TWI1_IrqHandler(void) {
	unsigned int status = TWI_GetStatus(TWI_STACK);

    if(((status & TWI_SR_SVACC) == TWI_SR_SVACC) && (eeprom.acquire == 0)) {
        TWI_DisableIt(TWI_STACK, TWI_IER_SVACC);
        TWI_EnableIt(TWI_STACK, TWI_IER_RXRDY |
                                TWI_IER_GACC |
                                TWI_IER_NACK |
                                TWI_IER_EOSACC |
                                TWI_IER_SCL_WS);
        eeprom.acquire++;
        eeprom.page = 0;
        eeprom.offset = 0;
    }

    if(((status & TWI_SR_SVACC) == TWI_SR_SVACC) &&
       ((status & TWI_SR_GACC) == 0) &&
       ((status & TWI_SR_RXRDY) == TWI_SR_RXRDY)) {

        if(eeprom.acquire == 1) {
            // Acquire LSB address
        	eeprom.page = (TWI_ReadByte(TWI_STACK) & 0xFF);
        	eeprom.acquire++;
        } else if(eeprom.acquire == 2) {
            // Acquire MSB address
        	eeprom.page |= (TWI_ReadByte(TWI_STACK) & 0xFF) << 8;
        	eeprom.acquire++;
        } else {
            // Read one byte of data from master to slave device
        	uint16_t addr = I2C_EEPROM_PAGE_SIZE*eeprom.page + eeprom.offset;
        	i2c_eeprom_slave_set_memory(addr, TWI_ReadByte(TWI_STACK) & 0xFF);
        	eeprom.offset++;
        }
    } else if(((status & TWI_SR_TXRDY) == TWI_SR_TXRDY) &&
              ((status & TWI_SR_TXCOMP) == TWI_SR_TXCOMP) &&
              ((status & TWI_SR_EOSACC) == TWI_SR_EOSACC)) {

        // End of transfer, end of slave access
    	eeprom.offset = 0;
    	eeprom.acquire = 0;
    	eeprom.page = 0;
        TWI_EnableIt(TWI_STACK, TWI_IER_SVACC);
        TWI_DisableIt(TWI_STACK, TWI_IER_RXRDY |
                                 TWI_IDR_GACC |
                                 TWI_IDR_NACK |
                                 TWI_IER_EOSACC |
                                 TWI_IER_SCL_WS);
    } else if(((status & TWI_SR_SVACC) == TWI_SR_SVACC) &&
              ((status & TWI_SR_GACC) == 0) &&
              (eeprom.acquire == 3) &&
              ((status & TWI_SR_SVREAD) == TWI_SR_SVREAD) &&
              ((status & TWI_SR_NACK) == 0)) {

        // Write one byte of data from slave to master device
    	uint16_t addr = I2C_EEPROM_PAGE_SIZE*eeprom.page + eeprom.offset;
        TWI_WriteByte(TWI_STACK, i2c_eeprom_slave_get_memory(addr));
        eeprom.offset++;
    }
}

char i2c_eeprom_slave_get_memory(uint16_t address) {
	if(address < sizeof(uint32_t)) {
		return eeprom.memory[address];
	}
	return 0;
}

void i2c_eeprom_slave_set_memory(uint16_t address, char data) {
	if(address < sizeof(uint32_t)) {
		eeprom.memory[address] = data;
	}
}

void i2c_eeprom_slave_init(void) {
	uint32_t uid = uid_get_uid32();

	memcpy(eeprom.memory, &uid, sizeof(uint32_t));

	eeprom.offset = 0;
	eeprom.acquire = 0;
	eeprom.page = 0;

	const Pin twi_pins[] = {PINS_TWI_STACK};

	// Configure TWI pins
	PIO_Configure(twi_pins, PIO_LISTSIZE(twi_pins));

	// Enable TWI peripheral clock
	PMC->PMC_PCER0 = 1 << ID_TWI1;

	// Configure TWI interrupts
	NVIC_DisableIRQ(TWI1_IRQn);
	NVIC_ClearPendingIRQ(TWI1_IRQn);
	NVIC_SetPriority(TWI1_IRQn, PRIORITY_EEPROM_SLAVE_TWI1);
	NVIC_EnableIRQ(TWI1_IRQn);

    // Configure TWI as slave, with address I2C_EEPROM_SLAVE_ADDRESS
    TWI_ConfigureSlave(TWI_STACK, I2C_EEPROM_SLAVE_ADDRESS);

    // Enable TWI interrupt
    TWI_EnableIt(TWI_STACK, TWI_IER_SVACC);
}
