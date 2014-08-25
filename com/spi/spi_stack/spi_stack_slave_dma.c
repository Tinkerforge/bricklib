/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_slave_sma.c: SPI stack slave functionality with DMA
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

#include "spi_stack_slave_dma.h"

#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/pio/pio_it.h"
#include <string.h>
#include <stdio.h>
#include "bricklib/drivers/spi/spi.h"
#include "bricklib/drivers/cmsis/core_cm3.h"
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include <stdbool.h>

#include "bricklib/utility/util_definitions.h"
#include "bricklib/com/com_common.h"
#include "bricklib/com/com_messages.h"

#include "spi_stack_common_dma.h"
#include "config.h"

extern uint8_t spi_stack_buffer_recv[SPI_STACK_MAX_MESSAGE_LENGTH];
extern uint8_t spi_stack_buffer_send[SPI_STACK_MAX_MESSAGE_LENGTH];

// Recv and send buffer size for SPI stack (written by spi_stack_send/recv)
extern uint16_t spi_stack_buffer_size_send;
extern uint16_t spi_stack_buffer_size_recv;

extern uint32_t led_rxtx;

static const Pin spi_slave_pins[] = {PINS_SPI, PIN_SPI_SELECT_SLAVE};

uint8_t spi_dma_buffer_recv[SPI_STACK_MAX_MESSAGE_LENGTH];
uint8_t spi_dma_buffer_send[SPI_STACK_MAX_MESSAGE_LENGTH];

uint8_t spi_stack_slave_master_seq = 0;
uint8_t spi_stack_slave_slave_seq = 0;

// Packet:
// Byte 0: Preamble
// Byte 1: Length == n+2
// Byte 2-n: Payload
// Byte n+1: Info (Busy Flag)
// Byte n+2: Checksum over bytes 0 to n+1

void spi_stack_slave_reset_recv_dma_buffer(void) {
	// Set preamble to something invalid, so we can't accidentally
	// Reuse old data
	spi_dma_buffer_recv[SPI_STACK_PREAMBLE] = 0;
    SPI->SPI_RPR = (uint32_t)spi_dma_buffer_recv;
    SPI->SPI_RCR = SPI_STACK_MAX_MESSAGE_LENGTH;
	SPI->SPI_PTCR = SPI_PTCR_RXTEN;
}

void spi_stack_slave_reset_send_dma_buffer(void) {
	const uint8_t length = spi_dma_buffer_send[SPI_STACK_LENGTH];
	const uint8_t mask = SPI_STACK_INFO_SEQUENCE_SLAVE_MASK | SPI_STACK_INFO_SEQUENCE_MASTER_MASK;
	const uint8_t value = spi_stack_slave_slave_seq | spi_stack_slave_master_seq;
	const uint8_t pos = SPI_STACK_INFO(length);

	if((spi_dma_buffer_send[pos] & mask) != value) {
		spi_dma_buffer_send[pos] = (spi_dma_buffer_send[pos] & (~mask)) | value;
		spi_dma_buffer_send[SPI_STACK_CHECKSUM(length)] = spi_stack_calculate_pearson(spi_dma_buffer_send, length-1);
	}

    SPI->SPI_TPR = (uint32_t)spi_dma_buffer_send;
    SPI->SPI_TCR = SPI_STACK_MAX_MESSAGE_LENGTH;
	SPI->SPI_PTCR = SPI_PTCR_TXTEN;
}

