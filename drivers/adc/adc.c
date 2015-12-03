/* bricklib
 * Copyright (C) 2011-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * adc.c: Implementation of adc access
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

#include "adc.h"

#include <stdio.h>
#include "config.h"

#include "bricklib/utility/led.h"
#include "bricklib/utility/util_definitions.h"
#include "bricklib/drivers/tc/tc.h"
#include "bricklib/drivers/flash/flashd.h"

int32_t adc_offset = 0;
uint32_t adc_gain_mul = ADC_MAX_VALUE;
uint32_t adc_gain_div = ADC_MAX_VALUE;

// sam3s adc characteristics: 41.7 (1058ff)
// max adc frequency: 20mhz
// max startup(standby -> normal mode): 12us
// track and holding time (min): 160ns
// settling: 200ns

// We try to satisfy these boundaries with a factor of at least 10

// use 0.25mhz adc frequency with prescaler 127
// 1 clock cycle is 8us
#define ADC_PRESCALER 127

// 512 clock cycle startup time
#define ADC_STARTUP   ADC_MR_STARTUP_SUT512

// Tracking Time = (TRACKTIM + 1) * ADCClock periods ->
// 3*8us
#define ADC_TRACKTIM 2

// smallest possible value is 5 ->
// 5*8us
#define ADC_SETTLING 1

// Transfer Period = (TRANSFER * 2 + 3) ADCClock periods ->
// 5*8us
#define ADC_TRANSFER 1

// All together, when 8 adc channels are used:
// 4*(3+5+5)*8us = 416us
// Maximum trigger frequency is 2.404kHz

void adc_init(void) {
#ifndef ADC_NO_CALIBRATION
	adc_read_calibration_from_flash();
#endif

    // Enable peripheral clock
    PMC->PMC_PCER0 = 1 << ID_ADC;

    // Reset controller
    ADC->ADC_CR = ADC_CR_SWRST;

    // Track time 0
    // Transfer 1
    // Settling 3
    ADC->ADC_MR = ADC_MR_TRANSFER(ADC_TRANSFER) |
                  ADC_MR_TRACKTIM(ADC_TRACKTIM) |
                  ADC_MR_SETTLING(ADC_SETTLING) |
                  ADC_MR_PRESCAL(ADC_PRESCALER) |
                  ADC_STARTUP;
}

void adc_start_periodic_conversion(void) {
	ADC->ADC_MR |= ADC_MR_FREERUN_ON;
}

void adc_stop_periodic_conversion(void) {
	const uint32_t mode = ADC->ADC_MR;
	ADC->ADC_MR = mode & (~ADC_MR_FREERUN_ON);
}


uint16_t adc_channel_get_data(const uint8_t c) {
	int32_t value;
#ifdef BRICK_CAN_BE_MASTER
	uint32_t orig_value = ADC->ADC_CDR[c]*10;
	if(orig_value == ADC_MAX_VALUE*10) {
		return ADC_MAX_VALUE;
	} else if(orig_value == 0) {
		return 0;
	}

	if(adc_gain_div > ADC_MAX_VALUE) {
		value = ((int32_t)(adc_gain_mul*orig_value/adc_gain_div) + adc_offset + 5)/10;
	} else {
		value = adc_gain_mul*ADC->ADC_CDR[c]/adc_gain_div + adc_offset;
	}
#else
	value = adc_gain_mul*ADC->ADC_CDR[c]/adc_gain_div + adc_offset;
#endif

	if(value < 0) {
		return 0;
	}
	if(value > ADC_MAX_VALUE) {
		return ADC_MAX_VALUE;
	}

	return (uint16_t)value;
}


#ifndef ADC_NO_CALIBRATION

void adc_read_calibration_from_flash(void) {
	// If adc values are set before this is called we don't read values
	if(adc_offset != 0 ||
	   adc_gain_mul != ADC_MAX_VALUE ||
	   adc_gain_div != ADC_MAX_VALUE) {
		return;
	}
	uint32_t *data = (uint32_t*)ADC_CALIBRATION_ADDRESS;
	int16_t offset = *data & 0xFFFF;
	int16_t gain = *data >> 16;

	// If values are not yet written use default values
	if(gain == -1 && offset == -1) {
		gain = ADC_MAX_VALUE;
		offset = 0;
	}

	adc_offset = offset;
	adc_gain_div = gain;
}

void adc_write_calibration_to_flash(void) {
	uint32_t data = ((uint16_t)adc_offset) | (((uint16_t)adc_gain_div) << 16);

	// Disable all irqs before plugin is written to flash.
	// While writing to flash there can't be any other access to the flash
	// (e.g. via interrupts).
	DISABLE_RESET_BUTTON();
	__disable_irq();

	// Unlock flash region
	FLASHD_Unlock(ADC_CALIBRATION_ADDRESS,
	              ADC_CALIBRATION_ADDRESS + 4,
				  0,
				  0);

	// Write api address to flash
	FLASHD_Write(ADC_CALIBRATION_ADDRESS, &data, sizeof(data));

	// Lock flash and enable irqs again
	FLASHD_Lock(ADC_CALIBRATION_ADDRESS,
	            ADC_CALIBRATION_ADDRESS + 4,
	            0,
				0);

	__enable_irq();
    ENABLE_RESET_BUTTON();
}

void adc_calibrate(const uint8_t c) {
	bool was_enabled = false;
	if(adc_channel_is_enabled(c)) {
		was_enabled = true;
	} else {
		adc_channel_enable(c);
		SLEEP_MS(2);
	}

	uint32_t value = 0;

	for(uint8_t i = 0; i < 100; i++) {
		value += adc_channel_get_data_unfiltered(c);
		SLEEP_NS(100);
	}

	value /= 100;

	if(value < ADC_MAX_VALUE/2) {
		adc_offset = -value;
	} else {
		adc_gain_div = value;
	}

	if(!was_enabled) {
		adc_channel_disable(c);
	}

}

#endif

void adc_set_calibration(const int32_t offset, const uint32_t gain_mul, const uint32_t gain_div) {
	adc_offset = offset;
	adc_gain_mul = gain_mul;
	adc_gain_div = gain_div;
}

void adc_enable_temperature_sensor(void) {
	ADC->ADC_ACR |= ADC_ACR_TSON;
	adc_channel_enable(ADC_CHANNEL_TEMPERATURE_SENSOR);
}
