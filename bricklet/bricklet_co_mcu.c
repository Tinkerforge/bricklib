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
#include "bricklib/utility/system_timer.h"

#include "bricklib/drivers/adc/adc.h"

#include "bricklet_config.h"
#include "config.h"

#include <string.h>

uint32_t bricklet_spitfp_baudrate[BRICKLET_NUM] = {
	#if BRICKLET_NUM > 0
		CO_MCU_DEFAULT_BAUDRATE,
	#endif
	#if BRICKLET_NUM > 1
		CO_MCU_DEFAULT_BAUDRATE,
	#endif
	#if BRICKLET_NUM > 2
		CO_MCU_DEFAULT_BAUDRATE,
	#endif
	#if BRICKLET_NUM > 3
		CO_MCU_DEFAULT_BAUDRATE
	#endif
};

uint32_t bricklet_spitfp_baudrate_current = CO_MCU_DEFAULT_BAUDRATE;
uint32_t bricklet_spitfp_minimum_dynamic_baudrate = CO_MCU_MINIMUM_BAUDRATE;
bool bricklet_spitfp_dynamic_baudrate_enabled = true;

#define BRICKLET_LED_STRIP_DEVICE_IDENTIFIER 231

#define BUFFER_SEND_ACK_TIMEOUT 20 // in ms
#define MAX_TRIES_IF_NOT_CONNECTED 100

#define PROTOCOL_OVERHEAD 3 // 3 byte overhead for Brick <-> Bricklet protocol
#define MIN_TFP_MESSAGE_LENGTH (8 + PROTOCOL_OVERHEAD)
#define MAX_TFP_MESSAGE_LENGTH (80 + PROTOCOL_OVERHEAD)
#define SLEEP_HALF_BIT_NS(baudrate) ((1000*1000*1000/2)/(baudrate) - 200) // We subtract 200ns for setting of pins etc

extern BrickletSettings bs[BRICKLET_NUM];
extern uint32_t bc[BRICKLET_NUM][BRICKLET_CONTEXT_MAX_SIZE/4];
extern ComInfo com_info;
extern uint8_t bricklet_attached[BRICKLET_NUM];
extern const BrickletAddress baddr[BRICKLET_NUM];

#define SPI_SS(i)   (bs[i].pin1_ad)
#define SPI_CLK(i)  (bs[i].pin2_da)
#define SPI_MOSI(i) (bs[i].pin3_pwm)
#define SPI_MISO(i) (bs[i].pin4_io)

#define BRICKLET_COMCU_RESET_WAIT 500 // ms

static uint32_t bricklet_comcu_reset_wait_time[BRICKLET_NUM] = {
#if BRICKLET_NUM > 0
	0,
#endif
#if BRICKLET_NUM > 1
	0,
#endif
#if BRICKLET_NUM > 2
	0,
#endif
#if BRICKLET_NUM > 3
	0
#endif
};
static uint32_t bricklet_comcu_data_last_time = 0;
static uint32_t bricklet_comcu_data_counter   = 0;

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

	memset(CO_MCU_DATA(bricklet_num)->buffer_send, 0, CO_MCU_BUFFER_SIZE_SEND);
	memset(CO_MCU_DATA(bricklet_num)->buffer_recv, 0, CO_MCU_BUFFER_SIZE_RECV);
	CO_MCU_DATA(bricklet_num)->availability.access.got_message = false;
	CO_MCU_DATA(bricklet_num)->availability.access.tries = 0;
	CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout   = -1; // Try first send immediately
	CO_MCU_DATA(bricklet_num)->current_sequence_number   = 0; // Initialize sequence number as 0, so the first one will be written as 1.
	CO_MCU_DATA(bricklet_num)->last_sequence_number_seen = 0;
	CO_MCU_DATA(bricklet_num)->first_enumerate_send      = false;

	CO_MCU_DATA(bricklet_num)->error_count.error_count_ack_checksum     = 0;
	CO_MCU_DATA(bricklet_num)->error_count.error_count_message_checksum = 0;
	CO_MCU_DATA(bricklet_num)->error_count.error_count_frame            = 0;

	CO_MCU_DATA(bricklet_num)->buffer_send_length = 0;

	ringbuffer_init(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, CO_MCU_BUFFER_SIZE_RECV, CO_MCU_DATA(bricklet_num)->buffer_recv);

	bricklet_comcu_data_last_time = system_timer_get_ms();
	bricklet_comcu_data_counter   = 0;
}

