/* bricklib
 * Copyright (C) 2010-2013 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_init.c: functions for bricklet initialization
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

#include "bricklet_init.h"

#include <string.h>
#include "bricklib/drivers/flash/flashd.h"
#include "bricklib/drivers/efc/efc.h"
#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"
#include "bricklib/drivers/adc/adc.h"
#include "bricklib/drivers/wdt/wdt.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/pio/pio_it.h"

#include "bricklib/logging/logging.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"
#include "bricklib/com/i2c/i2c_clear_bus.h"
#include "bricklib/bricklet/bricklet_communication.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/utility/init.h"

#include "config.h"
#include "bricklet_config.h"
#include "bricklet_co_mcu.h"

// Includes for bricklet api
#include "bricklib/drivers/twi/twid.h" // TWID_Read, TWID_Write
#include <stdio.h> // printf
#include "bricklib/drivers/pio/pio.h" // PIO_Configure
#include "bricklib/drivers/pio/pio_it.h" // PIO_ConfigureIt
#include "bricklib/drivers/adc/adc.h" // adc_channel_get_data
#include "bricklib/com/com_messages.h" // get_com_from_type
#include "bricklib/com/com_common.h" // send_blocking_with_timeout
#include "bricklib/utility/mutex.h" // mutex_*

#define BRICKLET_DEBOUNCE_TICKS 1000

extern ComInfo com_info;
extern uint8_t bricklet_eeprom_address;
extern Mutex mutex_twi_bricklet;

Twid twid = {TWI_BRICKLET, NULL};

// Declare bricklet api (ba)
const BrickletAPI ba = {
	&com_info.current,
	printf,
	send_blocking_with_timeout,
	bricklet_select,
	bricklet_deselect,
	TWID_Read,
	TWID_Write,
	PIO_Configure,
	PIO_ConfigureIt,
	mutex_create,
	mutex_take,
	mutex_give,
	adc_channel_get_data,
	i2c_eeprom_master_read,
	i2c_eeprom_master_write,
	&twid,
	&mutex_twi_bricklet,
	&com_return_error,
	&com_return_setter,
	&com_make_default_header,
	&mutex_give_isr,
	&yield_from_isr
};

const BrickletAddress baddr[BRICKLET_NUM] = {
#if BRICKLET_NUM > 0
	 {BRICKLET_ADDRESS_A,
	  BRICKLET_ENTRY_A,
	  BRICKLET_API_ADDRESS_A,
	  BRICKLET_SETTINGS_ADDRESS_A,
	  BRICKLET_CONTEXT_ADDRESS_A}
#endif
#if BRICKLET_NUM > 1
	,{BRICKLET_ADDRESS_B,
	  BRICKLET_ENTRY_B,
	  BRICKLET_API_ADDRESS_B,
	  BRICKLET_SETTINGS_ADDRESS_B,
	  BRICKLET_CONTEXT_ADDRESS_B}
#endif
#if BRICKLET_NUM > 2
	,{BRICKLET_ADDRESS_C,
	  BRICKLET_ENTRY_C,
	  BRICKLET_API_ADDRESS_C,
	  BRICKLET_SETTINGS_ADDRESS_C,
	  BRICKLET_CONTEXT_ADDRESS_C}
#endif
#if BRICKLET_NUM > 3
	,{BRICKLET_ADDRESS_D,
	  BRICKLET_ENTRY_D,
	  BRICKLET_API_ADDRESS_D,
	  BRICKLET_SETTINGS_ADDRESS_D,
	  BRICKLET_CONTEXT_ADDRESS_D}
#endif
};

// Declare bricklet settings (bs)
BrickletSettings bs[BRICKLET_NUM] = {
	#if BRICKLET_NUM > 0
		 {'a',
		  BRICKLET_A_ADDRESS,
		  BRICKLET_A_PIN_1_AD,
		  BRICKLET_A_PIN_2_DA,
		  BRICKLET_A_PIN_3_PWM,
		  BRICKLET_A_PIN_4_IO,
		  BRICKLET_A_PIN_SELECT,
		  BRICKLET_A_ADC_CHANNEL,
		  &baddr[0],
		  0, {0, 0, 0}, {0, 0, 0}, 0}
	#endif
	#if BRICKLET_NUM > 1
		,{'b',
		  BRICKLET_B_ADDRESS,
		  BRICKLET_B_PIN_1_AD,
		  BRICKLET_B_PIN_2_DA,
		  BRICKLET_B_PIN_3_PWM,
		  BRICKLET_B_PIN_4_IO,
		  BRICKLET_B_PIN_SELECT,
		  BRICKLET_B_ADC_CHANNEL,
		  &baddr[1],
		  0, {0, 0, 0}, {0, 0, 0}, 0}
	#endif
	#if BRICKLET_NUM > 2
		,{'c',
		  BRICKLET_C_ADDRESS,
		  BRICKLET_C_PIN_1_AD,
		  BRICKLET_C_PIN_2_DA,
		  BRICKLET_C_PIN_3_PWM,
		  BRICKLET_C_PIN_4_IO,
		  BRICKLET_C_PIN_SELECT,
		  BRICKLET_C_ADC_CHANNEL,
		  &baddr[2],
		  0, {0, 0, 0}, {0, 0, 0}, 0}
	#endif
	#if BRICKLET_NUM > 3
		,{'d',
		  BRICKLET_D_ADDRESS,
		  BRICKLET_D_PIN_1_AD,
		  BRICKLET_D_PIN_2_DA,
		  BRICKLET_D_PIN_3_PWM,
		  BRICKLET_D_PIN_4_IO,
		  BRICKLET_D_PIN_SELECT,
		  BRICKLET_D_ADC_CHANNEL,
		  &baddr[3],
		  0, {0, 0, 0}, {0, 0, 0}, 0}
	#endif
};

// Declare bricklet context (bc)
uint32_t bc[BRICKLET_NUM][BRICKLET_CONTEXT_MAX_SIZE/4] = {{0}};

uint8_t bricklet_attached[BRICKLET_NUM] = {
	#if BRICKLET_NUM > 0
		BRICKLET_INIT_NO_BRICKLET,
	#endif
	#if BRICKLET_NUM > 1
		BRICKLET_INIT_NO_BRICKLET,
	#endif
	#if BRICKLET_NUM > 2
		BRICKLET_INIT_NO_BRICKLET,
	#endif
	#if BRICKLET_NUM > 3
		BRICKLET_INIT_NO_BRICKLET
	#endif
};

void bricklet_select(const uint8_t bricklet) {
	bricklet_eeprom_address = bs[bricklet].address;
	if(bs[bricklet].pin_select.pio != NULL) {
		PIO_Set(&(bs[bricklet].pin_select));
	}
}

void bricklet_deselect(const uint8_t bricklet) {
	if(bs[bricklet].pin_select.pio != NULL) {
		PIO_Clear(&(bs[bricklet].pin_select));
	}
}

void bricklet_write_asc_to_flash(const uint8_t bricklet) {
	// Disable all irqs before plugin is written to flash.
	// While writing to flash there can't be any other access to the flash
	// (e.g. via interrupts).
	DISABLE_RESET_BUTTON();
	__disable_irq();

	// Write api address to flash
	int adr = (int)&ba;
	FLASHD_Write(baddr[bricklet].api, &adr, sizeof(int*));

	// Write settings address to flash
	adr = (int)&bs[bricklet];
	FLASHD_Write(baddr[bricklet].settings, &adr, sizeof(int*));

	// Write context address to flash
	adr = (int)&bc[bricklet];
	FLASHD_Write(baddr[bricklet].context, &adr, sizeof(int*));

	__enable_irq();
    ENABLE_RESET_BUTTON();
}

void bricklet_write_plugin_to_flash(const char *plugin,
	                                const uint8_t position,
	                                const uint8_t bricklet,
	                                const uint16_t chunk_size) {
	// Disable all irqs before plugin is written to flash.
	// While writing to flash there can't be any other access to the flash
	// (e.g. via interrupts).
	DISABLE_RESET_BUTTON();
	__disable_irq();

    // Write plugin to flash
    uint16_t add = position*chunk_size;
    FLASHD_Write(baddr[bricklet].plugin + add, plugin, chunk_size);

    __enable_irq();
    ENABLE_RESET_BUTTON();
}

uint8_t bricklet_init_plugin(const uint8_t bricklet) {
	const uint32_t PLUGIN_CHUNK_SIZE_STARTUP = IS_SAM3() ? IFLASH_PAGE_SIZE_SAM3 : IFLASH_PAGE_SIZE_SAM4;
	bricklet_select(bricklet);

	char plugin[PLUGIN_CHUNK_SIZE_STARTUP];

	const uint16_t end = BRICKLET_PLUGIN_MAX_SIZE/PLUGIN_CHUNK_SIZE_STARTUP;

	for(uint16_t position = 0; position < end; position++) {
		i2c_eeprom_master_read_plugin(TWI_BRICKLET, plugin, position, PLUGIN_CHUNK_SIZE_STARTUP);
		bricklet_write_plugin_to_flash(plugin, position, bricklet, PLUGIN_CHUNK_SIZE_STARTUP);
	}

	bricklet_deselect(bricklet);
	bricklet_write_asc_to_flash(bricklet);

    memset(bc[bricklet], 0, BRICKLET_CONTEXT_MAX_SIZE);

    uint8_t protocol_version = 0;

    baddr[bricklet].entry(BRICKLET_TYPE_PROTOCOL_VERSION, 0, &protocol_version);
    if(protocol_version != 2) {
    	logbleti("Bricklet %c plugin has protocol version 1\n\r", 'a' + bricklet);
    	return BRICKLET_INIT_PROTOCOL_VERSION_1;
    }

	logbleti("Calling constructor for bricklet %c\n\r", 'a' + bricklet);
	baddr[bricklet].entry(BRICKLET_TYPE_CONSTRUCTOR, 0, NULL);

	return BRICKLET_INIT_PROTOCOL_VERSION_2;
}

void bricklet_try_connection(const uint8_t bricklet) {
	bricklet_select(bricklet);
/*	const uint32_t magic_number = i2c_eeprom_master_read_magic_number(TWI_BRICKLET);
	if(magic_number != BRICKLET_MAGIC_NUMBER) {
		logbleti("Bricklet %c not connected (wrong magic: %d != %d)\n\r", 'a' + bricklet, magic_number, BRICKLET_MAGIC_NUMBER);
		bricklet_deselect(bricklet);
		return;
	}*/

	const uint32_t uid = i2c_eeprom_master_read_uid(TWI_BRICKLET);
	bricklet_deselect(bricklet);

	if(uid == 0) {
#ifdef BRICK_HAS_CO_MCU_SUPPORT
		bricklet_co_mcu_init(bricklet);
		bricklet_attached[bricklet] = BRICKLET_INIT_CO_MCU;
		bs[bricklet].uid = 0;
		bs[bricklet].device_identifier = 0;
#endif
		return;
	}

	uint32_t plugin_entry;
	i2c_eeprom_master_read_plugin(TWI_BRICKLET, (char*)&plugin_entry, 0, 4);

	if(plugin_entry == 0xFFFFFFFF || plugin_entry == 0) {
		logbleti("Bricklet %c does not have a valid plugin\n\r", 'a' + bricklet);
		return;
	}

	logbleti("Bricklet %c connected\n\r", 'a' + bricklet);

	bricklet_attached[bricklet] =  bricklet_init_plugin(bricklet);
	if(bricklet_attached[bricklet] == BRICKLET_INIT_NO_BRICKLET) {
		logbletw("Could not init Bricklet %c\n\r", 'a' + bricklet);
		return;
	} else if(bricklet_attached[bricklet] == BRICKLET_INIT_PROTOCOL_VERSION_1) {
		logbletw("Bricklet %c has incompatible protocol version\n\r", 'a' + bricklet);
		return;
	}

	baddr[bricklet].entry(BRICKLET_TYPE_INFO,
	                      0,
	                      (uint8_t*)&bs[bricklet]);

	bs[bricklet].uid = uid;

	logbleti("Bricklet %c configured (UID %lu)\n\r", 'a' + bricklet, uid);
}

