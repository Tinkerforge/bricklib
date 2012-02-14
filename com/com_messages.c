/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * com_messages.c: Implementation of general brick messages
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

#include "com_messages.h"

#include <string.h>
#include <usb/USBD.h>

#include "bricklib/utility/init.h"
#include "bricklib/logging/logging.h"
#include "bricklib/drivers/board/cpu.h"
#include "bricklib/drivers/pio/pio.h"
#include "bricklib/drivers/adc/adc.h"
#include "bricklib/bricklet/bricklet_communication.h"
#include "bricklib/bricklet/bricklet_config.h"
#include "bricklib/bricklet/bricklet_init.h"

#ifdef BRICK_CAN_BE_MASTER
#include "extensions/extension_init.h"
#endif

#include "com_common.h"
#include "config.h"

extern int16_t adc_offset;
extern int16_t adc_gain;

extern uint8_t master_mode;

extern uint8_t com_last_ext_id[];
extern uint8_t master_routing_table[];
extern uint8_t com_last_spi_stack_id;
extern uint8_t com_stack_id;
extern uint64_t com_brick_uid;
extern uint8_t com_last_stack_address;
extern ComType com_current;
extern ComType com_ext[];
extern BrickletSettings bs[];
extern const BrickletAddress baddr[];

#ifndef COM_MESSAGES_USER
#define COM_MESSAGES_USER
#endif

const ComMessage com_messages[] = {
	COM_NO_MESSAGE,
	COM_MESSAGES_USER
	COM_MESSAGES_BRICKLET
	{TYPE_GET_ADC_CALIBRATION, (message_handler_func_t)get_adc_calibration},
	{TYPE_ADC_CALIBRATE, (message_handler_func_t)com_adc_calibrate},
	{TYPE_STACK_ENUMERATE, (message_handler_func_t)stack_enumerate},
	{TYPE_ENUMERATE_CALLBACK, (message_handler_func_t)NULL},
	{TYPE_ENUMERATE, (message_handler_func_t)enumerate},
	{TYPE_GET_STACK_ID, (message_handler_func_t)get_stack_id}
};

const uint8_t COM_MESSAGES_NUM = (sizeof(com_messages) / sizeof(ComMessage));

const ComMessage* get_com_from_data(const char *data) {
	const uint8_t type = get_type_from_data(data);
	if(type < COM_GENERAL_TYPE_MAX) {
		return &com_messages[type];
	} else {
		return &com_messages[COM_MESSAGES_NUM - 1 - 255 + type];
	}
}

inline uint16_t get_length_from_data(const char *data) {
	return *((uint16_t *)(data + 2));
}

inline uint8_t get_stack_id_from_data(const char *data) {
	return data[0];
}

inline uint8_t get_type_from_data(const char *data) {
	return data[1];
}

void get_adc_calibration(uint8_t com, const GetADCCalibration *data) {
	GetADCCalibrationReturn gadccr = {
		com_stack_id,
		TYPE_GET_ADC_CALIBRATION,
		sizeof(GetADCCalibrationReturn),
		adc_offset,
		adc_gain
	};

	send_blocking_with_timeout(&gadccr, sizeof(GetADCCalibrationReturn), com);
	logd("Get ADC Calibration (offset, gain): %d %d\n\r", adc_offset, adc_gain);
}

void com_adc_calibrate(uint8_t com, const ADCCalibrate *data) {
	uint8_t port = (uint8_t)(tolower(data->bricklet_port) - 'a');
	if(port >= BRICKLET_NUM) {
		// TODO: Error?
		logblete("Bricklet Port %d does not exist (adc calibrate)\n\r", port);
		return;
	}

	adc_calibrate(bs[port].adc_channel);
	adc_write_calibration_to_flash();
	logd("ADC Calibrate (port %c)\n\r", data->bricklet_port);
}

void stack_enumerate(uint8_t com, const StackEnumerate *data) {
	com_stack_id = data->stack_id_start;

	uint8_t stack_id_upto = com_stack_id;

	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].uid != 0) {
			stack_id_upto++;
			bs[i].stack_id = stack_id_upto;
		}
	}


	com_current = com;

#ifdef BRICK_CAN_BE_MASTER
	if(master_mode & MASTER_MODE_MASTER) {
		// make new routing table

		int16_t id_diff = 0;
		for(uint16_t i = 0; i <= com_last_spi_stack_id; i++) {
			if(master_routing_table[i] != 0) {
				id_diff = stack_id_upto + 1 - i;
				break;
			}
		}

		if(id_diff != 0) {
			uint8_t sa = 0;
			for(uint16_t i = 0; i <= com_last_spi_stack_id; i++) {

				if(master_routing_table[i] != sa) {
					sa++;

					StackEnumerate se = {
						i,
						TYPE_STACK_ENUMERATE,
						sizeof(StackEnumerate),
						i + id_diff
					};

					send_blocking_with_timeout(&se,
											   sizeof(StackEnumerate),
											   COM_SPI_STACK);

				}
			}

			com_last_spi_stack_id += id_diff;

			if(id_diff > 0) {
				for(int16_t i = MAX_STACK_IDS - 1; i >= id_diff; i--) {
					master_routing_table[i] = master_routing_table[i - id_diff];
				}
				for(int16_t i = 0; i < id_diff; i++) {
					master_routing_table[i] = 0;
				}
			} else {
				for(int16_t i = 0; i < MAX_STACK_IDS + id_diff; i++) {
					master_routing_table[i] = master_routing_table[i - id_diff];
				}
				for(int16_t i = MAX_STACK_IDS + id_diff; i < MAX_STACK_IDS; i++) {
					master_routing_table[i] = 0;
				}
			}
		}

		if(com_last_spi_stack_id > stack_id_upto) {
			stack_id_upto = com_last_spi_stack_id;
		}

	}