void bricklet_co_mcu_spibb_select(const uint8_t bricklet_num) {
	SPI_SS(bricklet_num).pio->PIO_CODR = SPI_SS(bricklet_num).mask;
}

void bricklet_co_mcu_spibb_deselect(const uint8_t bricklet_num) {
	SPI_SS(bricklet_num).pio->PIO_SODR = SPI_SS(bricklet_num).mask;
}

uint8_t  bricklet_co_mcu_entry_spibb_transceive_byte(const uint8_t bricklet_num, const uint8_t value) {
	const uint32_t pin_clk_mask = SPI_CLK(bricklet_num).mask;
	const uint32_t pin_mosi_mask = SPI_MOSI(bricklet_num).mask;
	const uint32_t pin_miso_mask = SPI_MISO(bricklet_num).mask;
	Pio *pin_clk = SPI_CLK(bricklet_num).pio;
	Pio *pin_mosi = SPI_MOSI(bricklet_num).pio;
	Pio *pin_miso = SPI_MISO(bricklet_num).pio;

	const uint32_t baudrate = MIN(bricklet_spitfp_baudrate[bricklet_num], bricklet_spitfp_baudrate_current);
	const uint32_t sleep_half_bit_ns = SLEEP_HALF_BIT_NS(baudrate);

	uint8_t recv = 0;

	for(int8_t i = 7; i >= 1; i--) { // Go through all but the last bit
		if((value >> i) & 1) {
			pin_clk->PIO_CODR = pin_clk_mask;
			pin_mosi->PIO_SODR = pin_mosi_mask;
		} else {
			pin_clk->PIO_CODR = pin_clk_mask;
			pin_mosi->PIO_CODR = pin_mosi_mask;
		}

		SLEEP_NS(sleep_half_bit_ns);
		pin_clk->PIO_SODR = pin_clk_mask;
		if(pin_miso->PIO_PDSR & pin_miso_mask) {
			recv |= (1 << i);
		}

		SLEEP_NS(sleep_half_bit_ns);
	}

	// We unroll the last element of the loop and remove
	// the last sleep. The code that runs between each byte
	// takes enough time for a half bit if communication
	// if speed is >= 400khz
	if((value >> 0) & 1) {
		pin_clk->PIO_CODR = pin_clk_mask;
		pin_mosi->PIO_SODR = pin_mosi_mask;
	} else {
		pin_clk->PIO_CODR = pin_clk_mask;
		pin_mosi->PIO_CODR = pin_mosi_mask;
	}

	SLEEP_NS(sleep_half_bit_ns);
	pin_clk->PIO_SODR = pin_clk_mask;
	if(pin_miso->PIO_PDSR & pin_miso_mask) {
		recv |= (1 << 0);
	}

	return recv;
}


