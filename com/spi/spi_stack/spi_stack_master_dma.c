/* bricklib
 * Copyright (C) 2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_stack_master_dma.c: SPI stack DMA master functionality
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

#include "spi_stack_master_dma.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bricklib/com/com_common.h"
#include "bricklib/com/com_messages.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"
#include "bricklib/drivers/cmsis/core_cm3.h"
#include "bricklib/drivers/crc/crc.h"
#include "bricklib/drivers/uid/uid.h"
#include "bricklib/drivers/spi/spi.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/pio/pio_it.h"
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/pearson_hash.h"

#include "extensions/rs485/rs485_low_level.h"

#include "spi_stack_select.h"
#include "spi_stack_common_dma.h"
#include "config.h"
#include "routing.h"

#define SPI_DLYBCS(delay, mc) ((uint32_t)((mc/1000000) * delay) << 24)
#define SPI_STACK_MASTER_TIMEOUT 10000

#define SPI_STACK_TIME_BETWEEN_SELECT (640*5)   // 640 cycles  = 10us

extern uint8_t spi_stack_buffer_recv[SPI_STACK_MAX_MESSAGE_LENGTH];
extern uint8_t spi_stack_buffer_send[SPI_STACK_MAX_MESSAGE_LENGTH];

// Recv and send buffer size for SPI stack (written by spi_stack_send/recv)
extern uint16_t spi_stack_buffer_size_send;
extern uint16_t spi_stack_buffer_size_recv;

extern Pin spi_select_master[];
extern ComInfo com_info;

extern uint32_t led_rxtx;

uint8_t spi_stack_master_master_seq[SPI_ADDRESS_MAX] = {1, 1, 1, 1, 1, 1, 1, 1};
uint8_t spi_stack_master_slave_seq[SPI_ADDRESS_MAX] = {0};

// Note that stack address is "1 based" (0 is master of stack)
// while slave_status is 0 based, don't forget the -1!
SPIStackMasterSlaveStatus slave_status[SPI_ADDRESS_MAX] = {
	SLAVE_STATUS_AVAILABLE, SLAVE_STATUS_AVAILABLE, SLAVE_STATUS_AVAILABLE, SLAVE_STATUS_AVAILABLE,
	SLAVE_STATUS_AVAILABLE, SLAVE_STATUS_AVAILABLE, SLAVE_STATUS_AVAILABLE, SLAVE_STATUS_AVAILABLE
};

uint8_t stack_address_counter = 1;
uint8_t stack_address_current = 1;
uint8_t stack_address_broadcast = 0;
SPIStackMasterTransceiveState transceive_state = TRANSCEIVE_STATE_MESSAGE_EMPTY;

int32_t spi_stack_deselect_time = 0;

void spi_stack_master_init(void) {
	// Set starting sequence number to something that slave does not expect
	// (default for slave is 0)
	//spi_stack_master_seq = 1;

	for(uint8_t i = 0; i < SPI_ADDRESS_MAX; i++) {
		slave_status[i] = SLAVE_STATUS_ABSENT;
	}

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

    // Configure SPI interrupts for Master
    NVIC_DisableIRQ(SPI_IRQn);
    NVIC_ClearPendingIRQ(SPI_IRQn);
    NVIC_SetPriority(SPI_IRQn, PRIORITY_STACK_MASTER_SPI);
    NVIC_EnableIRQ(SPI_IRQn);

    // SPI reset
    SPI->SPI_CR = SPI_CR_SWRST;

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

    // Disable RX and TX DMA transfer requests
    spi_stack_master_disable_dma();

    // Enable SPI peripheral.
    SPI_Enable(SPI);

	spi_stack_master_reset_recv_dma_buffer();
	spi_stack_master_reset_send_dma_buffer();

    // Call interrupt on end of slave select
    SPI_EnableIt(SPI, SPI_IER_ENDRX | SPI_IER_ENDTX);
}

void spi_stack_master_reset_recv_dma_buffer(void) {
	// Set preamble to something invalid, so we can't accidentally
	// Reuse old data
	spi_stack_buffer_recv[SPI_STACK_PREAMBLE] = 0;
    SPI->SPI_RPR = (uint32_t)spi_stack_buffer_recv;
    SPI->SPI_RCR = SPI_STACK_MAX_MESSAGE_LENGTH;
}

void spi_stack_master_reset_send_dma_buffer(void) {
	const uint8_t length = spi_stack_buffer_send[SPI_STACK_LENGTH];
	const uint8_t mask = SPI_STACK_INFO_SEQUENCE_SLAVE_MASK | SPI_STACK_INFO_SEQUENCE_MASTER_MASK;
	const uint8_t value = spi_stack_master_slave_seq[stack_address_current-1] | spi_stack_master_master_seq[stack_address_current-1];
	const uint8_t pos = SPI_STACK_INFO(length);

	if((spi_stack_buffer_send[pos] & mask) != value) {
		spi_stack_buffer_send[pos] = (spi_stack_buffer_send[pos] & (~mask)) | value;
		spi_stack_buffer_send[SPI_STACK_CHECKSUM(length)] = spi_stack_calculate_pearson(spi_stack_buffer_send, length-1);
	}

    SPI->SPI_TPR = (uint32_t)spi_stack_buffer_send;
    SPI->SPI_TCR = SPI_STACK_MAX_MESSAGE_LENGTH;
}

void spi_stack_master_enable_dma(void) {
	transceive_state = TRANSCEIVE_STATE_BUSY;
	spi_stack_select(stack_address_current);
	spi_stack_master_reset_recv_dma_buffer();
	spi_stack_master_reset_send_dma_buffer();
	SPI_EnableIt(SPI, SPI_IER_ENDRX | SPI_IER_ENDTX);
	SPI->SPI_PTCR = SPI_PTCR_RXTEN | SPI_PTCR_TXTEN;
}

void spi_stack_master_disable_dma(void) {
	SPI_DisableIt(SPI, SPI_IER_ENDRX | SPI_IER_ENDTX);
	SPI->SPI_PTCR = SPI_PTCR_RXTDIS | SPI_PTCR_TXTDIS;
	spi_stack_deselect();
}

void spi_stack_master_make_empty_send_packet(void) {
	spi_stack_buffer_send[SPI_STACK_PREAMBLE] = SPI_STACK_PREAMBLE_VALUE;
	spi_stack_buffer_send[SPI_STACK_LENGTH] = SPI_STACK_EMPTY_MESSAGE_LENGTH;
	spi_stack_buffer_send[SPI_STACK_INFO(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = spi_stack_master_master_seq[stack_address_current-1] | spi_stack_master_slave_seq[stack_address_current-1];
	spi_stack_buffer_send[SPI_STACK_CHECKSUM(SPI_STACK_EMPTY_MESSAGE_LENGTH)] = spi_stack_calculate_pearson(spi_stack_buffer_send, SPI_STACK_EMPTY_MESSAGE_LENGTH-1);

	spi_stack_buffer_size_send = 0;
}

SPIStackMasterTransceiveInfo spi_stack_master_start_transceive(const uint8_t *data, const uint8_t length, const uint8_t stack_address) {
	__disable_irq();

	// If the last deselect just happened we return as busy.
	// In this case the slave may not be ready yet, we have to give him some
	// more time.
	int32_t current_time = SysTick->VAL;
	if(spi_stack_deselect_time != current_time) {
		if(spi_stack_deselect_time < current_time) {
			spi_stack_deselect_time += 63999;
		}

		if(current_time + SPI_STACK_TIME_BETWEEN_SELECT > spi_stack_deselect_time) {
			__enable_irq();
			return TRANSCEIVE_INFO_SEND_NOTHING_BUSY;
		}
	}

	if(spi_stack_buffer_size_recv > 0) {
		// The receive buffer is not empty, we can not transceive!
		__enable_irq();
		return TRANSCEIVE_INFO_SEND_NOTHING_BUSY;
	}

	if(transceive_state == TRANSCEIVE_STATE_MESSAGE_EMPTY) {
		if(stack_address == 0) { // We are called with sa = 0 if nothing is to send
			// There is nothing to send,
			// we send a message with empty payload (4 byte)

			// Since there is nothing to send we can ask the slaves round robin
			stack_address_counter++;
			if(stack_address_counter > com_info.last_stack_address) {
				stack_address_counter = 1;
			}

			stack_address_current = stack_address_counter;

			spi_stack_master_make_empty_send_packet();
			spi_stack_master_enable_dma();

			__enable_irq();
			return TRANSCEIVE_INFO_SEND_EMPTY_MESSAGE;
		}

		stack_address_current = stack_address;
		if(slave_status[stack_address-1] == SLAVE_STATUS_AVAILABLE) {
			// The send buffer is empty, there is something to send
			// and the slave is ready to receive data, that means we can copy the data!

			spi_stack_buffer_send[SPI_STACK_PREAMBLE] = SPI_STACK_PREAMBLE_VALUE;
			spi_stack_buffer_send[SPI_STACK_LENGTH] = length + SPI_STACK_EMPTY_MESSAGE_LENGTH;
			spi_stack_buffer_send[SPI_STACK_INFO(spi_stack_buffer_send[SPI_STACK_LENGTH])] = 0 | spi_stack_master_master_seq[stack_address_current-1] | spi_stack_master_slave_seq[stack_address_current-1]; // Master is never busy

			for(uint8_t i = 0; i < length; i++) {
				spi_stack_buffer_send[i+2] = data[i];
			}

			spi_stack_buffer_send[SPI_STACK_CHECKSUM(spi_stack_buffer_send[SPI_STACK_LENGTH])] = spi_stack_calculate_pearson(spi_stack_buffer_send, spi_stack_buffer_send[SPI_STACK_LENGTH]-1);
			spi_stack_buffer_size_send = length;

			spi_stack_master_enable_dma();
			__enable_irq();
			return TRANSCEIVE_INFO_SEND_OK;
		} else {
			// This case should not be reachable.
			// What can we do here?
			__enable_irq();
			return TRANSCEIVE_INFO_SEND_ERROR;
		}
	} else if(transceive_state == TRANSCEIVE_STATE_MESSAGE_READY) {
		// There is a ready-to-send message in the send buffer that we
		// were not able to send for some reason, lets send it.
		spi_stack_master_enable_dma();
		__enable_irq();
		return TRANSCEIVE_INFO_SEND_NOTHING_BUSY;
	} else if(transceive_state == TRANSCEIVE_STATE_BUSY) {
		// There is currently data in transfer
		__enable_irq();
		return TRANSCEIVE_INFO_SEND_NOTHING_BUSY;
	}

	// This should not be reachable either.
	// What can we do here?
	__enable_irq();
	return TRANSCEIVE_INFO_SEND_ERROR;
}

void spi_stack_master_irq(void) {
	volatile uint32_t status = SPI->SPI_SR;

	// We only do anything here if RX and TX are finished.
	if((status & (SPI_SR_ENDRX | SPI_SR_ENDTX)) == (SPI_SR_ENDRX | SPI_SR_ENDTX)) {
		spi_stack_master_disable_dma();

		if(spi_stack_buffer_recv[SPI_STACK_PREAMBLE] != SPI_STACK_PREAMBLE_VALUE) {
			// An "unproper preamble" is part of the protocol,
			// if the slave is too busy to fill the DMA buffers fast enough.
			// If we did try to send something we need to try again.

			transceive_state = TRANSCEIVE_STATE_MESSAGE_READY;
			return;
		}

		uint8_t length = spi_stack_buffer_recv[SPI_STACK_LENGTH];
		if((length != SPI_STACK_EMPTY_MESSAGE_LENGTH) &&
		   ((length < SPI_STACK_MESSAGE_LENGTH_MIN) ||
		    (length > SPI_STACK_MAX_MESSAGE_LENGTH))) {
			logspise("Received packet with malformed length: %d\n\r", length);

			transceive_state = TRANSCEIVE_STATE_MESSAGE_READY;
			return;
		}

		uint8_t checksum = spi_stack_calculate_pearson(spi_stack_buffer_recv, length-1);
		if(checksum != spi_stack_buffer_recv[SPI_STACK_CHECKSUM(length)]) {
			logspise("Received packet with wrong checksum (actual: %x != expected: %x)\n\r",
			         checksum, spi_stack_buffer_recv[SPI_STACK_CHECKSUM(length)]);

			// Commented out code below is for debugging wrong checksum problems.
			// This should only be needed during initial development, we leave it here just in case.
/*
			logwohe("spi packet (%d): preamble(%d), length(%d), info(%d), checksum(%d)\n\r",
					stack_address_current,
					spi_stack_buffer_recv[SPI_STACK_PREAMBLE],
					spi_stack_buffer_recv[SPI_STACK_LENGTH],
					spi_stack_buffer_recv[SPI_STACK_INFO(length)],
					spi_stack_buffer_recv[SPI_STACK_CHECKSUM(length)]);

			MessageHeader *header = ((MessageHeader*)(spi_stack_buffer_recv+2));

			logwohe("Message UID: %lu", header->uid);
			logwohe(", length: %d", header->length);
			logwohe(", fid: %d", header->fid);
			logwohe(", (seq#, r, a, oo): (%d, %d, %d, %d)", header->sequence_num, header->return_expected, header->authentication, header->other_options);
			logwohe(", (e, fu): (%d, %d)\n\r", header->error, header->future_use);

			logwohe("Message data: [");

			uint8_t *data = (uint8_t*)header;
			for(uint8_t i = 0; i < header->length - 8; i++) {
				logwohe("%d ", data[i+8]);
			}
			logwohe("]\n\r");*/

			transceive_state = TRANSCEIVE_STATE_MESSAGE_READY;
			return;
		}

		// If the sequence number is the same as the last time, we already
		// handled this response, we don't handle it again in this case
		if((spi_stack_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_SLAVE_MASK) != spi_stack_master_slave_seq[stack_address_current-1]) {
			spi_stack_master_slave_seq[stack_address_current-1] = spi_stack_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_SLAVE_MASK;

			if(length == SPI_STACK_EMPTY_MESSAGE_LENGTH) {
				// We didn't receive anything, there is nothing to copy anywhere
			} else {
				for(uint8_t i = 0; i < length-SPI_STACK_EMPTY_MESSAGE_LENGTH; i++) {
					spi_stack_buffer_recv[i] = spi_stack_buffer_recv[i+2];
				}

				spi_stack_buffer_size_recv = length-SPI_STACK_EMPTY_MESSAGE_LENGTH;

				// Insert stack position (in case of Enumerate or GetIdentity).
				// The SPI Slave can not know its position in the stack.
				spi_stack_master_insert_position(spi_stack_buffer_recv, stack_address_current);
			}
		}

		// If the slave received our last message or we didn't send anything anyway, we can increase sequence number
		if(((spi_stack_buffer_recv[SPI_STACK_INFO(length)] & SPI_STACK_INFO_SEQUENCE_MASTER_MASK) != spi_stack_master_master_seq[stack_address_current-1]) && (spi_stack_buffer_size_send > 0)) {
			transceive_state = TRANSCEIVE_STATE_MESSAGE_READY;
			return;
		} else {
			spi_stack_increase_master_seq(&spi_stack_master_master_seq[stack_address_current-1]);
			spi_stack_buffer_size_send = 0;
		}

		transceive_state = TRANSCEIVE_STATE_MESSAGE_EMPTY;
	}
}

