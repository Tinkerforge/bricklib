/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
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
#include <flash/flashd.h>
#include <FreeRTOS.h>
#include <task.h>
#include <adc/adc.h>
#include <pio/pio.h>
#include <pio/pio_it.h>

#include "bricklib/logging/logging.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"
#include "bricklib/com/i2c/i2c_clear_bus.h"
#include "bricklib/bricklet/bricklet_communication.h"
#include "bricklib/utility/util_definitions.h"

#include "config.h"
#include "bricklet_config.h"

// Includes for bricklet api
#include <twi/twid.h> // TWID_Read, TWID_Write
#include <stdio.h> // printf
#include <pio/pio.h> // PIO_Configure
#include <pio/pio_it.h> // PIO_ConfigureIt
#include "bricklib/drivers/adc/adc.h" // adc_channel_get_data
#include "bricklib/com/com_messages.h" // get_com_from_type
#include "bricklib/com/com_common.h" // send_blocking_with_timeout
#include "bricklib/utility/mutex.h" // mutex_*

#define BRICKLET_DEBOUNCE_TICKS 1000

extern ComType com_current;
extern uint8_t com_last_spi_stack_id;
extern uint8_t com_stack_id;
extern uint8_t bricklet_eeprom_address;
extern Mutex mutex_twi_bricklet;

Twid twid = {TWI_BRICKLET, NULL};

// Declare bricklet api (ba)
const BrickletAPI ba = {
	&com_current,
	printf,
	get_com_from_data,
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
	&mutex_twi_bricklet
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
		  0, 0, {0, 0, 0}, ""}
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
		  0, 0, {0, 0, 0}, ""}
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
		  0, 0, {0, 0, 0}, ""}
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
		  0, 0, {0, 0, 0}, ""}
	#endif
};

// Declare bricklet context (bc)
uint8_t bc[BRICKLET_NUM][250] = {{0}};

static bool bricklet_attached[4] = {false, false, false, false};

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

	// Unlock flash region
	FLASHD_Unlock(baddr[bricklet].plugin,
				  baddr[bricklet].plugin + BRICKLET_PLUGIN_MAX_SIZE,
				  0,
				  0);

	// Write api address to flash
	int adr = (int)&ba;
	FLASHD_Write(baddr[bricklet].api, &adr, sizeof(BrickletAPI*));

	// Write settings address to flash
	adr = (int)&bs[bricklet];
	FLASHD_Write(baddr[bricklet].settings, &adr, sizeof(BrickletSettings*));

	// Write context address to flash
	adr = (int)&bc[bricklet];
	FLASHD_Write(baddr[bricklet].context, &adr, sizeof(int*));

	// Lock flash and enable irqs again
	FLASHD_Lock(baddr[bricklet].plugin,
				baddr[bricklet].plugin + BRICKLET_PLUGIN_MAX_SIZE,
				0,
				0);

	__enable_irq();
    ENABLE_RESET_BUTTON();
}

void bricklet_write_plugin_to_flash(const char *plugin,
	                                const uint8_t position,
	                                const uint8_t bricklet) {
	// Disable all irqs before plugin is written to flash.
	// While writing to flash there can't be any other access to the flash
	// (e.g. via interrupts).
	DISABLE_RESET_BUTTON();
	__disable_irq();

	// Unlock flash region
    FLASHD_Unlock(baddr[bricklet].plugin,
	              baddr[bricklet].plugin + BRICKLET_PLUGIN_MAX_SIZE,
				  0,
				  0);

    // Write plugin to flash
    uint16_t add = position*PLUGIN_CHUNK_SIZE;
    FLASHD_Write(baddr[bricklet].plugin + add, plugin, PLUGIN_CHUNK_SIZE);

    // Lock flash and enable irqs again
    FLASHD_Lock(baddr[bricklet].plugin,
                baddr[bricklet].plugin + BRICKLET_PLUGIN_MAX_SIZE,
				0,
				0);

    __enable_irq();
    ENABLE_RESET_BUTTON();
}

bool bricklet_init_plugin(const uint8_t bricklet) {
	bricklet_select(bricklet);

	char plugin[PLUGIN_CHUNK_SIZE];

	const uint16_t end = BRICKLET_PLUGIN_MAX_SIZE/PLUGIN_CHUNK_SIZE - 1;

	for(int position = 0; position < end; position++) {
		i2c_eeprom_master_read_plugin(TWI_BRICKLET, plugin, position);
		bricklet_write_plugin_to_flash(plugin, position, bricklet);
	}

	bricklet_deselect(bricklet);
	bricklet_write_asc_to_flash(bricklet);

	logbleti("Calling constructor for bricklet %c\n\r", 'a' + bricklet);
	baddr[bricklet].entry(BRICKLET_TYPE_CONSTRUCTOR, 0, NULL);
	bricklet_attached[bricklet] = true;

	return true;
}

void bricklet_try_connection(const uint8_t bricklet) {
	bricklet_select(bricklet);
	uint64_t uid = i2c_eeprom_master_read_uid(TWI_BRICKLET);
	bricklet_deselect(bricklet);

	if(uid == 0) {
		logbleti("Bricklet %c not connected\n\r", 'a' + bricklet);
		return;
	}

	logbleti("Bricklet %c connected\n\r", 'a' + bricklet);

	if(!bricklet_init_plugin(bricklet)) {
		logbletw("Could not init Bricklet %c\n\r", 'a' + bricklet);
		return;
	}

	BrickletInfo bi;
	baddr[bricklet].entry(BRICKLET_TYPE_INFO,
	                      0,
	                      (uint8_t*)&bi);

	bs[bricklet].firmware_version[0] = bi.firmware_version[0];
	bs[bricklet].firmware_version[1] = bi.firmware_version[1];
	bs[bricklet].firmware_version[2] = bi.firmware_version[2];
	strncpy(bs[bricklet].name, bi.name, MAX_LENGTH_NAME);

	bs[bricklet].uid = uid;

	logbleti("Bricklet %c configured\n\r", 'a' + bricklet);
}

void bricklet_tick_task(void) {
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bricklet_attached[i]) {
			baddr[i].entry(BRICKLET_TYPE_TICK, 0, NULL);
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
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].pin_select.pio != NULL) {
			PIO_Configure(&(bs[i].pin_select), 1);
		}
		bricklet_attached[i] = false;
	}

	uint8_t bricklet_stack_id = com_stack_id;
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		bricklet_try_connection(i);
		if(bs[i].uid != 0) {
			bricklet_stack_id++;
			bs[i].stack_id = bricklet_stack_id;
		}
	}
	com_last_spi_stack_id = bricklet_stack_id;
}