uint8_t bricklet_co_mcu_transceive(const uint8_t bricklet_num, const uint8_t data) {
	const uint8_t value = bricklet_co_mcu_entry_spibb_transceive_byte(bricklet_num, data);
	bricklet_comcu_data_counter++;

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

	switch(fid) {
		// The relevant message content (uid and position) of Enumerate Callback
		// and GetIdentitiy is the same, so we can handle it in one case.
		case FID_ENUMERATE_CALLBACK:
		case FID_GET_IDENTITY: {
			GetIdentityReturn *gir = data;
			// If the device is an Isolator Bricklet (ID 2122) we update
			// the isolator uid and ignore the device indentifier.
			if(gir->device_identifier == 2122) {
				bs[bricklet_num].uid_isolator = ((MessageHeader*)data)->uid;
			} else {
				bs[bricklet_num].device_identifier = gir->device_identifier;
			}

			// If the Bricklet is connected to an isolator we don't have to
			// update the position and the connected UID. this is already
			// done by the isolator itself.
			if(gir->position != 'Z') {
				memset(gir->connected_uid, '\0', UID_STR_MAX_LENGTH);
				uid_to_serial_number(com_info.uid, gir->connected_uid);
				gir->position = 'a' + bricklet_num;
			}

			break;
		}
	}

	// Update UID for routing purposes, if the message is from an
	// Isolator Bricklet we ignore it (isolator UID is only updated
	// with enumerate/identity messages)
	if(((MessageHeader*)data)->uid != bs[bricklet_num].uid_isolator) {
		bs[bricklet_num].uid = ((MessageHeader*)data)->uid;
	}

	// There is a Bricklet connected to this port!
	CO_MCU_DATA(bricklet_num)->availability.access.got_message = true;

	// Send message via current com protocol.
	if(CO_MCU_DATA(bricklet_num)->first_enumerate_send) {
		send_blocking_with_timeout(data, length, com);
	} else {
		// We make sure that the very first enumerate is definitely send.
		// With this we can make sure that the Master of a stack can
		// always creates a proper routing table
		send_blocking(data, length, com);
		CO_MCU_DATA(bricklet_num)->first_enumerate_send = true;
	}
}

void bricklet_co_mcu_handle_error(const uint8_t bricklet_num) {
	// Randomly remove a byte from slave buffer.
	// If slave and master get out-of-sync and the out-of-sync
	// data is completely uniform and by chance the emptying
	// of a ringbuffer runs in a cycle, we may end up in a
	// deadlock situation. By randomly removing one additional
	// byte we get out of the deadlock.
	if(adc_get_temperature() % 2) {
		bricklet_co_mcu_entry_spibb_transceive_byte(bricklet_num, 0);
	}

	// In case of error we completely empty the ringbuffer
	uint8_t data;
	while(ringbuffer_get(&CO_MCU_DATA(bricklet_num)->ringbuffer_recv, &data));
}

uint16_t bricklet_co_mcu_check_missing_length(const uint8_t bricklet_num) {
	// Peak into the buffer to get the message length.
	// Only call this before or after bricklet_co_mcu_check_recv.
	Ringbuffer *rb = &CO_MCU_DATA(bricklet_num)->ringbuffer_recv;
	while(rb->start != rb->end) {
		uint8_t length = rb->buffer[rb->start];
		if((length < MIN_TFP_MESSAGE_LENGTH || length > MAX_TFP_MESSAGE_LENGTH) && length != PROTOCOL_OVERHEAD) {
			if(length != 0) {
				CO_MCU_DATA(bricklet_num)->error_count.error_count_frame++;
			}
			ringbuffer_remove(rb, 1);
			continue;
		}

		int32_t ret = length - ringbuffer_get_used(rb);
		if((ret < 0) || (ret > 80)) {
			return 0;
		}

		return ret;
	}

	return 0;
}

