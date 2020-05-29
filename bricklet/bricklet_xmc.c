/* bricklib
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_xmc.c: Functions to write bootloader/bootstrapper to XMC1xxx
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

#include "bricklet_xmc.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/system_timer.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/wdt/wdt.h"
#include "bricklib/utility/syscalls.h"

#define BRICKLET_XMC_BSL_START  0x00
#define BRICKLET_XMC_BSL_ASC_F  0x6C
#define BRICKLET_XMC_BSL_ASC_H  0x12
#define BRICKLET_XMC_BSL_ENC_F  0x93
#define BRICKLET_XMC_BSL_ENC_H  0xED

#define BRICKLET_XMC_BSL_BR_OK  0xF0
#define BRICKLET_XMC_BSL_ID     0x5D
#define BRICKLET_XMC_BSL_ENC_ID 0xA2
#define BRICKLET_XMC_BSL_BR_OK  0xF0
#define BRICKLET_XMC_BSL_OK     0x01
#define BRICKLET_XMC_BSL_NOK    0x02

#define BRICKLET_XMC_BOOTLOADER_MAX_SIZE (1024*8)
#define BRICKLET_XMC_BOOTSTRAPPER_MAX_SIZE 1024

#define BRICKLET_XMC_BOOTLOADER_CHUNK_SIZE 256
#define BRICKLET_XMC_CHUNK_SIZE 64

#define BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS 64000
#define BRICKLET_XMC_UARTBB_BIT_TIME 640 // 64000000/100000

bool     bricklet_xmc_use_half_duplex = true;

// Bootloader RAM is allocated on very first call starting from the end of the heap
uint8_t  *bricklet_xmc_bootloader = NULL; // size BRICKLET_XMC_BOOTLOADER_MAX_SIZE

// Steal bootstrapper RAM from WIFI Extension
extern char wifi_ringbuffer[];
uint8_t  *bricklet_xmc_bootstrapper = (uint8_t*)&wifi_ringbuffer[0]; // size BRICKLET_XMC_BOOTSTRAPPER_MAX_SIZE

uint32_t bricklet_xmc_bootloader_length   = 1024*8;
uint32_t bricklet_xmc_bootstrapper_length = 1024;

uint32_t bricklet_xmc_bootstrapper_address_recv = 0;
uint32_t bricklet_xmc_bootloader_address_recv   = 0;

bool     bricklet_xmc_do_comcu_tick = true;

uint32_t bricklet_xmc_current_boot_type = 0;
Pin      bricklet_xmc_pin_out  = BRICKLET_D_PIN_2_DA_30;
Pin      bricklet_xmc_pin_in   = BRICKLET_D_PIN_3_PWM;

extern caddr_t syscalls_heap;

static inline void bricklet_xmc_uart_wait_05bit(uint32_t start) {
	while(true) {
		int32_t current = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;
		int32_t result = current - start;
		if(result < 0) {
			result += BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS;
		}

		if(result >= BRICKLET_XMC_UARTBB_BIT_TIME/2) {
			break;
		}
	}
}

static inline void bricklet_xmc_uart_wait_1bit(uint32_t start) {
	while(true) {
		int32_t current = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;
		int32_t result = current - start;
		if(result < 0) {
			result += BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS;
		}

		if(result >= BRICKLET_XMC_UARTBB_BIT_TIME) {
			break;
		}
	}
}

void bricklet_xmc_uart_fd_write(uint8_t value) {
	uint16_t value16 = 0 | (value << 1) | 1 << 9;

	uint32_t start;
	uint8_t bit_count = 10;

	start = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;

	do {
		if(value16 & 1) {
			bricklet_xmc_pin_out.pio->PIO_SODR = bricklet_xmc_pin_out.mask;
		} else {
			bricklet_xmc_pin_out.pio->PIO_CODR = bricklet_xmc_pin_out.mask;
		}

		bricklet_xmc_uart_wait_1bit(start);
		start += BRICKLET_XMC_UARTBB_BIT_TIME;

		value16 >>= 1;
	} while (--bit_count);

	bricklet_xmc_uart_wait_05bit(start);
}

uint8_t bricklet_xmc_uart_fd_read_first(void) {
	bool bit_value = bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask;

	uint32_t start = system_timer_get_ms();
	while(!bit_value && !system_timer_is_time_elapsed_ms(start, 1)) {
		bit_value = bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask;
	}
	if(!bit_value) {
		return 0;
	}

	start = system_timer_get_ms();
	while(bit_value && !system_timer_is_time_elapsed_ms(start, 1)) {
		bit_value = bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask;
	}
	if(bit_value) {
		return 0;
	}

	start = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;
	bricklet_xmc_uart_wait_05bit(start);
	start += BRICKLET_XMC_UARTBB_BIT_TIME/2;

	uint16_t uart_value = 0;
	for(uint8_t i = 0; i < 9; i++) {
		if(bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask) {
			uart_value |= 1 << i;
		}
		if(i < 8) {
			bricklet_xmc_uart_wait_1bit(start);
			start += BRICKLET_XMC_UARTBB_BIT_TIME;
		}
	}

	// remove start bit
	return uart_value >> 1;
}

uint8_t bricklet_xmc_uart_fd_read(void) {
	bool bit_value = bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask;
	uint32_t start = system_timer_get_ms();
	while(bit_value && !system_timer_is_time_elapsed_ms(start, 50)) {
		bit_value = bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask;
	}
	if(bit_value) {
		return 0;
	}

	start = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;
	bricklet_xmc_uart_wait_05bit(start);
	start += BRICKLET_XMC_UARTBB_BIT_TIME/2;

	uint16_t uart_value = 0;
	for(uint8_t i = 0; i < 9; i++) {
		if(bricklet_xmc_pin_in.pio->PIO_PDSR & bricklet_xmc_pin_in.mask) {
			uart_value |= 1 << i;
		}
		if(i < 8) {
			bricklet_xmc_uart_wait_1bit(start);
			start += BRICKLET_XMC_UARTBB_BIT_TIME;
		}
	}

	// remove start bit
	return uart_value >> 1;
}


void bricklet_xmc_uart_hd_write(uint8_t value, bool after_read) {
	uint16_t value16 = 0 | (value << 1) | 1 << 9;

	uint32_t start;
	uint8_t bit_count = 10;

	if(after_read) {
		SLEEP_US(100);
		bricklet_xmc_pin_out.type = PIO_OUTPUT_0;
		bricklet_xmc_pin_out.attribute = PIO_DEFAULT;
		PIO_Configure(&bricklet_xmc_pin_out, 1);
	}

	start = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;

	do {
		if(value16 & 1) {
			bricklet_xmc_pin_out.pio->PIO_SODR = bricklet_xmc_pin_out.mask;
		} else {
			bricklet_xmc_pin_out.pio->PIO_CODR = bricklet_xmc_pin_out.mask;
		}

		bricklet_xmc_uart_wait_1bit(start);
		start += BRICKLET_XMC_UARTBB_BIT_TIME;

		value16 >>= 1;
	} while (--bit_count);

	bricklet_xmc_uart_wait_05bit(start);
}

uint8_t bricklet_xmc_uart_hd_read(void) {
	bricklet_xmc_pin_out.type = PIO_INPUT;
	bricklet_xmc_pin_out.attribute = PIO_PULLUP;
	PIO_Configure(&bricklet_xmc_pin_out, 1);

	bool bit_value = bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask;

	uint32_t start = system_timer_get_ms();
	while(bit_value && !system_timer_is_time_elapsed_ms(start, 1)) {
		bit_value = bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask;
	}
	if(bit_value) {
		return 0;
	}

	start = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;
	bricklet_xmc_uart_wait_05bit(start);
	start += BRICKLET_XMC_UARTBB_BIT_TIME/2;

	uint16_t uart_value = 0;
	for(uint8_t i = 0; i < 9; i++) {
		if(bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask) {
			uart_value |= 1 << i;
		}

		if(i < 8) {
			bricklet_xmc_uart_wait_1bit(start);
			start += BRICKLET_XMC_UARTBB_BIT_TIME;
		}
	}

	// remove start bit
	return uart_value >> 1;
}

uint8_t bricklet_xmc_uart_hd_read_first(void) {
	bricklet_xmc_pin_out.type = PIO_INPUT;
	bricklet_xmc_pin_out.attribute = PIO_PULLUP;
	PIO_Configure(&bricklet_xmc_pin_out, 1);

	bool bit_value = bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask;

//#if 0
	uint32_t start = system_timer_get_ms();
	if(!bit_value) {
		while(!bit_value && !system_timer_is_time_elapsed_ms(start, 1)) {
			bit_value = bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask;
		}
		if(!bit_value) {
			return 0;
		}
	}
//#endif

	start = system_timer_get_ms();
	while(bit_value && !system_timer_is_time_elapsed_ms(start, 1)) {
		bit_value = bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask;
	}
	if(bit_value) {
		return 0;
	}

	start = BRICKLET_XMC_UARTBB_COUNT_TO_IN_1MS - SysTick->VAL;
	bricklet_xmc_uart_wait_05bit(start);
	start += BRICKLET_XMC_UARTBB_BIT_TIME/2;

	uint16_t uart_value = 0;
	for(uint8_t i = 0; i < 9; i++) {
		if(bricklet_xmc_pin_out.pio->PIO_PDSR & bricklet_xmc_pin_out.mask) {
			uart_value |= 1 << i;
		}

		if(i < 8) {
			bricklet_xmc_uart_wait_1bit(start);
			start += BRICKLET_XMC_UARTBB_BIT_TIME;
		}
	}

	// remove start bit
	return uart_value >> 1;
}

uint32_t bricklet_xmc_write_bootstrapper(void) {
	wdt_restart();
	bricklet_xmc_pin_out.type = PIO_OUTPUT_1;
	bricklet_xmc_pin_out.attribute = PIO_DEFAULT;
	PIO_Configure(&bricklet_xmc_pin_out, 1);

	uint8_t value = 0;
	if(bricklet_xmc_use_half_duplex) {
		bricklet_xmc_uart_hd_write(BRICKLET_XMC_BSL_START, false);
		bricklet_xmc_uart_hd_write(BRICKLET_XMC_BSL_ASC_H, false);
		value = bricklet_xmc_uart_hd_read_first();
	} else {
		bricklet_xmc_uart_fd_write(BRICKLET_XMC_BSL_START);
		bricklet_xmc_uart_fd_write(BRICKLET_XMC_BSL_ASC_F);
		value = bricklet_xmc_uart_fd_read_first();
	}
	if(value != BRICKLET_XMC_BSL_ID) { // Test if handshake is OK
		return 4;
	}

	if(bricklet_xmc_use_half_duplex) {
		bricklet_xmc_uart_hd_write((bricklet_xmc_bootstrapper_length >>  0) & 0xFF, true);
		bricklet_xmc_uart_hd_write((bricklet_xmc_bootstrapper_length >>  8) & 0xFF, false);
		bricklet_xmc_uart_hd_write((bricklet_xmc_bootstrapper_length >> 16) & 0xFF, false);
		bricklet_xmc_uart_hd_write((bricklet_xmc_bootstrapper_length >> 24) & 0xFF, false);

		value = bricklet_xmc_uart_hd_read();
	} else {
		bricklet_xmc_uart_fd_write((bricklet_xmc_bootstrapper_length >>  0) & 0xFF);
		bricklet_xmc_uart_fd_write((bricklet_xmc_bootstrapper_length >>  8) & 0xFF);
		bricklet_xmc_uart_fd_write((bricklet_xmc_bootstrapper_length >> 16) & 0xFF);
		bricklet_xmc_uart_fd_write((bricklet_xmc_bootstrapper_length >> 24) & 0xFF);

		value = bricklet_xmc_uart_fd_read();
	}

	if(value != BRICKLET_XMC_BSL_OK && value != 3) { // Test if length was received
		return 5;
	}

	if(bricklet_xmc_use_half_duplex) {
		bricklet_xmc_uart_hd_write(bricklet_xmc_bootstrapper[0], true);
		for(uint32_t i = 1; i < bricklet_xmc_bootstrapper_length; i++) {
			bricklet_xmc_uart_hd_write(bricklet_xmc_bootstrapper[i], false);
		}

		value = bricklet_xmc_uart_hd_read();
	} else {
		bricklet_xmc_uart_fd_write(bricklet_xmc_bootstrapper[0]);
		for(uint32_t i = 1; i < bricklet_xmc_bootstrapper_length; i++) {
			bricklet_xmc_uart_fd_write(bricklet_xmc_bootstrapper[i]);
		}

		value = bricklet_xmc_uart_fd_read();
	}
	if(value != BRICKLET_XMC_BSL_OK) { // Test if bootstrapper was received
		return 6;
	}

	return 0;
}

uint32_t bricklet_xmc_write_bootloader(void) {
	wdt_restart();

	bricklet_xmc_pin_out.type = PIO_OUTPUT_1;
	bricklet_xmc_pin_out.attribute = PIO_DEFAULT;
	PIO_Configure(&bricklet_xmc_pin_out, 1);

	bricklet_xmc_pin_in.type = PIO_INPUT;
	bricklet_xmc_pin_in.attribute = PIO_DEFAULT;
	PIO_Configure(&bricklet_xmc_pin_in, 1);

	// Wait for bootstrapper to be ready
	SLEEP_US(200);

	for(uint32_t chunk = 0; chunk < BRICKLET_XMC_BOOTLOADER_MAX_SIZE/BRICKLET_XMC_BOOTLOADER_CHUNK_SIZE; chunk++) {
		uint8_t crc_calc = 0;
		for(uint32_t i = 0; i < BRICKLET_XMC_BOOTLOADER_CHUNK_SIZE; i++) {
			const uint8_t value = bricklet_xmc_bootloader[BRICKLET_XMC_BOOTLOADER_CHUNK_SIZE*chunk + i];
			bricklet_xmc_uart_fd_write(value);
			crc_calc ^= value;
		}

		SLEEP_US(25);
		uint8_t crc_read = bricklet_xmc_uart_fd_read();

		if(crc_read != crc_calc) {
			return 7;
		}
	}

	return 0;
}

void bricklet_xmc_set_port_d_input(void) {
	Pin pin1 = BRICKLET_D_PIN_1_AD_30;
	Pin pin2 = BRICKLET_D_PIN_2_DA_30;
	Pin pin3 = BRICKLET_D_PIN_3_PWM;
	Pin pin4 = BRICKLET_D_PIN_4_IO;

	pin1.type = PIO_INPUT;
	pin1.attribute = PIO_DEFAULT;
	PIO_Configure(&pin1, 1);

	pin2.type = PIO_INPUT;
	pin2.attribute = PIO_PULLUP;
	PIO_Configure(&pin2, 1);

	pin3.type = PIO_INPUT;
	pin3.attribute = PIO_DEFAULT;
	PIO_Configure(&pin3, 1);

	pin4.type = PIO_INPUT;
	pin4.attribute = PIO_DEFAULT;
	PIO_Configure(&pin4, 1);
}

uint32_t bricklet_xmc_flash_config(uint32_t config, uint32_t parameter1, uint32_t parameter2, const uint8_t *data) {
	if(bricklet_xmc_bootloader == NULL) {
		bricklet_xmc_bootloader = (uint8_t*)syscalls_heap;
	}

	bricklet_xmc_use_half_duplex = parameter2;

	uint32_t ret = 0; // OK

	switch(config) {
		case 0: { // Start Bootstrapper Write
			bricklet_xmc_set_port_d_input();
			bricklet_xmc_bootstrapper_address_recv = 0;
			bricklet_xmc_bootstrapper_length  = parameter1;
			bricklet_xmc_current_boot_type = 0;
			bricklet_xmc_do_comcu_tick = false;

			ret = 0; // OK
			break;
		}

		case 1: { // Start Bootloader Write
			bricklet_xmc_set_port_d_input();
			bricklet_xmc_bootloader_address_recv = 0;
			bricklet_xmc_bootloader_length = parameter1;
			bricklet_xmc_current_boot_type = 1;
			bricklet_xmc_do_comcu_tick = false;

			ret = 0; // OK
			break;
		}

		case 2: { // Flash to Bricklet
			if(bricklet_xmc_bootstrapper_address_recv < bricklet_xmc_bootstrapper_length) {
				ret = 2; // Error
				break;
			}
			if(bricklet_xmc_bootloader_address_recv < bricklet_xmc_bootloader_length) {
				ret = 3; // Error
				break;
			}

			ret = bricklet_xmc_write_bootstrapper();
			bricklet_xmc_pin_out.type = PIO_OUTPUT_1;
			bricklet_xmc_pin_out.attribute = PIO_DEFAULT;
			PIO_Configure(&bricklet_xmc_pin_out, 1);

			if(ret == 0) {
				ret = bricklet_xmc_write_bootloader();
			}
			break;
		}

		default: {
			ret = 1; // Error

			break;
		}
	}

	if(ret != 0) {
		bricklet_xmc_set_port_d_input();
	}

	return ret;
}

uint32_t bricklet_xmc_flash_data(const uint8_t *data) {
	if(bricklet_xmc_bootloader == NULL) {
		bricklet_xmc_bootloader = (uint8_t*)syscalls_heap;
	}

	uint32_t ret = 0; // OK

	if(bricklet_xmc_current_boot_type == 0) {
		if(bricklet_xmc_bootstrapper_address_recv + BRICKLET_XMC_CHUNK_SIZE > BRICKLET_XMC_BOOTSTRAPPER_MAX_SIZE) {
			ret = 1; // Error
		} else {
			memcpy(bricklet_xmc_bootstrapper + bricklet_xmc_bootstrapper_address_recv, data, BRICKLET_XMC_CHUNK_SIZE);
			bricklet_xmc_bootstrapper_address_recv += BRICKLET_XMC_CHUNK_SIZE;
		}
	} else if(bricklet_xmc_current_boot_type == 1) {
		if(bricklet_xmc_bootloader_address_recv + BRICKLET_XMC_CHUNK_SIZE > BRICKLET_XMC_BOOTLOADER_MAX_SIZE) {
			ret = 2; // Error
		} else {
			memcpy(bricklet_xmc_bootloader + bricklet_xmc_bootloader_address_recv, data, BRICKLET_XMC_CHUNK_SIZE);
			bricklet_xmc_bootloader_address_recv += BRICKLET_XMC_CHUNK_SIZE;
		}
	} else {
		ret = 3; // Error
	}

	return ret;
}
