/* bricklib
 * Copyright (C) 2011-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * adc.h: Implementation of adc access
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

#ifndef ADC_H
#define ADC_H

#include "config.h"
#include "bricklib/bricklet/bricklet_config.h"

#define ADC_CHANNEL_TEMPERATURE_SENSOR 15
#define ADC_CALIBRATION_ADDRESS (END_OF_BRICKLET_MEMORY - 4)

#define ADC_MAX_VALUE ((1 << 12) - 1)

#define adc_start_conversion() do{ ADC->ADC_CR = ADC_CR_START; }while(0)
#define adc_channel_enable(c) do{ ADC->ADC_CHER = (1 << (c)); }while(0)
#define adc_channel_disable(c) do{ ADC->ADC_CHDR = (1 << (c)); }while(0)
#define adc_channel_is_enabled(c) (ADC->ADC_CHSR & (1 << (c)))
#define adc_channel_has_new_data(c) (ADC->ADC_ISR & (1 << (c)))
#define adc_channel_get_data_unfiltered(c) (ADC->ADC_CDR[(c)])
#define adc_get_temperature() (((((int32_t)adc_channel_get_data(ADC_CHANNEL_TEMPERATURE_SENSOR))*3300/4095)-800)*1000/265 + 270)

void adc_enable_temperature_sensor(void);
void adc_init(void);
void adc_start_periodic_conversion(void);
uint16_t adc_channel_get_data(const uint8_t c);
void adc_set_calibration(const int32_t offset, const uint32_t gain_mul, const uint32_t gain_div);
void adc_calibrate(const uint8_t c);
void adc_read_calibration_from_flash(void);
void adc_write_calibration_to_flash(void);

#endif
