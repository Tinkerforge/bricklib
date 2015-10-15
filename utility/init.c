/* bricklib
 * Copyright (C) 2010-2014 Olaf Lüke <olaf@tinkerforge.com>
 *
 * init.c: Implementation of initialization valid for all bricks
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

#include "init.h"

#include "bricklib/drivers/pio/pio_it.h"
#include "bricklib/drivers/wdt/wdt.h"
#include "bricklib/drivers/adc/adc.h"

#include "bricklib/free_rtos/include/FreeRTOS.h"
#include "bricklib/free_rtos/include/task.h"

#include "bricklib/com/com_common.h"
#include "bricklib/bricklet/bricklet_init.h"
#include "bricklib/com/i2c/i2c_eeprom/i2c_eeprom_master.h"
#include "bricklib/com/usb/usb.h"
#include "bricklib/logging/logging.h"
#include "bricklib/utility/led.h"
#include "bricklib/utility/mutex.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/logging/logging.h"
#include "bricklib/bricklet/bricklet_init.h"

#include "config.h"

extern ComInfo com_info;

static uint8_t type_calculation = TICK_TASK_TYPE_CALCULATION;
static uint8_t type_message = TICK_TASK_TYPE_MESSAGE;

void brick_init(void) {
	// Wait 5ms so everything can power up
	SLEEP_MS(5);
	logging_init();

	logsi("Booting %d\n\r", BRICK_DEVICE_IDENTIFIER);
	logsi("Compiled on %s %s\n\r", __DATE__, __TIME__);

    led_init();
	led_on(LED_STD_BLUE);
#ifdef LED_STD_RED
#if LOGGING_LEVEL == LOGGING_NONE
	led_off(LED_STD_RED);
#else
	led_on(LED_STD_RED);
#endif
#endif
	logsi("LEDs initialized\n\r");

	com_info.uid = uid_get_uid32();

	// Add 0 at end for printing
    char sn[MAX_BASE58_STR_SIZE] = {'\0'};
    uid_to_serial_number(com_info.uid, sn);
    set_serial_number_descriptor(sn, MAX_BASE58_STR_SIZE);
    logsi("Unique ID %s (%lu)\n\r\n\r", sn, com_info.uid);

    wdt_start();
    logsi("Watchdog disabled\n\r");

    mutex_init();
    logsi("Mutexes initialized\n\r");

	// Disable JTAG (Pins are needed for i2c)
#ifdef DISABLE_JTAG_ON_STARTUP
	MATRIX->CCFG_SYSIO |= (CCFG_SYSIO_SYSIO12 |
	                       CCFG_SYSIO_SYSIO4  |
	                       CCFG_SYSIO_SYSIO5  |
	                       CCFG_SYSIO_SYSIO6  |
	                       CCFG_SYSIO_SYSIO7);
	logsi("JTAG disabled\n\r");
#endif

	com_info.current = COM_NONE;
    PIO_InitializeInterrupts(15);

    bricklet_clear_eeproms();
    i2c_eeprom_master_init(TWI_BRICKLET);
    logsi("I2C for Bricklets initialized\n\r");

	usb_detect_configure();

	adc_init();
	adc_enable_temperature_sensor();
#ifndef NO_PERIODIC_ADC_CONVERISION
	adc_start_periodic_conversion();
#endif
    logsi("A/D converter initialized\n\r");

	bricklet_init();
}

void SUPC_IrqHandler(void) {
    loge("SUPC: %x\n\r", SUPC->SUPC_SR);
    while(1);
}


void brick_enable_brownout_detection(void) {
	SUPC->SUPC_MR |= SUPC_MR_KEY(0xA5) | SUPC_MR_BODDIS_ENABLE;
//	SUPC->SUPC_SMMR |= SUPC_SMMR_SMSMPL_CSM | SUPC_SMMR_SMTH_3_2V | SUPC_SMMR_SMIEN_ENABLE;
	volatile uint32_t tmp = SUPC->SUPC_SR; // clear sms and smos
	NVIC_DisableIRQ(SUPC_IRQn);
	NVIC_ClearPendingIRQ(SUPC_IRQn);
	NVIC_SetPriority(SUPC_IRQn, 0);
	NVIC_EnableIRQ(SUPC_IRQn);
}

void brick_reset(void) {
	logd("brick_reset\n\r");
	RSTC->RSTC_MR = RSTC_MR_URSTEN |
	                (10 << RSTC_MR_ERSTL_Pos) |
	                RSTC_MR_KEY(0xA5);

	RSTC->RSTC_CR = RSTC_CR_EXTRST |
	                RSTC_CR_PERRST |
	                RSTC_CR_PROCRST |
	                RSTC_CR_KEY(0xA5);
}

void brick_init_start_tick_task(void) {
	logsi("Add tick_task\n\r");

	xTaskCreate(brick_tick_task,
				(signed char *)"bmt",
#ifdef BRICK_CAN_BE_MASTER
				800,
#else
				600,
#endif
				&type_message,
				1,
				(xTaskHandle *)NULL);

	xTaskCreate(brick_tick_task,
				(signed char *)"bct",
#ifdef BRICK_CAN_BE_MASTER
				800,
#else
				600,
#endif
				&type_calculation,
				1,
				(xTaskHandle *)NULL);
}

bool brick_init_enumeration(const ComType com) {
	EnumerateCallback ec = MESSAGE_EMPTY_INITIALIZER;
	make_brick_enumerate(&ec);
	ec.enumeration_type = ENUMERATE_TYPE_ADDED;

	if(SEND(&ec, sizeof(EnumerateCallback), com, NULL) != 0) {
		logd("Returning initial Enumeration for Brick: %lu\n\r", ec.header.uid);

		for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
			EnumerateCallback ec = MESSAGE_EMPTY_INITIALIZER;
			make_bricklet_enumerate(&ec, i);
			if(ec.device_identifier != 0) {
				logd("Returning initial Enumeration for Bricklet %c: %lu\n\r", 'a' + i, ec.header.uid);
				ec.enumeration_type = ENUMERATE_TYPE_ADDED;
				while(send_blocking_with_timeout(&ec, sizeof(EnumerateCallback), com) == 0);
			}
		}

		return true;
	}

	return false;
}

void brick_tick_task(void *parameters) {
	const uint8_t tick_type = *((uint8_t*)parameters);
	unsigned long last_wake_time = xTaskGetTickCount();
	while(true) {
		wdt_restart();
		tick_task(tick_type);
		taskYIELD();
		bricklet_tick_task(tick_type);
		led_tick_task(tick_type);
		usb_detect_task(tick_type);

		// 1ms resolution
		unsigned long tick_count = xTaskGetTickCount();
		if(tick_count > last_wake_time + 2) {
			last_wake_time = tick_count - 1;
		}

		vTaskDelayUntil(&last_wake_time, 1);
	}
}