void bricklet_tick_task(const uint8_t tick_type) {
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		switch(bricklet_attached[i]) {
			case BRICKLET_INIT_PROTOCOL_VERSION_2: {
				uint8_t tt = tick_type;
				baddr[i].entry(BRICKLET_TYPE_TICK, 0, &tt);
				break;
			}

			case BRICKLET_INIT_CO_MCU: {
				if(tick_type == TICK_TASK_TYPE_MESSAGE) {
					bricklet_co_mcu_poll(i);
				}
				break;
			}

			default: break;
		}
	}
}


// This is needed if reset is pressed while communication
// to the eeprom is running.
// NOTE: after bricklet_clear_eeproms i2c has to be reconfigured!
void bricklet_clear_eeproms(void) {
	Pin sda = PIN_TWI_TWD_BRICKLET;
	Pin scl = PIN_TWI_TWCK_BRICKLET;
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		bricklet_select(i);
		i2c_clear_bus(&sda, &scl);
		bricklet_deselect(i);
	}
}

void bricklet_init(void) {
	wdt_restart();
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].pin_select.pio != NULL) {
			PIO_Configure(&(bs[i].pin_select), 1);
		}
		bricklet_attached[i] = BRICKLET_INIT_NO_BRICKLET;
	}

	if(!IS_SAM3()) {
		EFC_SetWaitState(EFC, 6);
	}

	// Unlock flash region for all Bricklet plugins
	FLASHD_Unlock(END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*BRICKLET_NUM,
	              END_OF_MEMORY,
				  0,
				  0);

	// On SAM4 we have to erase the flash here, the EFC_FCMD_EWP command does not work...
	if(!IS_SAM3()) {
		// EFC_FCMD_EPA = 0x07
		if(BRICKLET_NUM == 2) {
			EFC_PerformCommand(EFC, 0x07, (((256-16)/16) << 4) | 2, 0);
		} else if(BRICKLET_NUM == 4) {
			EFC_PerformCommand(EFC, 0x07, (((512-32)/32) << 5) | 3, 0);
		}
	}

	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		bricklet_try_connection(i);
	}

    // Lock flash again
    FLASHD_Lock(END_OF_MEMORY - BRICKLET_PLUGIN_MAX_SIZE*BRICKLET_NUM,
                END_OF_MEMORY,
				0,
				0);

	// Only change wait states on SAM4, because this makes a SAM3 hang for unknown reasons
	if(!IS_SAM3()) {
		EFC_SetWaitState(EFC, 2);
	}
}
