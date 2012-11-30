/* bricklib
 * Copyright (C) 2009-2012 Olaf Lüke <olaf@tinkerforge.com>
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
#include "bricklib/drivers/usb/USBD.h"

#include "bricklib/utility/init.h"
#include "bricklib/logging/logging.h"
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

extern int32_t adc_offset;
extern int32_t adc_gain_div;

extern ComInfo com_info;
extern BrickletSettings bs[];

extern uint8_t brick_hardware_version[];

#ifndef COM_MESSAGES_USER
#define COM_MESSAGES_USER
#endif

const ComMessage com_messages[] = {
	COM_NO_MESSAGE,
	COM_MESSAGES_USER
	{FID_GET_CHIP_TEMPERATURE, (message_handler_func_t)get_chip_temperature},
	{FID_RESET, (message_handler_func_t)reset},
	COM_MESSAGES_BRICKLET
	{FID_GET_ADC_CALIBRATION, (message_handler_func_t)get_adc_calibration},
	{FID_ADC_CALIBRATE, (message_handler_func_t)com_adc_calibrate},
	{FID_STACK_ENUMERATE, (message_handler_func_t)stack_enumerate},
	{FID_ENUMERATE_CALLBACK, (message_handler_func_t)NULL},
	{FID_ENUMERATE, (message_handler_func_t)enumerate},
	{FID_GET_IDENTITY, (message_handler_func_t)get_identity}
};

const uint8_t COM_MESSAGES_NUM = (sizeof(com_messages) / sizeof(ComMessage));

const ComMessage* get_com_from_header(const MessageHeader *header) {
	uint8_t fid = 0;
	if(header->fid < COM_GENERAL_FID_MAX) {
		fid = header->fid;
	} else {
		fid = COM_MESSAGES_NUM - 1 - 255 + header->fid;
	}

	if(fid > COM_MESSAGES_NUM) {
		return NULL;
	}

	return &com_messages[fid];
}

void reset(const ComType com, const Reset *data) {
	com_return_setter(com, data);

	// TODO: Protocol V2.0 -> wait until msg is send?

	brick_reset();
}

void get_chip_temperature(const ComType com, const GetChipTemperature *data) {
	GetChipTemperatureReturn gctr;

	gctr.header          = data->header;
	gctr.header.length   = sizeof(GetChipTemperatureReturn);
	gctr.temperature     = adc_get_temperature();

	send_blocking_with_timeout(&gctr, sizeof(GetChipTemperatureReturn), com);
}

void get_adc_calibration(const ComType com, const GetADCCalibration *data) {
	GetADCCalibrationReturn gadccr;
	gadccr.header        = data->header;
	gadccr.header.length = sizeof(GetADCCalibrationReturn);
	gadccr.offset        = adc_offset;
	gadccr.gain          = adc_gain_div;

	send_blocking_with_timeout(&gadccr, sizeof(GetADCCalibrationReturn), com);
	logd("Get ADC Calibration (offset, gain): %ld, %lu\n\r", adc_offset, adc_gain_div);
}

void com_adc_calibrate(const ComType com, const ADCCalibrate *data) {
	uint8_t port = (uint8_t)(tolower((uint8_t)data->bricklet_port) - 'a');
	if(port >= BRICKLET_NUM) {
		com_return_error(data, sizeof(MessageHeader), MESSAGE_ERROR_CODE_INVALID_PARAMETER, com);
		logblete("Bricklet Port %d does not exist (adc calibrate)\n\r", port);
		return;
	}

	adc_calibrate(bs[port].adc_channel);
	adc_write_calibration_to_flash();

	com_return_setter(com, data);
	logd("ADC Calibrate (port %c)\n\r", data->bricklet_port);
}

void stack_enumerate(const ComType com, const StackEnumerate *data) {
	StackEnumerateReturn ser = MESSAGE_EMPTY_INITIALIZER;
	ser.header               = data->header;
	ser.header.length        = sizeof(StackEnumerateReturn);
	ser.uids[0]              = com_info.uid;

	uint8_t i = 1;
	for(uint8_t j = 0; j < BRICKLET_NUM; j++) {
		if(bs[j].uid != 0) {
			ser.uids[i]      = bs[j].uid;
			i++;
		}
	}

	com_info.current = com;

	send_blocking_with_timeout(&ser, sizeof(StackEnumerateReturn), com);
	logd("Stack Enumerate: %lu %lu %lu %lu %lu\n\r", ser.uids[0], ser.uids[1], ser.uids[2], ser.uids[3], ser.uids[4]);

	while(!brick_init_enumeration(com));
}


void make_brick_enumerate(EnumerateCallback *ec) {
	com_make_default_header(ec, com_info.uid, sizeof(EnumerateCallback), FID_ENUMERATE_CALLBACK);

	memset(ec->uid, '\0', UID_STR_MAX_LENGTH);
	uid_to_serial_number(com_info.uid, ec->uid);
	strncpy(ec->connected_uid, NO_CONNECTED_UID_STR, NO_CONNECTED_UID_STR_LENGTH);
	ec->position = '0';
	ec->version_fw[0] = BRICK_FIRMWARE_VERSION_MAJOR;
	ec->version_fw[1] = BRICK_FIRMWARE_VERSION_MINOR;
	ec->version_fw[2] = BRICK_FIRMWARE_VERSION_REVISION;
	ec->version_hw[0] = brick_hardware_version[0];
	ec->version_hw[1] = brick_hardware_version[1];
	ec->version_hw[2] = brick_hardware_version[2];
	ec->device_identifier = BRICK_DEVICE_IDENTIFIER;
}

void make_bricklet_enumerate(EnumerateCallback *ec, const uint8_t bricklet) {
	com_make_default_header(ec, bs[bricklet].uid, sizeof(EnumerateCallback), FID_ENUMERATE_CALLBACK);

	memset(ec->uid, '\0', UID_STR_MAX_LENGTH);
	uid_to_serial_number(bs[bricklet].uid, ec->uid);

	memset(ec->connected_uid, '\0', UID_STR_MAX_LENGTH);
	uid_to_serial_number(com_info.uid, ec->connected_uid);
	ec->position = 'a' + bricklet;
	ec->version_fw[0] = bs[bricklet].firmware_version[0];
	ec->version_fw[1] = bs[bricklet].firmware_version[1];
	ec->version_fw[2] = bs[bricklet].firmware_version[2];
	ec->version_hw[0] = bs[bricklet].hardware_version[0];
	ec->version_hw[1] = bs[bricklet].hardware_version[1];
	ec->version_hw[2] = bs[bricklet].hardware_version[2];
	ec->device_identifier = bs[bricklet].device_identifier;
}

void enumerate(const ComType com, const Enumerate *data) {
	logd("Returning Enumeration for Brick: %d\n\r", com);

	// Enumerate Brick
	EnumerateCallback ec = MESSAGE_EMPTY_INITIALIZER;
	make_brick_enumerate(&ec);

	ec.enumeration_type = ENUMERATE_TYPE_AVAILABLE;

	send_blocking_with_timeout(&ec, sizeof(EnumerateCallback), com);

	// Enumerate Bricklet
	for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
		if(bs[i].uid == 0) {
			continue;
		}

		logd("Returning Enumeration for Bricklet %c\n\r", 'a' + i);

		EnumerateCallback ec = MESSAGE_EMPTY_INITIALIZER;
		make_bricklet_enumerate(&ec, i);

		ec.enumeration_type = ENUMERATE_TYPE_AVAILABLE;

		send_blocking_with_timeout(&ec, sizeof(EnumerateCallback), com);
	}

	com_info.current = com;
}

void get_identity(const ComType com, const GetIdentity *data) {
	logd("GetIdentitiy for %lu\n\r", data->header.uid);
	GetIdentityReturn gir = MESSAGE_EMPTY_INITIALIZER;
	gir.header        = data->header;
	gir.header.length = sizeof(GetIdentityReturn);

	if(data->header.uid == com_info.uid) {
		memset(gir.uid, '\0', UID_STR_MAX_LENGTH);
		uid_to_serial_number(com_info.uid, gir.uid);
		strncpy(gir.connected_uid, NO_CONNECTED_UID_STR, NO_CONNECTED_UID_STR_LENGTH);
		gir.position = '0';
		gir.version_fw[0] = BRICK_FIRMWARE_VERSION_MAJOR;
		gir.version_fw[1] = BRICK_FIRMWARE_VERSION_MINOR;
		gir.version_fw[2] = BRICK_FIRMWARE_VERSION_REVISION;
		gir.version_hw[0] = brick_hardware_version[0];
		gir.version_hw[1] = brick_hardware_version[1];
		gir.version_hw[2] = brick_hardware_version[2];
		gir.device_identifier = BRICK_DEVICE_IDENTIFIER;
	} else {
		for(uint8_t i = 0; i < BRICKLET_NUM; i++) {
			if(bs[i].uid == data->header.uid) {
				memset(gir.uid, '\0', UID_STR_MAX_LENGTH);
				uid_to_serial_number(bs[i].uid, gir.uid);

				memset(gir.connected_uid, '\0', UID_STR_MAX_LENGTH);
				uid_to_serial_number(com_info.uid, gir.connected_uid);
				gir.position = 'a' + i;
				gir.version_fw[0] = bs[i].firmware_version[0];
				gir.version_fw[1] = bs[i].firmware_version[1];
				gir.version_fw[2] = bs[i].firmware_version[2];
				gir.version_hw[0] = bs[i].hardware_version[0];
				gir.version_hw[1] = bs[i].hardware_version[1];
				gir.version_hw[2] = bs[i].hardware_version[2];
				gir.device_identifier = bs[i].device_identifier;

				break;
			}
		}
	}

	if(gir.device_identifier != 0) {
		send_blocking_with_timeout(&gir, sizeof(GetIdentityReturn), com);
	}
}