void spi_stack_slave_handle_irq_recv(void) {
	// Check preamble
	if(spi_dma_buffer_recv[SPI_STACK_PREAMBLE] != SPI_STACK_PREAMBLE_VALUE) {
		spi_stack_slave_reset_recv_dma_buffer();
		return;
	}

	// Check length
	const uint8_t length = spi_dma_buffer_recv[SPI_STACK_LENGTH];
	if(length == SPI_STACK_EMPTY_MESSAGE_LENGTH) {
		//logspisd("Empty message (recv)\n\r");

		// Check if last slave sequence number that the master has seen is the same as the last one we send.
		if((spi_dma_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_SLAVE_MASK) == spi_stack_slave_slave_seq) {
			// We set the preamble in the send buffer to zero to show that a new message can be send.
			spi_dma_buffer_send[0] = 0;
			spi_stack_increase_slave_seq(&spi_stack_slave_slave_seq);
		}

		spi_stack_slave_master_seq = spi_dma_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_MASTER_MASK;
		spi_stack_slave_reset_recv_dma_buffer();
		return;
	}

	if((length < SPI_STACK_MESSAGE_LENGTH_MIN) ||
	   (length > SPI_STACK_MAX_MESSAGE_LENGTH)) {
		logspisw("Length is not proper: %d\n\r", length);
		spi_stack_slave_reset_recv_dma_buffer();
		return;
	}

	// Calculate and check checksum
	const uint8_t checksum = spi_stack_calculate_pearson(spi_dma_buffer_recv, length-1);
	if(checksum != spi_dma_buffer_recv[SPI_STACK_CHECKSUM(length)]) {
		logspisw("Checksum not proper: %x != %x\n\r",
				 checksum,
				 spi_dma_buffer_recv[SPI_STACK_CHECKSUM(length)]);
		spi_stack_slave_reset_recv_dma_buffer();
		return;
	}

	// Check if last slave sequence number that the master has seen is the same as the last one we send.
	if((spi_dma_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_SLAVE_MASK) == spi_stack_slave_slave_seq) {
		// We set the preamble in the send buffer to zero to show that a new message can be send.
		spi_dma_buffer_send[0] = 0;
		spi_stack_increase_slave_seq(&spi_stack_slave_slave_seq);
	}

	// Check if master sequence number is the same as the last one we have seen. In this case we ignore the message
	// to make sure that we don't handle a message two times.
	if((spi_dma_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_MASTER_MASK) == spi_stack_slave_master_seq) {
		logspisw("Received same master sequence number two times: %x\n\r",
		         spi_stack_slave_master_seq);

		spi_stack_slave_reset_recv_dma_buffer();
		return;
	} else {
		spi_stack_slave_master_seq = spi_dma_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_MASTER_MASK;
	}

	// Everything seems OK, can we copy the DMA buffer?
	if(spi_stack_buffer_size_recv == 0) {
		for(uint8_t j = 0; j < length - SPI_STACK_EMPTY_MESSAGE_LENGTH; j++) {
			spi_stack_buffer_recv[j] = spi_dma_buffer_recv[2+j];
		}
		spi_stack_buffer_size_recv = length - SPI_STACK_EMPTY_MESSAGE_LENGTH;

		spi_stack_slave_reset_recv_dma_buffer();
		return;
	} else {
		// If the Master behaves as specified this else branch
		// can't be reached
		logspise("We received packet while recv buffer full, throwing away message\n\r");
		spi_stack_slave_reset_recv_dma_buffer();
		return;
	}
}

