/* bricklib
 * Copyright (C) 2009-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * com_messages.h: Implementation of general brick messages
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

#ifndef COM_MESSAGES_H
#define COM_MESSAGES_H

#include "com.h"
#include "com_common.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "bricklib/com/usb/usb_descriptors.h"
#include "bricklib/drivers/uid/uid.h"
#include "bricklib/drivers/pio/pio.h"

#define SIZE_OF_MESSAGE_HEADER 8

#define FID_ENABLE_STATUS_LED 238
#define FID_DISABLE_STATUS_LED 239
#define FID_IS_STATUS_LED_ENABLED 240

#define FID_GET_PROTOCOL1_BRICKLET_NAME 241
#define FID_GET_CHIP_TEMPERATURE 242
#define FID_RESET 243

#define FID_GET_ADC_CALIBRATION 250
#define FID_ADC_CALIBRATE 251
#define FID_STACK_ENUMERATE 252
#define FID_ENUMERATE_CALLBACK 253
#define FID_ENUMERATE 254
#define FID_GET_IDENTITY 255

#define COM_GENERAL_FID_MAX 200
#define MAX_LENGTH_NAME 40

#define COM_NO_MESSAGE {0, NULL}

#define STACK_PARTICIPANT_PLUG 1
#define STACK_PARTICIPANT_UNPLUG 2

#define STACK_ENUMERATE_MAX_UIDS 16

#define ENUMERATE_TYPE_AVAILABLE 0
#define ENUMERATE_TYPE_ADDED     1
#define ENUMERATE_TYPE_REMOVED   2

#define NO_CONNECTED_UID_STR         "0\0\0\0\0\0\0\0"
#define NO_CONNECTED_UID_STR_LENGTH  sizeof(NO_CONNECTED_UID_STR)

#define UID_STR_MAX_LENGTH MAX_BASE58_STR_SIZE

typedef void (*message_handler_func_t)(uint8_t, const void*);

typedef struct {
	uint8_t type;
	message_handler_func_t reply_func;
} ComMessage;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) EnableStatusLED;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) DisableStatusLED;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) IsStatusLEDEnabled;

typedef struct {
	MessageHeader header;
	bool enabled;
} __attribute__((__packed__)) IsStatusLEDEnabledReturn;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) GetChipTemperature;

typedef struct {
	MessageHeader header;
	int16_t temperature;
} __attribute__((__packed__)) GetChipTemperatureReturn;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) Reset;

typedef struct {
	MessageHeader header;
} __attribute__((packed)) GetADCCalibration;

typedef struct {
	MessageHeader header;
	int16_t offset;
	int16_t gain;
} __attribute__((packed)) GetADCCalibrationReturn;

typedef struct {
	MessageHeader header;
	char bricklet_port;
} __attribute__((packed)) ADCCalibrate;

typedef struct {
	MessageHeader header;
	uint32_t uids[STACK_ENUMERATE_MAX_UIDS];
} __attribute__((packed)) StackEnumerateReturn;

typedef struct {
	MessageHeader header;
} __attribute__((packed)) StackEnumerate;

typedef struct {
	MessageHeader header;
	char uid[UID_STR_MAX_LENGTH];
	char connected_uid[UID_STR_MAX_LENGTH];
	char position;
	uint8_t version_hw[3];
	uint8_t version_fw[3];
	uint16_t device_identifier;
	uint8_t enumeration_type;
} __attribute__((__packed__)) EnumerateCallback;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) Enumerate;

typedef struct {
	MessageHeader header;
} __attribute__((__packed__)) GetIdentity;

typedef struct {
	MessageHeader header;
	char uid[UID_STR_MAX_LENGTH];
	char connected_uid[UID_STR_MAX_LENGTH];
	char position;
	uint8_t version_hw[3];
	uint8_t version_fw[3];
	uint16_t device_identifier;
} __attribute__((__packed__)) GetIdentityReturn;


typedef struct {
	MessageHeader header;
	char port;
} __attribute__((__packed__)) GetProtocol1BrickletName;

typedef struct {
	MessageHeader header;
	uint8_t protocol_version;
	uint8_t firmware_version[3];
	char name[40];
} __attribute__((__packed__)) GetProtocol1BrickletNameReturn;

const ComMessage* get_com_from_header(const MessageHeader *header);
uint16_t get_length_from_data(const char *data);
uint8_t get_stack_id_from_data(const char *data);
uint8_t get_type_from_data(const char *data);

void enable_status_led(const ComType com, const EnableStatusLED *data);
void disable_status_led(const ComType com, const DisableStatusLED *data);
void is_status_led_enabled(const ComType com, const IsStatusLEDEnabled *data);

void get_protocol1_bricklet_name(const ComType com, const GetProtocol1BrickletName *data);
void reset(const ComType com, const Reset *data);
void get_chip_temperature(const ComType com, const GetChipTemperature *data);
void get_adc_calibration(const ComType com, const GetADCCalibration *data);
void com_adc_calibrate(const ComType com, const ADCCalibrate *data);
void stack_enumerate(const ComType com, const StackEnumerate *data);
void enumerate(const ComType com, const Enumerate *data);
void get_identity(const ComType com, const GetIdentity *data);
void make_brick_enumerate(EnumerateCallback *ec);
void make_bricklet_enumerate(EnumerateCallback *ec, const uint8_t bricklet);


#endif
