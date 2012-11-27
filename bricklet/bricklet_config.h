/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_config.h: configurations for bricklets
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

#ifndef BRICKLET_CONFIG_H
#define BRICKLET_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "bricklet_init.h"
#include "bricklib/com/com_messages.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/twi/twid.h"

#include "bricklib/logging/logging.h"
#include "bricklib/utility/mutex.h"

typedef int (*BrickletEntryFunction)(const uint8_t, const uint8_t, uint8_t*);

typedef struct {
	ComType *com_current;
	signed int (*printf)(const char *, ...);
	uint16_t (*send_blocking_with_timeout)(const void*,
	                                       const uint16_t,
	                                       const ComType com);
	void (*bricklet_select)(const uint8_t bricklet);
	void (*bricklet_deselect)(const uint8_t bricklet);
	uint8_t (*TWID_Read)(Twid *pTwid,
	                     uint8_t address,
	                     uint32_t iaddress,
	                     uint8_t isize,
	                     uint8_t *pData,
	                     uint32_t num,
	                     Async *pAsync);
	uint8_t (*TWID_Write)(Twid *pTwid,
	                      uint8_t address,
	                      uint32_t iaddress,
	                      uint8_t isize,
	                      uint8_t *pData,
	                      uint32_t num,
	                      Async *pAsync);
	uint8_t (*PIO_Configure)(const Pin *list, uint32_t size);
	void (*PIO_ConfigureIt)(const Pin *pPin,
	                        void (*handler)(const Pin*));
	Mutex (*mutex_create)(void);
	bool (*mutex_take)(Mutex mutex, uint32_t time);
	bool (*mutex_give)(Mutex mutex);
	uint16_t (*adc_channel_get_data)(const uint8_t c);
	bool (*i2c_eeprom_master_read)(Twi *twi,
	                               const uint16_t internal_address,
	                               char *data,
	                               const uint16_t length);
	bool (*i2c_eeprom_master_write)(Twi *twi,
	                                const uint16_t internal_address,
	                                const char *data,
	                                const uint16_t length);
	Twid *twid;
	Mutex *mutex_twi_bricklet;
	void (*com_return_error)(const void *data, const uint8_t ret_length, uint8_t error_code, ComType com);
	void (*com_return_setter)(ComType com, const void *data);
	void (*com_make_default_header)(void *message, uint32_t uid, uint8_t length, uint8_t fid);
} BrickletAPI;

typedef struct {
	uint32_t plugin;
	BrickletEntryFunction entry;
	uint32_t api;
	uint32_t settings;
	uint32_t context;
} BrickletAddress;

typedef struct {
	char port;
	uint8_t address;
	Pin pin1_ad;
	Pin pin2_da;
	Pin pin3_pwm;
	Pin pin4_io;
	Pin pin_select;
	uint8_t adc_channel;
	const BrickletAddress *baddr;
	uint32_t uid;
	uint8_t firmware_version[3];
	uint8_t hardware_version[3];
	uint16_t device_identifier;
} BrickletSettings;

#define BRICKLET_MAGIC_NUMBER (23 | (42 << 8) | (17 << 16) | (99 << 24))

#define END_OF_MEMORY (IFLASH_ADDR + IFLASH_SIZE)
#if BRICKLET_NUM == 1
#define END_OF_BRICKLET_MEMORY BRICKLET_CONTEXT_ADDRESS_B
#elif BRICKLET_NUM == 2
#define END_OF_BRICKLET_MEMORY BRICKLET_CONTEXT_ADDRESS_C
#elif BRICKLET_NUM == 3
#define END_OF_BRICKLET_MEMORY BRICKLET_CONTEXT_ADDRESS_D
#elif BRICKLET_NUM == 4
#define END_OF_BRICKLET_MEMORY (BRICKLET_CONTEXT_ADDRESS_D - BRICKLET_PLUGIN_MAX_SIZE)
#endif

#define BRICKLET_CONTEXT_MAX_SIZE 256
#define BRICKLET_PLUGIN_MAX_SIZE 0x1000 // 4KByte (0x1600 8)

#define BRICKLET_ADDRESS_A (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*1)
#define BRICKLET_ADDRESS_B (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*2)
#define BRICKLET_ADDRESS_C (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*3)
#define BRICKLET_ADDRESS_D (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*4)

#define BRICKLET_ENTRY_A ((BrickletEntryFunction) (BRICKLET_ADDRESS_A+1))
#define BRICKLET_ENTRY_B ((BrickletEntryFunction) (BRICKLET_ADDRESS_B+1))
#define BRICKLET_ENTRY_C ((BrickletEntryFunction) (BRICKLET_ADDRESS_C+1))
#define BRICKLET_ENTRY_D ((BrickletEntryFunction) (BRICKLET_ADDRESS_D+1))

#define BRICKLET_API_ADDRESS_A (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*0 - 4)
#define BRICKLET_API_ADDRESS_B (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*1 - 4)
#define BRICKLET_API_ADDRESS_C (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*2 - 4)
#define BRICKLET_API_ADDRESS_D (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*3 - 4)

#define BRICKLET_SETTINGS_ADDRESS_A (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*0 - 8)
#define BRICKLET_SETTINGS_ADDRESS_B (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*1 - 8)
#define BRICKLET_SETTINGS_ADDRESS_C (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*2 - 8)
#define BRICKLET_SETTINGS_ADDRESS_D (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*3 - 8)

#define BRICKLET_CONTEXT_ADDRESS_A (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*0 - 12)
#define BRICKLET_CONTEXT_ADDRESS_B (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*1 - 12)
#define BRICKLET_CONTEXT_ADDRESS_C (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*2 - 12)
#define BRICKLET_CONTEXT_ADDRESS_D (END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*3 - 12)

#define BRICKLET_TYPE_INVOCATION 1
#define BRICKLET_TYPE_CONSTRUCTOR 2
#define BRICKLET_TYPE_DESTRUCTOR 4
#define BRICKLET_TYPE_TICK 8
#define BRICKLET_TYPE_INFO 16

#if(DEBUG_BRICKLET)
#define logbletd(str, ...) do{logd("blet: " str, ##__VA_ARGS__);}while(0)
#define logbleti(str, ...) do{logi("blet: " str, ##__VA_ARGS__);}while(0)
#define logbletw(str, ...) do{logw("blet: " str, ##__VA_ARGS__);}while(0)
#define logblete(str, ...) do{loge("blet: " str, ##__VA_ARGS__);}while(0)
#define logbletf(str, ...) do{logf("blet: " str, ##__VA_ARGS__);}while(0)
#else
#define logbletd(str, ...) {}
#define logbleti(str, ...) {}
#define logbletw(str, ...) {}
#define logblete(str, ...) {}
#define logbletf(str, ...) {}
#endif

#endif