void spi_stack_slave_handle_irq_send(void) {
	// If the preamble is still set in the dma buffer, we did have
	// an checksum error or similar and we need to send the message again.
	// Otherwise the recv code would have overwritten the preamble
	if(spi_dma_buffer_send[0] == SPI_STACK_PREAMBLE_VALUE) {
		spi_stack_slave_reset_send_dma_buffer();
		return;
	} else if(spi_stack_buffer_size_send > 0) {
		// If we have something to send we will immediately give it to the DMA
		const uint8_t length = spi_stack_buffer_size_send + SPI_STACK_EMPTY_MESSAGE_LENGTH;

		// Set preamble, length and data
		spi_dma_buffer_send[SPI_STACK_PREAMBLE] = SPI_STACK_PREAMBLE_VALUE;
		spi_dma_buffer_send[SPI_STACK_LENGTH] = length;
		for(uint8_t i = 0; i < spi_stack_buffer_size_send; i++) {
			spi_dma_buffer_send[i+2] = spi_stack_buffer_send[i];
		}

		// If recv buffer is not empty we are busy
		if(spi_stack_buffer_size_recv > 0) {
			spi_dma_buffer_send[SPI_STACK_INFO(length)] = SPI_STACK_INFO_BUSY | spi_stack_slave_slave_seq | spi_stack_slave_master_seq;
		} else {
			spi_dma_buffer_send[SPI_STACK_INFO(length)] = 0 | spi_stack_slave_slave_seq | spi_stack_slave_master_seq;
		}

		// Calculate checksum
		spi_dma_buffer_send[SPI_STACK_CHECKSUM(length)] = spi_stack_calculate_pearson(spi_dma_buffer_send, length-1);

		logspisd("Sending message: length(%d), info(%x), checksum(%x)\n\r",
		         length,
		         spi_dma_buffer_send[SPI_STACK_INFO(length)],
		         spi_dma_buffer_send[SPI_STACK_CHECKSUM(length)]);


		// If we had an over- or under-run in the meantime we have to abort the
		// sending of data.
		volatile uint32_t status = SPI->SPI_SR;
		if(!(status & (SPI_SR_OVRES | SPI_SR_UNDES))) {
			// We are done, send buffer can now be given to dma
			spi_stack_slave_reset_send_dma_buffer();

			// Wait for one byte to be transfered and check again.
			// We have a very unlikely race condition here that we
			// need to check for. One byte at 8MHz takes exactly 1us.
			SLEEP_US(1);

			status = SPI->SPI_SR;
			if(!(status & (SPI_SR_OVRES | SPI_SR_UNDES))) {
				// Everything is copied and seems OK, we can set the buffer free again
				spi_stack_buffer_size_send = 0;
			}
		}
		return;
	} else if(spi_stack_buffer_size_recv > 0) {
		// if we don't have anything to send and the recv buffer is full,
		// we have to give the DMA an empty packet with BUSY flag set.
		spi_dma_buffer_send[SPI_STACK_PREAMBLE] = SPI_STACK_PREAMBLE_VALUE;
		spi_dma_buffer_send[SPI_STACK_LENGTH] = SPI_STACK_EMPTY_MESSAGE_LENGTH;
		spi_dma_buffer_send[SPI_STACK_INFO(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = SPI_STACK_INFO_BUSY | spi_stack_slave_slave_seq | spi_stack_slave_master_seq;
		spi_dma_buffer_send[SPI_STACK_CHECKSUM(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = spi_stack_calculate_pearson(spi_dma_buffer_send, SPI_STACK_EMPTY_MESSAGE_LENGTH-1);

		spi_stack_slave_reset_send_dma_buffer();
		return;
	}

	// Otherwise the send buffer and the recv buffer are empty
	// In this case we have to send and empty message with busy flag unset
	spi_dma_buffer_send[SPI_STACK_PREAMBLE] = SPI_STACK_PREAMBLE_VALUE;
	spi_dma_buffer_send[SPI_STACK_LENGTH] = SPI_STACK_EMPTY_MESSAGE_LENGTH;
	spi_dma_buffer_send[SPI_STACK_INFO(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = 0 | spi_stack_slave_slave_seq | spi_stack_slave_master_seq;
	spi_dma_buffer_send[SPI_STACK_CHECKSUM(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = spi_stack_calculate_pearson(spi_dma_buffer_send, SPI_STACK_EMPTY_MESSAGE_LENGTH-1);

	spi_stack_slave_reset_send_dma_buffer();
}

void spi_stack_slave_irq(void) {
	volatile uint32_t status = SPI->SPI_SR;

	if(status & SPI_SR_NSSR) {
		//logspisd("NSSR: Status(%x), RCR(%d), TCR(%d)\n\r", status, SPI->SPI_RCR, SPI->SPI_TCR);

		// Either read or transmit DMA buffer are not empty, this can mean two things:
		// 1. We had a glitch in the slave select pin
		// 2. Something went wrong during the transfer (we didn't see all clock bits or similar)
		// In both cases we will wait for the next select. Is there a better way to handle this?
		if(SPI->SPI_RCR != 0 || SPI->SPI_TCR != 0) {
			return;
		}

		// If an overrun or underrun occurred, we can't be sure that there is not currently
		// another transmission in progress.
		// We wait for a max time of approximately 1ms.
		int32_t counter = 64*1000;
		if(status & (SPI_SR_OVRES | SPI_SR_UNDES)) {
			status = SPI->SPI_SR;
			do{
				counter--;
				if(counter <= 0) {
					return;
				}
				status = SPI->SPI_SR;
			} while(!(status & SPI_SR_NSSR));
		}

		// Reset relevant SPI hardware, just in case we had some funny bit offset or similar
	    SPI_Disable(SPI);
	    SPI->SPI_CR = SPI_CR_SWRST;
	    SPI_Configure(SPI, ID_SPI, 0);
	    SPI_ConfigureNPCS(SPI, 0 , 0);
	    SPI_Enable(SPI);
		SPI->SPI_PTCR = SPI_PTCR_RXTDIS;
		SPI->SPI_PTCR = SPI_PTCR_TXTDIS;
		SPI_EnableIt(SPI, SPI_IER_NSSR);

		// Handle recv and send buffer handling
		spi_stack_slave_handle_irq_recv();
		spi_stack_slave_handle_irq_send();
	}
}

void spi_stack_slave_init(void) {
	PIO_Configure(spi_slave_pins, PIO_LISTSIZE(spi_slave_pins));

    // Disable SPI peripheral
    SPI_Disable(SPI);

    // Configure SPI interrupts for Slave
    NVIC_DisableIRQ(SPI_IRQn);
    NVIC_ClearPendingIRQ(SPI_IRQn);
    NVIC_SetPriority(SPI_IRQn, PRIORITY_STACK_SLAVE_SPI);
    NVIC_EnableIRQ(SPI_IRQn);

    // SPI reset
    SPI->SPI_CR = SPI_CR_SWRST;

    // Configure SPI peripheral
    SPI_Configure(SPI, ID_SPI, 0);
    SPI_ConfigureNPCS(SPI, 0 , 0);

    // Enable SPI peripheral
    SPI_Enable(SPI);

    // Disable RX and TX DMA transfer requests
    SPI->SPI_PTCR = SPI_PTCR_RXTDIS | SPI_PTCR_TXTDIS;

    spi_stack_slave_reset_recv_dma_buffer();

	spi_dma_buffer_send[SPI_STACK_PREAMBLE] = SPI_STACK_PREAMBLE_VALUE;
	spi_dma_buffer_send[SPI_STACK_LENGTH] = SPI_STACK_EMPTY_MESSAGE_LENGTH;
	spi_dma_buffer_send[SPI_STACK_INFO(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = 0 | spi_stack_slave_slave_seq | spi_stack_slave_master_seq;
	spi_dma_buffer_send[SPI_STACK_CHECKSUM(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = spi_stack_calculate_pearson(spi_dma_buffer_send, SPI_STACK_EMPTY_MESSAGE_LENGTH-1);

	spi_stack_slave_reset_send_dma_buffer();

    // Call interrupt on end of slave select
    SPI_EnableIt(SPI, SPI_IER_NSSR);
}

void spi_stack_slave_message_loop_return(const char *data, const uint16_t length) {
	com_route_message_brick(data, length, COM_SPI_STACK);
}

void spi_stack_slave_message_loop(void *parameters) {
	MessageLoopParameter mlp;
	mlp.buffer_size = SPI_STACK_BUFFER_SIZE;
	mlp.com_type    = COM_SPI_STACK;
	mlp.return_func = spi_stack_slave_message_loop_return;
	com_message_loop(&mlp);
}

uint16_t spi_stack_slave_send(const void *data, const uint16_t length, uint32_t *options) {
	if(spi_stack_buffer_size_send > 0) {
		return 0;
	}

	led_rxtx++;

	uint16_t send_length = MIN(length, SPI_STACK_BUFFER_SIZE);

	memcpy(spi_stack_buffer_send, data, send_length);
	spi_stack_buffer_size_send = send_length;

	return send_length;
}

uint16_t spi_stack_slave_recv(void *data, const uint16_t length, uint32_t *options) {
	if(spi_stack_buffer_size_recv == 0) {
		return 0;
	}

	led_rxtx++;

	static uint16_t recv_pointer = 0;

	uint16_t recv_length = MIN(length, spi_stack_buffer_size_recv);

	memcpy(data, spi_stack_buffer_recv + recv_pointer, recv_length);

	if(spi_stack_buffer_size_recv - recv_length == 0) {
		recv_pointer = 0;
	} else {
		recv_pointer += recv_length;
	}

	spi_stack_buffer_size_recv -= recv_length;

	return recv_length;
}
