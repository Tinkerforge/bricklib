/* bricklib
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_co_mcu.c: Functions for Bricklets with co-processor
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

#include "bricklet_co_mcu.h"

#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/init.h"
#include "bricklib/utility/ringbuffer.h"
#include "bricklib/utility/pearson_hash.h"

#include "bricklet_config.h"
#include "config.h"

#include <string.h>

#define BUFFER_SIZE_SEND 80
#define BUFFER_SIZE_RECV 154
#define BUFFER_SEND_ACK_TIMEOUT 20 // in ms
#define MAX_TRIES_IF_NOT_CONNECTED 100

#define PROTOCOL_OVERHEAD 3 // 3 byte overhead for Brick <-> Bricklet protocol
#define MIN_TFP_MESSAGE_LENGTH (8 + PROTOCOL_OVERHEAD)
#define MAX_TFP_MESSAGE_LENGTH (80 + PROTOCOL_OVERHEAD)
#define BAUDRATE 400000
#define SLEEP_HALF_BIT_NS (((1000*1000*1000/2)/BAUDRATE) - 200) // We subtract 200ns for setting of pins etc

typedef enum {
	STATE_START,
	STATE_ACK_SEQUENCE_NUMBER,
	STATE_ACK_CHECKSUM,
	STATE_MESSAGE_SEQUENCE_NUMBER,
	STATE_MESSAGE_DATA,
	STATE_MESSAGE_CHECKSUM
} CoMCURecvState;

typedef union {
	struct {
		uint8_t got_message:1;
		uint8_t tries:7;
	} access;
	uint8_t data;
} CoMCURecvAvailability;

typedef struct {
	CoMCURecvAvailability availability;
	uint8_t buffer_send_length;
	int16_t buffer_send_ack_timeout;
	Ringbuffer ringbuffer_recv;
	uint8_t current_sequence_number;
	uint8_t last_sequence_number_seen;
	uint8_t buffer_send[BUFFER_SIZE_SEND];
	uint8_t buffer_recv[BUFFER_SIZE_RECV];
} CoMCUData;

extern BrickletSettings bs[BRICKLET_NUM];
extern uint32_t bc[BRICKLET_NUM][BRICKLET_CONTEXT_MAX_SIZE/4];
extern ComInfo com_info;

#define SPI_SS(i)   (bs[i].pin1_ad)
#define SPI_CLK(i)  (bs[i].pin2_da)
#define SPI_MOSI(i) (bs[i].pin3_pwm)
#define SPI_MISO(i) (bs[i].pin4_io)

#define CO_MCU_DATA(i) ((CoMCUData*)(bc[i]))

void bricklet_co_mcu_init(const uint8_t bricklet_num) {
	logd("Initialize CO MCU Bricklet %c\n\r", 'a' + bricklet_num);
	_Static_assert(sizeof(CoMCUData) <= BRICKLET_CONTEXT_MAX_SIZE, "CoMCUData too big");

	SPI_SS(bricklet_num).type = PIO_OUTPUT_1;
	SPI_SS(bricklet_num).attribute = PIO_DEFAULT;
	PIO_Configure(&SPI_SS(bricklet_num), 1);

	SPI_CLK(bricklet_num).type = PIO_OUTPUT_1;
	SPI_CLK(bricklet_num).attribute = PIO_DEFAULT;
	PIO_Configure(&SPI_CLK(bricklet_num), 1);

	SPI_MOSI(bricklet_num).type = PIO_OUTPUT_1;
	SPI_MOSI(bricklet_num).attribute = PIO_DEFAULT;
	PIO_Configure(&SPI_MOSI(bricklet_num), 1);

	SPI_MISO(bricklet_num).type = PIO_INPUT;
	SPI_MISO(bricklet_num).attribute = PIO_PULLDOWN;
	PIO_Configure(&SPI_MISO(bricklet_num), 1);

	memset(CO_MCU_DATA(bricklet_num)->buffer_send, 0, BUFFER_SIZE_SEND);
	memset(CO_MCU_DATA(bricklet_num)->buffer_recv, 0, BUFFER_SIZE_RECV);
	CO_MCU_DATA(bricklet_num)->availability.access.got_message = false;
	CO_MCU_DATA(bricklet_num)->availability.access.tries = 0;
	CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = 50; // Wait for 50ms with first send
	CO_MCU_DATA(bricklet_num)->current_sequence_number = 1;
	CO_MCU_DATA(bricklet_num)->last_sequence_number_seen = 0;

	// Send initial enumerate. This will send back the initial enumeration callback
	// of type added. We also use this to set the UID for the first time.
	CoMCUEnumerate co_mcu_enumerate;
	co_mcu_enumerate.header.uid              = 0;
	co_mcu_enumerate.header.length           = sizeof(Enumerate);
	co_mcu_enumerate.header.fid              = FID_CO_MCU_ENUMERATE;
	co_mcu_enumerate.header.sequence_num     = 0; // Sequence number for callback is 0
	co_mcu_enumerate.header.return_expected  = 0;
	co_mcu_enumerate.header.authentication   = 0;
	co_mcu_enumerate.header.other_options    = 0;
	co_mcu_enumerate.header.error            = 0;
	co_mcu_enumerate.header.future_use       = 0;
	memcpy(CO_MCU_DATA(bricklet_num)->buffer_send, &co_mcu_enumerate, sizeof(CoMCUEnumerate));
	CO_MCU_DATA(bricklet_num)->buffer_send_length = sizeof(CoMCUEnumerate);

	ringbuffer_init(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, BUFFER_SIZE_RECV, CO_MCU_DATA(bricklet_num)->buffer_recv);
}

void bricklet_co_mcu_spibb_select(const uint8_t bricklet_num) {
	SPI_SS(bricklet_num).pio->PIO_CODR = SPI_SS(bricklet_num).mask;
}

void bricklet_co_mcu_spibb_deselect(const uint8_t bricklet_num) {
	SPI_SS(bricklet_num).pio->PIO_SODR = SPI_SS(bricklet_num).mask;
}

uint8_t bricklet_co_mcu_entry_spibb_transceive_byte(const uint8_t bricklet_num, const uint8_t value) {
	const uint32_t pin_clk_mask = SPI_CLK(bricklet_num).mask;
	const uint32_t pin_mosi_mask = SPI_MOSI(bricklet_num).mask;
	const uint32_t pin_miso_mask = SPI_MISO(bricklet_num).mask;
	Pio *pin_clk = SPI_CLK(bricklet_num).pio;
	Pio *pin_mosi = SPI_MOSI(bricklet_num).pio;
	Pio *pin_miso = SPI_MISO(bricklet_num).pio;

	uint8_t recv = 0;

	for(int8_t i = 7; i >= 0; i--) {
		pin_clk->PIO_CODR = pin_clk_mask;
		if((value >> i) & 1) {
			pin_mosi->PIO_SODR = pin_mosi_mask;
		} else {
			pin_mosi->PIO_CODR = pin_mosi_mask;
		}

		SLEEP_NS(SLEEP_HALF_BIT_NS); // TODO: Use TimerCounter or similar for sleep here
		if(pin_miso->PIO_PDSR & pin_miso_mask) {
			recv |= (1 << i);
		}

		pin_clk->PIO_SODR = pin_clk_mask;
		SLEEP_NS(SLEEP_HALF_BIT_NS);
	}

	return recv;
}

uint8_t bricklet_co_mcu_transceive(const uint8_t bricklet_num, const uint8_t data) {
	const uint8_t value = bricklet_co_mcu_entry_spibb_transceive_byte(bricklet_num, data);

	if(!ringbuffer_add(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, value)) {
		logw("Could not add to ringbuffer\n\r");
	}

	return value;
}

void bricklet_co_mcu_send_ack(const uint8_t bricklet_num, const uint8_t sequence_number) {
	const uint8_t sequence_number_to_send = sequence_number << 4;
	uint8_t checksum = 0;

	bricklet_co_mcu_spibb_select(bricklet_num);

	bricklet_co_mcu_transceive(bricklet_num, PROTOCOL_OVERHEAD);
	PEARSON(checksum, PROTOCOL_OVERHEAD);

	bricklet_co_mcu_transceive(bricklet_num, sequence_number_to_send);
	PEARSON(checksum, sequence_number_to_send);

	bricklet_co_mcu_transceive(bricklet_num, checksum);

	bricklet_co_mcu_spibb_deselect(bricklet_num);
}

void bricklet_co_mcu_new_message(void *data, const uint16_t length, const ComType com, const uint8_t bricklet_num) {
	// We have to inject connected UID to Enumerate and Identity messages
	uint8_t fid = ((MessageHeader*)data)->fid;
	logd("co mcu message recv: %d\n\r", fid);
	switch(fid) {
		// The relevant message content (uid and position) of Enumerate Callback
		// and GetIdentitiy is the same, so we can handle it in one case.
		case FID_ENUMERATE_CALLBACK:
		case FID_GET_IDENTITY: {
			GetIdentityReturn *gir = data;
			memset(gir->connected_uid, '\0', UID_STR_MAX_LENGTH);
			uid_to_serial_number(com_info.uid, gir->connected_uid);
			gir->position = 'a' + bricklet_num;

			bs[bricklet_num].device_identifier = gir->device_identifier;
			break;
		}
	}

	// Update UID for routing purposes
	bs[bricklet_num].uid = ((MessageHeader*)data)->uid;

	// There is a Bricklet connected to this port!
	CO_MCU_DATA(bricklet_num)->availability.access.got_message = true;

	// Send message via current com protocol.
	send_blocking_with_timeout(data, length, com);
}

void bricklet_co_mcu_handle_error(const uint8_t bricklet_num) {
	// In case of error we completely empty the ringbuffer
	uint8_t data;
	while(ringbuffer_get(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, &data));
}

uint16_t bricklet_co_mcu_check_missing_length(const uint8_t bricklet_num) {
	// Peak into the buffer to get the message length.
	// Only call this before or after bricklet_co_mcu_check_recv.
	Ringbuffer *rb = &CO_MCU_DATA(bricklet_num)->ringbuffer_recv;
	while(rb->start != rb->end) {
		if(rb->buffer[rb->start] == 0) {
			ringbuffer_remove(rb, 1);
			continue;
		}

		// TODO: Sanity check length here already?
		return rb->buffer[rb->start] - ringbuffer_get_used(rb);
	}

	return 0;
}

void bricklet_co_mcu_check_recv(const uint8_t bricklet_num) {
	uint8_t message[MAX_TFP_MESSAGE_LENGTH];
	uint16_t message_position = 0;
	uint16_t used = ringbuffer_get_used(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv);
	uint16_t start = CO_MCU_DATA(bricklet_num)->ringbuffer_recv.start;
	uint16_t num_to_remove_from_ringbuffer = 0;
	uint8_t checksum = 0;

	uint8_t data_sequence_number = 0;
	uint8_t data_length = 0;

	// We only remove data from the ringbuffer if we processed a message,
	// so the state always starts as "STATE_START".
	CoMCURecvState state = STATE_START;
	for(uint16_t i = start; i < start+used; i++) {
		const uint16_t index = i % BUFFER_SIZE_RECV;
		const uint8_t data = CO_MCU_DATA(bricklet_num)->buffer_recv[index];
		num_to_remove_from_ringbuffer++;

		switch(state) {
			case STATE_START: {
				checksum = 0;
				message_position = 0;

				if(data == PROTOCOL_OVERHEAD) {
					state = STATE_ACK_SEQUENCE_NUMBER;
				} else if(data >= MIN_TFP_MESSAGE_LENGTH && data <= MAX_TFP_MESSAGE_LENGTH) {
					state = STATE_MESSAGE_SEQUENCE_NUMBER;
				} else if(data == 0) {
					ringbuffer_remove(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, 1);
					num_to_remove_from_ringbuffer--;
					break;
				} else {
					// If the length is not PROTOCOL_OVERHEAD or within [MIN_TFP_MESSAGE_LENGTH, MAX_TFP_MESSAGE_LENGTH]
					// or 0, something has gone wrong!
					bricklet_co_mcu_handle_error(bricklet_num);
					logw("Error in STATE_START\n\r");
					return;
				}

				data_length = data;
				if((start+used - i) < data_length) {
					// There can't be enough data for a whole message, we can return here.
					return;
				}
				PEARSON(checksum, data_length);

				break;
			}

			case STATE_ACK_SEQUENCE_NUMBER: {
				data_sequence_number = data;
				PEARSON(checksum, data_sequence_number);
				state = STATE_ACK_CHECKSUM;
				break;
			}

			case STATE_ACK_CHECKSUM: {
				// Whatever happens here, we will go to start again and remove
				// data from ringbuffer
				state = STATE_START;
				ringbuffer_remove(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, num_to_remove_from_ringbuffer);
				num_to_remove_from_ringbuffer = 0;

				if(checksum != data) {
					bricklet_co_mcu_handle_error(bricklet_num);
					logw("Error in STATE_ACK_CHECKSUM\n\r");
					return;
				}

				uint8_t last_sequence_number_seen_by_slave = (data_sequence_number & 0xF0) >> 4;
				if(last_sequence_number_seen_by_slave == CO_MCU_DATA(bricklet_num)->current_sequence_number) {
					CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = -1;
					CO_MCU_DATA(bricklet_num)->buffer_send_length = 0;
				}

				break;
			}

			case STATE_MESSAGE_SEQUENCE_NUMBER: {
				data_sequence_number = data;
				PEARSON(checksum, data_sequence_number);
				state = STATE_MESSAGE_DATA;
				break;
			}

			case STATE_MESSAGE_DATA: {
				message[message_position] = data;
				message_position++;

				PEARSON(checksum, data);

				if(message_position == data_length-PROTOCOL_OVERHEAD) {
					state = STATE_MESSAGE_CHECKSUM;
				}
				break;
			}

			case STATE_MESSAGE_CHECKSUM: {
				// Whatever happens here, we will go to start again and remove
				// data from ringbuffer
				state = STATE_START;
				ringbuffer_remove(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, num_to_remove_from_ringbuffer);
				num_to_remove_from_ringbuffer = 0;

				if(checksum != data) {
					bricklet_co_mcu_handle_error(bricklet_num);
					logw("Error in STATE_MESSAGE_CHECKSUM (chk, rcv): %x != %x\n\r", checksum, data);
					return;
				}

				uint8_t last_sequence_number_seen_by_slave = (data_sequence_number & 0xF0) >> 4;
				if(last_sequence_number_seen_by_slave == CO_MCU_DATA(bricklet_num)->current_sequence_number) {
					CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = -1;
					CO_MCU_DATA(bricklet_num)->buffer_send_length = 0;
				}

				uint8_t message_sequence_number = data_sequence_number & 0x0F;
				bricklet_co_mcu_send_ack(bricklet_num, message_sequence_number);

				// If sequence number is new, we can send the message to current com.
				// Otherwise we only ack the already send message again.
				if(message_sequence_number != CO_MCU_DATA(bricklet_num)->last_sequence_number_seen) {
					CO_MCU_DATA(bricklet_num)->last_sequence_number_seen = message_sequence_number;
					bricklet_co_mcu_new_message(message, message_position, com_info.current, bricklet_num);
				}
				return;
			}
		}
	}
}

uint8_t bricklet_co_mcu_get_sequence_byte(const uint8_t bricklet_num, const bool increase) {
	if(increase) {
		CO_MCU_DATA(bricklet_num)->current_sequence_number++;
		if(CO_MCU_DATA(bricklet_num)->current_sequence_number > 0xF) {
			CO_MCU_DATA(bricklet_num)->current_sequence_number = 1;
		}
	}

	return CO_MCU_DATA(bricklet_num)->current_sequence_number | (CO_MCU_DATA(bricklet_num)->last_sequence_number_seen << 4);
}

void bricklet_co_mcu_poll(const uint8_t bricklet_num) {
	uint8_t checksum = 0;
	if(CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout > 0) {
		CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout--;
		if(CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout == 0) {
			CO_MCU_DATA(bricklet_num)->availability.access.tries++;
			if(CO_MCU_DATA(bricklet_num)->availability.access.tries >= MAX_TRIES_IF_NOT_CONNECTED && !CO_MCU_DATA(bricklet_num)->availability.access.got_message) {
				// If we have never seen a message and tried to send a message for
				// more then MAX_TRIES_IF_NOT_CONNCETED times, we assume that there is no
				// Bricklet connected and remove the message

				// For all following tries we only try once (until we really get an answer)
				CO_MCU_DATA(bricklet_num)->availability.access.tries = MAX_TRIES_IF_NOT_CONNECTED - 1;
				CO_MCU_DATA(bricklet_num)->buffer_send_length = 0;
				CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = -1;
			}
		}
	}

	bricklet_co_mcu_spibb_select(bricklet_num);

	if((CO_MCU_DATA(bricklet_num)->buffer_send_length > 0) && (CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout <= 0)) {
		const uint8_t length_to_send = CO_MCU_DATA(bricklet_num)->buffer_send_length + PROTOCOL_OVERHEAD;

		// If buffer_send_ack_timeout < 0 we received an ACK
		const uint8_t sequence_number_to_send = bricklet_co_mcu_get_sequence_byte(bricklet_num, CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout < 0);

		bricklet_co_mcu_transceive(bricklet_num, length_to_send);
		PEARSON(checksum, length_to_send);
		bricklet_co_mcu_transceive(bricklet_num, sequence_number_to_send);
		PEARSON(checksum, sequence_number_to_send);

		for(uint8_t i = 0; i < CO_MCU_DATA(bricklet_num)->buffer_send_length; i++) {
			const uint8_t data_to_send = CO_MCU_DATA(bricklet_num)->buffer_send[i];
			bricklet_co_mcu_transceive(bricklet_num, data_to_send);
			PEARSON(checksum, data_to_send);
		}

		CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = BUFFER_SEND_ACK_TIMEOUT;

		bricklet_co_mcu_transceive(bricklet_num, checksum);
	} else {
		const uint16_t free = ringbuffer_get_free(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv);
		for(uint8_t i = 0; i < free; i++) {
			bricklet_co_mcu_transceive(bricklet_num, 0);

			// If the missing length is 0 we either have a complete message or the
			// Bricklet did not have any data to send (buffer was empty and we got a 0)
			if(bricklet_co_mcu_check_missing_length(bricklet_num) == 0) {
				break;
			}
		}
	}

	bricklet_co_mcu_spibb_deselect(bricklet_num);

	bricklet_co_mcu_check_recv(bricklet_num);
}

void bricklet_co_mcu_send(const uint8_t bricklet_num, uint8_t *data, const uint8_t length) {
	if(length == 0 || length > BUFFER_SIZE_SEND) {
		// In this case something went wrong
		return;
	}

	uint32_t trials = SEND_BLOCKING_TRIALS_SPI_STACK;
	while(trials > 0) {
		if(CO_MCU_DATA(bricklet_num)->buffer_send_length == 0) {
			memcpy(CO_MCU_DATA(bricklet_num)->buffer_send, data, length);
			CO_MCU_DATA(bricklet_num)->buffer_send_length = length;
			CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = -1;
			bricklet_co_mcu_poll(bricklet_num);
			break;
		}
		taskYIELD();
		trials--;
	}

}
