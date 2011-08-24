/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <pio/pio.h>

#include "bricklib/com/usb/usb_descriptors.h"
#include "bricklib/drivers/uid/uid.h"

#define SIZE_OF_MESSAGE_TYPE 4

#define TYPE_GET_ADC_CALIBRATION 250
#define TYPE_ADC_CALIBRATE 251
#define TYPE_STACK_ENUMERATE 252
#define TYPE_ENUMERATE_CALLBACK 253
#define TYPE_ENUMERATE 254
#define TYPE_GET_STACK_ID 255

#define COM_GENERAL_TYPE_MAX 200
#define MAX_LENGTH_NAME 40

#define COM_NO_MESSAGE {0, NULL}

#define STACK_PARTICIPANT_PLUG 1
#define STACK_PARTICIPANT_UNPLUG 2

typedef void (*message_handler_func_t)(uint8_t, const void*);

typedef struct {
	uint8_t type;
	message_handler_func_t reply_func;
} ComMessage;

/*
typedef struct {
	uint8_t stack_address;
	uint8_t type;
	uint8_t priority;
	uint16_t line;
	uint16_t file_length;
	uint16_t message_length;
	char file_and_message[];
} GetLogging;
*/

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
} __attribute__((packed)) GetADCCalibration;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	int16_t offset;
	int16_t gain;
} __attribute__((packed)) GetADCCalibrationReturn;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	char bricklet_port;
} __attribute__((packed)) ADCCalibrate;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	uint8_t stack_id_upto;
} __attribute__((packed)) StackEnumerateReturn;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	uint8_t stack_id_start;
} __attribute__((packed)) StackEnumerate;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	uint64_t device_uid;
	char device_name[MAX_LENGTH_NAME];
	uint8_t device_stack_id;
	bool new;
} __attribute__((__packed__)) EnumerateCallback;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
} __attribute__((__packed__)) Enumerate;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	uint64_t uid;
} __attribute__((__packed__)) GetStackID;

typedef struct {
	uint8_t stack_id;
	uint8_t type;
	uint16_t length;
	uint64_t device_uid;
	uint8_t device_stack_id;
} __attribute__((__packed__)) GetStackIDReturn;

const ComMessage* get_com_from_data(const char *data);
uint16_t get_length_from_data(const char *data);
uint8_t get_stack_id_from_data(const char *data);
uint8_t get_type_from_data(const char *data);

void get_adc_calibration(uint8_t com, const GetADCCalibration *data);
void com_adc_calibrate(uint8_t com, const ADCCalibrate *data);
void stack_enumerate(uint8_t com, const StackEnumerate *data);
void enumerate(uint8_t com, const Enumerate *data);
void get_stack_id(uint8_t com, const GetStackID *data);


#endif
