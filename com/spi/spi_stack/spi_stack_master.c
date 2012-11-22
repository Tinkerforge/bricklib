/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_master.c: SPI stack master functionality
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

#include "spi_stack_master.h"

#include <stdio.h>
#include <string.h>
#include "bricklib/drivers/spi/spi.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/pio/pio_it.h"
#include "bricklib/drivers/cmsis/core_cm3.h"
#include <stdbool.h>
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"

#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/pearson_hash.h"
#include "bricklib/drivers/crc/crc.h"
#include "bricklib/com/com_common.h"
#include "bricklib/com/com_messages.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"
#include "bricklib/drivers/uid/uid.h"
#include "extensions/rs485/rs485_low_level.h"

#include "spi_stack_select.h"
#include "spi_stack_common.h"
#include "config.h"

#include "routing.h"

#define SPI_DLYBCS(delay, mc) ((uint32_t)((mc/1000000) * delay) << 24)
#define SPI_STACK_MASTER_TIMEOUT 10000

extern uint8_t spi_stack_buffer_recv[SPI_STACK_BUFFER_SIZE];
extern uint8_t spi_stack_buffer_send[SPI_STACK_BUFFER_SIZE];

// Recv and send buffer size for SPI stack (written by spi_stack_send/recv)
extern uint16_t spi_stack_buffer_size_send;
extern uint16_t spi_stack_buffer_size_recv;

extern int8_t spi_stack_send_to;

extern Pin spi_select_master[];
extern ComInfo com_info;

bool spi_stack_master_transceive(void) {
	uint8_t master_checksum = 0;
	uint8_t slave_checksum = 0;
	uint8_t send_length = spi_stack_buffer_size_send;

	volatile uint8_t dummy = 0;

	__disable_irq();
	SPI->SPI_SR;
	SPI->SPI_RDR;

	uint16_t timeout = SPI_STACK_MASTER_TIMEOUT;

	// Sync with slave
	while(dummy != 0xFF && timeout != 0) {
		while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
		SPI->SPI_TDR = 0xFF;
		while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
		dummy = SPI->SPI_RDR;
		timeout--;
	}

	if(timeout == 0) {
		logspise("Timeout, did not receive 0xFF from Slave\n\r");
		__enable_irq();
		return false;
	}

	// Write length
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = send_length;
	PEARSON(master_checksum, send_length);

	// Write first byte
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = spi_stack_buffer_send[0];
	PEARSON(master_checksum, spi_stack_buffer_send[0]);

	// Read length
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    uint8_t slave_length = SPI->SPI_RDR;
    PEARSON(slave_checksum, slave_length);

    // If master and slave length are 0, stop communication
    if(slave_length == 0 && send_length == 0) {
    	// Read dummy
        while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
        dummy = SPI->SPI_RDR;

		__enable_irq();
        return true;
    }

    // Length to transceive is maximum of slave and master length
    uint8_t max_length = MIN(MAX(send_length, slave_length),
                             SPI_STACK_BUFFER_SIZE);

    // Exchange data
    for(uint8_t i = 1; i < max_length; i++) {
    	// Write
    	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
    	SPI->SPI_TDR = spi_stack_buffer_send[i];
    	PEARSON(master_checksum, spi_stack_buffer_send[i]);

    	// Read
        while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
        spi_stack_buffer_recv[i-1] = SPI->SPI_RDR;
        PEARSON(slave_checksum, spi_stack_buffer_recv[i-1]);
    }

    // Write CRC
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = master_checksum;

	// Read last data byte
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    spi_stack_buffer_recv[max_length-1] = SPI->SPI_RDR;
    PEARSON(slave_checksum, spi_stack_buffer_recv[max_length-1]);

	// Read CRC
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    uint8_t crc = SPI->SPI_RDR;

    // Is CRC correct?
    uint8_t master_ack = crc == slave_checksum;

    __NOP();
    __NOP();
    __NOP();

	// Write ACK/NACK
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = master_ack;

	// Read ACK/NACK
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    uint8_t slave_ack = SPI->SPI_RDR;

    // If everything is OK, set sizes accordingly
    if(slave_ack == 1 && master_ack == 1) {
    	spi_stack_buffer_size_recv = slave_length;
    	if(send_length != 0) {
    		spi_stack_buffer_size_send = 0;
    	}
    } else {
    	if(!master_ack) {
    		logspisw("Checksum: Master(%d) != Slave(%d)\n\r", crc,
    		                                                  slave_checksum);
    	}
    	if(!slave_ack) {
    		logspisw("Checksum: Received NACK from Slave\n\r");
    	}
		__enable_irq();
    	return false;
    }

    __enable_irq();
    return true;
}