#endif

	StackEnumerateReturn ser = {
		data->stack_id,
		TYPE_STACK_ENUMERATE,
		sizeof(StackEnumerateReturn),
		stack_id_upto,
	};

	send_blocking_with_timeout(&ser, sizeof(StackEnumerateReturn), com);
	logd("Stack Enumerate: %d to %d\n\r", com_stack_id, stack_id_upto);
}

void enumerate(uint8_t com, const Enumerate *data) {
	logd("Returning Enumeration for Brick: %d\n\r", com);

	// Enumerate Brick
	EnumerateCallback ec = {
		0,
		TYPE_ENUMERATE_CALLBACK,
		sizeof(EnumerateCallback),
		com_brick_uid,
		BRICK_HARDWARE_NAME,
		com_stack_id,
		true
	};

	send_blocking_with_timeout(&ec, sizeof(EnumerateCallback), com);

	// Enumerate Bricklet
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].uid == 0) {
			continue;
		}

		logd("Returning Enumeration for Bricklet %c\n\r", 'a' + i);

		EnumerateCallback ec = {
			0,
			TYPE_ENUMERATE_CALLBACK,
			sizeof(EnumerateCallback),
			bs[i].uid,
			"",
			bs[i].stack_id,
			true
		};

		strncpy(ec.device_name,
		        bs[i].name,
		        MAX_LENGTH_NAME);

		send_blocking_with_timeout(&ec, sizeof(EnumerateCallback), com);
	}
	com_current = com;

#ifdef BRICK_CAN_BE_MASTER
	if(master_mode & MASTER_MODE_MASTER) {
		// Enumerate Stack
		for(uint8_t i = 1; i <= com_last_stack_address; i++) {
			uint16_t id;
			for(id = 0; id <= com_last_spi_stack_id; id++) {
				if(master_routing_table[id] == i) {
					break;
				}
			}

			Enumerate e = {
				id,
				TYPE_ENUMERATE,
				sizeof(Enumerate),
			};
			send_blocking_with_timeout(&e, sizeof(Enumerate), COM_SPI_STACK);
		}

		extension_enumerate(com, data);
	}
#endif
}

void get_stack_id(uint8_t com, const GetStackID *data) {
	logd("Get UID: %d\n\r", data->uid);
	GetStackIDReturn gsidr;

	gsidr.stack_id        = 0;
	gsidr.type            = data->type;
	gsidr.length          = sizeof(GetStackIDReturn);
	gsidr.device_uid      = data->uid;

	// Check own Stack ID
	if(data->uid == com_brick_uid) {
		gsidr.device_stack_id = com_stack_id;
		gsidr.device_firmware_version[0] = BRICK_FIRMWARE_VERSION_MAJOR;
		gsidr.device_firmware_version[1] = BRICK_FIRMWARE_VERSION_MINOR;
		gsidr.device_firmware_version[2] = BRICK_FIRMWARE_VERSION_REVISION;
		memset(gsidr.device_name, 0, MAX_LENGTH_NAME);
		strcpy(gsidr.device_name, BRICK_HARDWARE_NAME);
		send_blocking_with_timeout(&gsidr, sizeof(GetStackIDReturn), com);
		return;
	// Check Bricklet Stack ID
	} else {
		for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
			if(data->uid == bs[i].uid) {
				gsidr.device_stack_id = bs[i].stack_id;
				gsidr.device_firmware_version[0] = bs[i].firmware_version[0];
				gsidr.device_firmware_version[1] = bs[i].firmware_version[1];
				gsidr.device_firmware_version[2] = bs[i].firmware_version[2];
				strncpy(gsidr.device_name, bs[i].name, MAX_LENGTH_NAME);

				send_blocking_with_timeout(&gsidr,
				                           sizeof(GetStackIDReturn),
				                           com);
				return;
			}
		}
	}

#ifdef BRICK_CAN_BE_MASTER
	if(master_mode & MASTER_MODE_MASTER) {
		// Check Stack Stack IDs
		for(uint8_t i = 1; i <= com_last_stack_address; i++) {
			uint8_t id;
			for(id = 0; id < MAX_STACK_IDS; id++) {
				if(master_routing_table[id] == i) {
					break;
				}
			}

			GetStackID gsid = {
				id,
				TYPE_GET_STACK_ID,
				sizeof(GetStackID),
				data->uid
			};
			send_blocking_with_timeout(&gsid, sizeof(GetStackID), COM_SPI_STACK);
		}

		extension_stack_id(com, data);
	}
#endif
}
