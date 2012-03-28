/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
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

#include <pio/pio_it.h>
#include <wdt/wdt.h>
#include <adc/adc.h>

#include <FreeRTOS.h>
#include <task.h>

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

extern ComType com_current;
extern uint64_t com_brick_uid;

static uint8_t type_calculation = TICK_TASK_TYPE_CALCULATION;
static uint8_t type_message = TICK_TASK_TYPE_MESSAGE;

void brick_init(void) {
	// Wait 5ms so everything can power up
	SLEEP_MS(5);
	logging_init();

	logsi("Booting %s\n\r", BRICK_HARDWARE_NAME);
	logsi("Compiled on %s %s\n\r", __DATE__, __TIME__);

    led_init();
	led_on(LED_STD_BLUE);
#if LOGGING_LEVEL == LOGGING_NONE
	led_off(LED_STD_RED);
#else
	led_on(LED_STD_RED);
#endif
	logsi("LEDs initialized\n\r");

	com_brick_uid = uid_get_uid64();

	// Add 0 at end for printing
    char sn[12] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0};
    uid_to_serial_number(com_brick_uid, sn);
    set_serial_number_descriptor(sn, 11);
    logsi("Unique ID %s\n\r\n\r", sn);

    WDT_Disable(WDT);
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

    com_current = COM_NONE;
    PIO_InitializeInterrupts(0);

    bricklet_clear_eeproms();
    i2c_eeprom_master_init(TWI_BRICKLET);
    logsi("I2C for Bricklets initialized\n\r");

	usb_detect_configure();

	adc_init();
#ifndef NO_PERIODIC_ADC_CONVERISION
	adc_start_periodic_conversion();
#endif
    logsi("A/D converter initialized\n\r");

	bricklet_init();
}

void brick_init_start_tick_task(void) {
	logsi("Add tick_task\n\r");

	xTaskCreate(brick_tick_task,
				(signed char *)"bmt",
				700,
				&type_message,
				1,
				(xTaskHandle *)NULL);

	xTaskCreate(brick_tick_task,
				(signed char *)"bct",
				700,
				&type_calculation,
				1,
				(xTaskHandle *)NULL);
}

void brick_tick_task(void *parameters) {
	const uint8_t tick_type = *((uint8_t*)parameters);
	unsigned long last_wake_time = xTaskGetTickCount();
	while(true) {
		tick_task(tick_type);
		taskYIELD();
		bricklet_tick_task(tick_type);
		led_tick_task(tick_type);

		// 1ms resolution
		unsigned long tick_count = xTaskGetTickCount();
		if(tick_count > last_wake_time + 2) {
			last_wake_time = tick_count - 1;
		}

		vTaskDelayUntil(&last_wake_time, 1);
	}
}
