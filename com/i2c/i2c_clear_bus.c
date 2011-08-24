/* bricklib
 * Copyright (C) 2011 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_clear_bus.c: Clear Bus implementation (3.16 I2C specification)
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

#include "i2c_clear_bus.h"

#include "bricklib/drivers/pio/pio.h"
#include "bricklib/utility/util_definitions.h"

void i2c_clear_bus_delay(void) {
	SLEEP_US(10);
}

void i2c_clear_bus_9clock(Pin *sda, Pin *scl) {
	scl->type = PIO_OUTPUT_0;
	scl->attribute = PIO_DEFAULT;
	PIO_Configure(scl, 1);
	i2c_clear_bus_delay();

	sda->type = PIO_INPUT;
	sda->attribute = PIO_PULLUP;
	PIO_Configure(sda, 1);
	i2c_clear_bus_delay();

	for(uint8_t i = 0; i < 9; i++) {
		scl->type = PIO_INPUT;
		scl->attribute = PIO_PULLUP;
		PIO_Configure(scl, 1);
		i2c_clear_bus_delay();

		scl->type = PIO_OUTPUT_0;
		scl->attribute = PIO_DEFAULT;
		PIO_Configure(scl, 1);
		i2c_clear_bus_delay();
	}
}

void i2c_clear_bus_stop(Pin *sda, Pin *scl) {
	scl->type = PIO_OUTPUT_0;
	scl->attribute = PIO_DEFAULT;
	PIO_Configure(scl, 1);
	i2c_clear_bus_delay();

	sda->type = PIO_OUTPUT_0;
	sda->attribute = PIO_DEFAULT;
	PIO_Configure(sda, 1);
	i2c_clear_bus_delay();

	scl->type = PIO_INPUT;
	scl->attribute = PIO_PULLUP;
	PIO_Configure(scl, 1);
	i2c_clear_bus_delay();

	sda->type = PIO_INPUT;
	sda->attribute = PIO_DEFAULT;
	PIO_Configure(sda, 1);
	i2c_clear_bus_delay();
}

void i2c_clear_bus_start(Pin *sda, Pin *scl) {
	sda->type = PIO_INPUT;
	sda->attribute = PIO_DEFAULT;
	PIO_Configure(sda, 1);
	i2c_clear_bus_delay();

	scl->type = PIO_INPUT;
	scl->attribute = PIO_PULLUP;
	PIO_Configure(scl, 1);
	i2c_clear_bus_delay();

	sda->type = PIO_OUTPUT_0;
	sda->attribute = PIO_DEFAULT;
	PIO_Configure(sda, 1);
	i2c_clear_bus_delay();

	scl->type = PIO_OUTPUT_0;
	scl->attribute = PIO_DEFAULT;
	PIO_Configure(scl, 1);
	i2c_clear_bus_delay();
}

void i2c_clear_bus(Pin *sda, Pin *scl) {
	i2c_clear_bus_start(sda, scl);
	i2c_clear_bus_9clock(sda, scl);
	i2c_clear_bus_start(sda, scl);
	i2c_clear_bus_stop(sda, scl);
}