// Insert stack address into enumerate message
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
	// To be backward compatible to older firmwares we do not send
	// "enumerate-added callback" through to an ethernet socket
	if(((EnumerateCallback*)data)->header.fid == FID_ENUMERATE_CALLBACK &&
	   ((EnumerateCallback*)data)->enumeration_type == ENUMERATE_TYPE_ADDED &&
	   com_info.current == COM_ETHERNET) {
		return;
	}

	send_blocking_with_timeout(data, length, com_info.current);
}

void spi_stack_master_message_loop(void *parameters) {
	MessageLoopParameter mlp;
	mlp.buffer_size = SPI_STACK_BUFFER_SIZE;
	mlp.com_type    = COM_SPI_STACK;
	mlp.return_func = spi_stack_master_message_loop_return;
	com_message_loop(&mlp);
}


uint16_t spi_stack_master_send(const void *data, const uint16_t length, uint32_t *options) {
	if(spi_stack_buffer_size_send > 0 || transceive_state != TRANSCEIVE_STATE_MESSAGE_EMPTY) {
		return 0;
	}

	// If the stack address is in the options, we use it
	if(options && *options >= SPI_ADDRESS_MIN && *options <= com_info.last_stack_address) {
		if(spi_stack_master_start_transceive(data, length, *options) != TRANSCEIVE_INFO_SEND_OK) {
			return 0;
		}
	} else {
		// We should not need to do our own routing here, the (commented out) code below implements
		// a theoretical routing that we should not need to use.
		// Keep an eye out for this error messages!
		if(options == NULL) {
			logspise("spi_stack_master_send called with invalid option: NULL\n\r");
		} else {
			logspise("spi_stack_master_send called with invalid option: %d\n\r", *options);
		}

/*		// Otherwise we try to find it in the routing table
		RouteTo route_to = routing_route_stack_to(spi_stack_buffer_send[0] |
												  (spi_stack_buffer_send[1] << 8) |
												  (spi_stack_buffer_send[2] << 16) |
												  (spi_stack_buffer_send[3] << 24));

		// If it is not in there, we broadcast the message
		if(route_to.option < SPI_ADDRESS_MIN || route_to.option > com_info.last_stack_address) {
			stack_address_broadcast = 1;
			if(spi_stack_master_start_transceive(data, length, 1) != TRANSCEIVE_INFO_SEND_OK) {
				stack_address_broadcast = 0;
				return 0;
			}
		} else {
			// Otherwise we can send it the routing table address
			if(spi_stack_master_start_transceive(data, length, route_to.option) != TRANSCEIVE_INFO_SEND_OK) {
				return 0;
			}
		}*/
	}

	led_rxtx++;

	return length;
}


uint16_t spi_stack_master_recv(void *data, const uint16_t length, uint32_t *options) {
	if(spi_stack_buffer_size_recv == 0) {
		// Use recv loop to trigger regular SPI transmits.
		// It will scale automatically with the utilization of the Master
		// and we don't need a SPI recv loop anymore, sweet!
		if(com_info.current != COM_NONE) {
			// No need to send data over SPI if there is no connection to
			// any of the interfaces yet
			spi_stack_master_start_transceive(NULL, 0, 0);
		}
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

	// Also start a SPI transmission after we read out the recv buffer.
	spi_stack_master_start_transceive(NULL, 0, 0);

	return recv_length;
}