// This function is called after data was send successfully
void bricklet_co_mcu_check_reset(const uint8_t bricklet_num) {
	// Check if we actually send data
	if(CO_MCU_DATA(bricklet_num)->buffer_send_length != 0) {
		// and the request was a reset
		if(CO_MCU_DATA(bricklet_num)->buffer_send[MESSAGE_HEADER_FID_POSITION] == FID_RESET) {
			// Just to be sure we don't accidentially do this two times we overwrite the FID.
			CO_MCU_DATA(bricklet_num)->buffer_send[MESSAGE_HEADER_FID_POSITION] = 0;

			// Start reset wait timer.
			// We don't want to speak to the Bricklet during the reset, since the
			// XMC MCUs start in a bootloader-mode at the beginning and we may otherwise
			// confuse the bootloader if we speak SPI with it (it expects UART).
			bricklet_comcu_reset_wait_time[bricklet_num] = system_timer_get_ms();

			// Set sequence number back to 0, since the slave will now start at 1 again.
			// Otherwise we may accidentally be at 1 and throw the first message awai.
			CO_MCU_DATA(bricklet_num)->last_sequence_number_seen = 0;
		}
	}
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
		const uint16_t index = i % CO_MCU_BUFFER_SIZE_RECV;
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
					CO_MCU_DATA(bricklet_num)->error_count.error_count_frame++;
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
					CO_MCU_DATA(bricklet_num)->error_count.error_count_ack_checksum++;
					bricklet_co_mcu_handle_error(bricklet_num);
					logw("Error in STATE_ACK_CHECKSUM\n\r");
					return;
				}

				uint8_t last_sequence_number_seen_by_slave = (data_sequence_number & 0xF0) >> 4;
				if(last_sequence_number_seen_by_slave == CO_MCU_DATA(bricklet_num)->current_sequence_number) {
					bricklet_co_mcu_check_reset(bricklet_num);
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
					CO_MCU_DATA(bricklet_num)->error_count.error_count_message_checksum++;
					bricklet_co_mcu_handle_error(bricklet_num);
					logw("Error in STATE_MESSAGE_CHECKSUM (chk, rcv): %x != %x\n\r", checksum, data);
					return;
				}

				uint8_t last_sequence_number_seen_by_slave = (data_sequence_number & 0xF0) >> 4;
				if(last_sequence_number_seen_by_slave == CO_MCU_DATA(bricklet_num)->current_sequence_number) {
					bricklet_co_mcu_check_reset(bricklet_num);
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
			CO_MCU_DATA(bricklet_num)->current_sequence_number = 2;
		}
	}

	return CO_MCU_DATA(bricklet_num)->current_sequence_number | (CO_MCU_DATA(bricklet_num)->last_sequence_number_seen << 4);
}

bool bricklet_co_mcu_check_led_strip(const uint8_t bricklet_num) {
	if(false
#if BRICKLET_NUM > 0
	   || (bs[0].device_identifier == BRICKLET_LED_STRIP_DEVICE_IDENTIFIER)
#endif
#if BRICKLET_NUM > 1
	   || (bs[1].device_identifier == BRICKLET_LED_STRIP_DEVICE_IDENTIFIER)
#endif
#if BRICKLET_NUM > 2
	   || (bs[2].device_identifier == BRICKLET_LED_STRIP_DEVICE_IDENTIFIER)
#endif
#if BRICKLET_NUM > 3
	   || (bs[3].device_identifier == BRICKLET_LED_STRIP_DEVICE_IDENTIFIER)
#endif
	   ) {

		// If there is an LED Strip Bricklet we do not poll
		// this port anymore for a co mcu Bricklet and we reinitialize
		// the LED Strip Bricklet
		bricklet_attached[bricklet_num] = BRICKLET_INIT_NO_BRICKLET;
		bs[bricklet_num].device_identifier = 0;

		for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
			if(bs[i].device_identifier == BRICKLET_LED_STRIP_DEVICE_IDENTIFIER) {
				baddr[i].entry(BRICKLET_TYPE_CONSTRUCTOR, 0, NULL);
			}
		}
		return true;
	}

	return false;
}

