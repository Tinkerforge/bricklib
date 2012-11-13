/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_slave.c: SPI stack slave functionality
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

#include "spi_stack_slave.h"

#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/pio/pio_it.h"
#include <string.h>
#include <stdio.h>
#include "bricklib/drivers/spi/spi.h"
#include "bricklib/drivers/cmsis/core_cm3.h"
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include <stdbool.h>

#include "bricklib/bricklet/bricklet_config.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/pearson_hash.h"
#include "bricklib/com/com_common.h"
#include "bricklib/com/com_messages.h"

#include "spi_stack_common.h"
#include "config.h"

extern uint8_t spi_stack_buffer_recv[SPI_STACK_BUFFER_SIZE];
extern uint8_t spi_stack_buffer_send[SPI_STACK_BUFFER_SIZE];

// Recv and send buffer size for SPI stack (written by spi_stack_send/recv)
extern uint16_t spi_stack_buffer_size_send;
extern uint16_t spi_stack_buffer_size_recv;

static const Pin spi_slave_pins[] = {PINS_SPI, PIN_SPI_SELECT_SLAVE};

void SPI_IrqHandler(void) {
	if(spi_stack_buffer_size_recv != 0) {
		return;
	}

	// Disable SPI interrupt and interrupt controller
	SPI_DisableIt(SPI, SPI_IER_RDRF);
	__disable_irq();

	uint8_t slave_checksum = 0;
	uint8_t master_checksum = 0;
	PEARSON(slave_checksum, spi_stack_buffer_size_send);

	__attribute__((unused)) volatile uint8_t dummy;

	// Empty read register
	while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
	dummy = SPI->SPI_RDR;
	while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
	dummy = SPI->SPI_RDR;

	// Synchronize with master
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = 0xFF;
	while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
	dummy = SPI->SPI_RDR;

	// Write length
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = spi_stack_buffer_size_send;

	// Write first byte
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = spi_stack_buffer_send[0];
	PEARSON(slave_checksum, spi_stack_buffer_send[0]);

	// Read length from master
	while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
	uint8_t master_length = SPI->SPI_RDR;
    PEARSON(master_checksum, master_length);

    // If master and slave length are 0, stop communication
    if(master_length == 0 && spi_stack_buffer_size_send == 0) {
    	// Read dummy
        while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
        dummy = SPI->SPI_RDR;

    	// Write 0 (so there is no random 0xFF)
    	SPI->SPI_TDR = 0;

        SPI_EnableIt(SPI, SPI_IER_RDRF);
        __enable_irq();

        return;
    }

    // Length to transceive is maximum of slave and master length
    uint8_t max_length = MIN(MAX(spi_stack_buffer_size_send, master_length),
                             SPI_STACK_BUFFER_SIZE);

    // Exchange data
    for(uint8_t i = 1; i < max_length; i++) {
    	// Write
    	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
    	SPI->SPI_TDR = spi_stack_buffer_send[i];
    	PEARSON(slave_checksum, spi_stack_buffer_send[i]);

    	// Read
        while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
        spi_stack_buffer_recv[i-1] = SPI->SPI_RDR;
        PEARSON(master_checksum, spi_stack_buffer_recv[i-1]);
    }

    // Write CRC
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = slave_checksum;

	// Read last data byte
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    spi_stack_buffer_recv[max_length-1] = SPI->SPI_RDR;
    PEARSON(master_checksum, spi_stack_buffer_recv[max_length-1]);

	// Read CRC
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    uint8_t crc = SPI->SPI_RDR;

    // CRC correct?
    uint8_t slave_ack = crc == master_checksum;

	// Write ACK/NACK
	while((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = slave_ack;

	// Read ACK/NACK
    while((SPI->SPI_SR & SPI_SR_RDRF) == 0);
    uint8_t master_ack = SPI->SPI_RDR;

	// If everything OK, set sizes accordingly
    if(master_ack == 1 && slave_ack == 1) {
    	spi_stack_buffer_size_recv = master_length;
    	spi_stack_buffer_size_send = 0;
    }
    // Last byte written is 1 or 0 (ack/nack), so there can't be an
    // accidental 0xFF

    // Enable SPI interrupt only if no new data is received.
    // Otherwise the spi_recv function will enable interrupt again.
    if(spi_stack_buffer_size_recv == 0) {
    	SPI_EnableIt(SPI, SPI_IER_RDRF);
    }
    __enable_irq();
}

bool spi_stack_slave_is_selected(void) {
	return !PIO_Get(&spi_slave_pins[3]);
}

void spi_stack_slave_init(void) {
	PIO_Configure(spi_slave_pins, PIO_LISTSIZE(spi_slave_pins));

    // Configure SPI interrupts for Slave
    NVIC_DisableIRQ(SPI_IRQn);
    NVIC_ClearPendingIRQ(SPI_IRQn);
    NVIC_SetPriority(SPI_IRQn, PRIORITY_STACK_SLAVE_SPI);
    NVIC_EnableIRQ(SPI_IRQn);

    // Configure SPI peripheral
    SPI_Configure(SPI, ID_SPI, 0);
    SPI_ConfigureNPCS(SPI, 0 , 0);

    // Disable RX and TX DMA transfer requests
    SPI_PdcDisableTx(SPI);
    SPI_PdcDisableRx(SPI);

    // Enable SPI peripheral
    SPI_Enable(SPI);

    // Call interrupt on data in rx buffer
    SPI_EnableIt(SPI, SPI_IER_RDRF);
}

void spi_stack_slave_message_loop_return(char *data, const uint16_t length) {
	if(spi_stack_buffer_size_recv == 0) {
		SPI_EnableIt(SPI, SPI_IER_RDRF);
	}

	com_route_message_brick(data, length, COM_SPI_STACK);
}

void spi_stack_slave_message_loop(void *parameters) {
	MessageLoopParameter mlp;
	mlp.buffer_size = SPI_STACK_BUFFER_SIZE;
	mlp.com_type    = COM_SPI_STACK;
	mlp.return_func = spi_stack_slave_message_loop_return;
	com_message_loop(&mlp);
}