void spi_stack_master_init(void) {
	Pin spi_master_pins[] = {PINS_SPI};
	PIO_Configure(spi_master_pins, PIO_LISTSIZE(spi_master_pins));
#ifndef CONSOLE_USART_USE_UART1
	if(master_get_hardware_version() > 10) {
		Pin spi_select_7_20 = PIN_SPI_SELECT_MASTER_7_20;
		memcpy(&spi_select_master[7], &spi_select_7_20, sizeof(Pin));
	} else {
		Pin spi_select_7_10 = PIN_SPI_SELECT_MASTER_7_10;
		memcpy(&spi_select_master[7], &spi_select_7_10, sizeof(Pin));
	}

	PIO_Configure(spi_select_master, 8);
#else
	PIO_Configure(spi_select_master, 7);
#endif

    // Master mode configuration
    SPI_Configure(SPI,
                  ID_SPI,
                  // Master mode
                  SPI_MR_MSTR |
                  // Mode fault detection disabled
                  SPI_MR_MODFDIS |
                  // Wait until receive register empty before read
                  SPI_MR_WDRBT |
                  // Chip select number
                  SPI_PCS(0) |
                  // Delay between chip selects
                  SPI_DLYBCS(SPI_DELAY_BETWEEN_CHIP_SELECT, BOARD_MCK));

    // Configure slave select
    SPI_ConfigureNPCS(SPI,
                      // slave select num
                      0,
                      // Delay between consecutive transfers
                      SPI_DLYBCT(SPI_DELAY_BETWEEN_TRANSFER, BOARD_MCK) |
                      // Delay before first SPCK
                      SPI_DLYBS(SPI_DELAY_BEFORE_FIRST_SPI_CLOCK, BOARD_MCK) |
                      // SPI baud rate
                      SPI_SCBR(SPI_CLOCK, BOARD_MCK));

    // Enable SPI peripheral.
    SPI_Enable(SPI);
}

void spi_stack_master_state_machine_loop(void *arg) {
	uint8_t sa_counter = 1;
    while(true) {
		// As long as receive buffer is not empty
    	// Or there are no stack participants, do nothing
    	if(spi_stack_buffer_size_recv == 0 && com_info.current != COM_NONE) {
    		// If nothing to send ask for data round robin
			if(spi_stack_buffer_size_send == 0) {
				spi_stack_select(sa_counter);
				spi_stack_master_transceive();
				spi_stack_deselect();
	    		spi_stack_master_insert_position(spi_stack_buffer_recv, sa_counter);
				if(sa_counter == com_info.last_stack_address) {
					sa_counter = 1;
				} else {
					sa_counter++;
				}
			}

			// If something to send, handle it first
			else {
				if((spi_stack_send_to < SPI_ADDRESS_MIN) ||
				   (spi_stack_send_to > com_info.last_stack_address)) {
					RouteTo route_to = routing_route_stack_to(spi_stack_buffer_send[0] |
					                                          (spi_stack_buffer_send[1] << 8) |
					                                          (spi_stack_buffer_send[2] << 16) |
					                                          (spi_stack_buffer_send[3] << 24));
					spi_stack_send_to = route_to.to;
				}
				spi_stack_select(spi_stack_send_to);
				spi_stack_master_transceive();
				spi_stack_deselect();
	    		spi_stack_master_insert_position(spi_stack_buffer_recv, spi_stack_send_to);


			}
		}
		taskYIELD();
    }
}

void spi_stack_master_insert_position(void* data, const uint8_t position) {
	if(spi_stack_buffer_size_recv > sizeof(MessageHeader)) {
		EnumerateCallback *enum_cb =  (EnumerateCallback*)data;
		if(enum_cb->header.fid == FID_ENUMERATE_CALLBACK || enum_cb->header.fid == FID_GET_IDENTITY) {
			if(enum_cb->position == '0') {
				enum_cb->position = '0' + position;
				uid_to_serial_number(com_info.uid, enum_cb->connected_uid);
			}
		}
	}
}

void spi_stack_master_message_loop_return(const char *data, const uint16_t length) {
	send_blocking_with_timeout(data, length, com_info.current);
}

void spi_stack_master_message_loop(void *parameters) {
	MessageLoopParameter mlp;
	mlp.buffer_size = SPI_STACK_BUFFER_SIZE;
	mlp.com_type    = COM_SPI_STACK;
	mlp.return_func = spi_stack_master_message_loop_return;
	com_message_loop(&mlp);
}