void bricklet_co_mcu_update_speed(void) {
	if(system_timer_is_time_elapsed_ms(bricklet_comcu_data_last_time, 100)) {
		bricklet_comcu_data_last_time = system_timer_get_ms();
		const uint32_t counter = bricklet_comcu_data_counter;
		bricklet_comcu_data_counter = 0;

		if(bricklet_spitfp_dynamic_baudrate_enabled) {

			uint32_t new_baudrate = 0;

			if(counter <= 800) {
				new_baudrate = bricklet_spitfp_minimum_dynamic_baudrate;
			} else if(counter >= 2000) {
				new_baudrate = CO_MCU_MAXIMUM_BAUDRATE;
			} else {
				new_baudrate = SCALE(counter, 800, 2000, bricklet_spitfp_minimum_dynamic_baudrate, CO_MCU_MAXIMUM_BAUDRATE);
			}

			if(new_baudrate >= bricklet_spitfp_baudrate_current) {
				bricklet_spitfp_baudrate_current = new_baudrate;
			} else {
				bricklet_spitfp_baudrate_current -= 10000;
				bricklet_spitfp_baudrate_current = MAX(bricklet_spitfp_baudrate_current, new_baudrate);
			}
		} else {
			bricklet_spitfp_baudrate_current = CO_MCU_MAXIMUM_BAUDRATE;
		}
	}
}

void bricklet_co_mcu_poll(const uint8_t bricklet_num) {
	if(com_info.current == COM_NONE) {
		// Never communicate with the Bricklet if we don't know were to send
		// the data!
		return;
	}

	if(bricklet_comcu_reset_wait_time[bricklet_num] != 0) {
		if(system_timer_is_time_elapsed_ms(bricklet_comcu_reset_wait_time[bricklet_num], BRICKLET_COMCU_RESET_WAIT)) {
			bricklet_comcu_reset_wait_time[bricklet_num] = 0;
		} else {
			return;
		}
	}

	uint8_t checksum = 0;
	if(CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout > 0) {
		CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout--;
		if(CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout == 0) {
			CO_MCU_DATA(bricklet_num)->availability.access.tries++;
			if(CO_MCU_DATA(bricklet_num)->availability.access.tries >= MAX_TRIES_IF_NOT_CONNECTED && !CO_MCU_DATA(bricklet_num)->availability.access.got_message) {
				// If we have never seen a message and tried to send a message for
				// more then MAX_TRIES_IF_NOT_CONNCETED times, we assume that there is no
				// Bricklet connected and remove the message

				// HACK for LED Strip Bricklet: If we reached the first timeout and there is a
				//                              LED Strip Bricklet present, we return here and
				//                              don't check again. In this case the BC RAM can
				//                              then be used by the LED Strip Bricklet.
				if(bricklet_co_mcu_check_led_strip(bricklet_num)) {
					return;
				}

				// For all following tries we only try once (until we really get an answer)
				CO_MCU_DATA(bricklet_num)->availability.access.tries = MAX_TRIES_IF_NOT_CONNECTED - 1;
				CO_MCU_DATA(bricklet_num)->buffer_send_length = 0;
				CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = -1;
			}
		}
	}

	bricklet_co_mcu_update_speed();
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
	if(length == 0 || length > CO_MCU_BUFFER_SIZE_SEND) {
		// In this case something went wrong
		return;
	}

	// CoMCU Bricklets need to ignore stack enumerate message
	if(data[MESSAGE_HEADER_FID_POSITION] == FID_STACK_ENUMERATE) {
		return;
	}

	uint32_t start_time = system_timer_get_ms();
	while(!system_timer_is_time_elapsed_ms(start_time, SEND_BLOCKING_TIMEOUT_SPI_STACK)) {
		if(CO_MCU_DATA(bricklet_num)->buffer_send_length == 0) {
			memcpy(CO_MCU_DATA(bricklet_num)->buffer_send, data, length);
			CO_MCU_DATA(bricklet_num)->buffer_send_length = length;
			CO_MCU_DATA(bricklet_num)->buffer_send_ack_timeout = -1;

			if(CO_MCU_DATA(bricklet_num)->buffer_send[MESSAGE_HEADER_FID_POSITION] == FID_CREATE_ENUMERATE_CONNECTED) {
				CO_MCU_DATA(bricklet_num)->buffer_send[MESSAGE_HEADER_FID_POSITION] = FID_CO_MCU_ENUMERATE;
				logd("New enumerate connected request (comcu): %d\n\r", bricklet_num);
			}

			bricklet_co_mcu_poll(bricklet_num);
			break;
		}
		taskYIELD();
	}
}
