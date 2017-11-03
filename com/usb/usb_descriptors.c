/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_descriptors.c: USB descriptor definitions
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

#include "usb_descriptors.h"

#include "bricklib/drivers/usb/USBDescriptors.h"
#include "bricklib/drivers/usb/USBRequests.h"
#include "bricklib/drivers/usb/USBD.h"
#include "bricklib/drivers/usb/USBDDriver.h"
#include <string.h>

#include "bricklib/utility/util_definitions.h"
#include "bricklib/drivers/uid/uid.h"

#include "config.h"

static const USBDeviceDescriptor device_descriptor = {
    sizeof(USBDeviceDescriptor),
    USBGenericDescriptor_DEVICE,
    USBDeviceDescriptor_USB2_00,
    0xFF,
    0x00,
    0x00,
    64, // Max size
    0x16D0,
    0x063D,
    0x110,
    1, // Manufacturer string descriptor index.
    2, // Product string descriptor index.
    3, // Serial number string descriptor index.
    1  // Device has one possible configuration.
};

const struct BrickConfigurationDescriptors configuration_descriptor = {
	{
		sizeof(USBConfigurationDescriptor),
		USBGenericDescriptor_CONFIGURATION,
		sizeof(EEConfigurationDescriptors),
		1,
		1,
		0,
		// TODO: change to USBConfigurationDescriptor_BUSPOWERED_RWAKEUP
		//       for own boards!
		//USBConfigurationDescriptor_SELFPOWERED_RWAKEUP,
		USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP,
		USBConfigurationDescriptor_POWER(500)
	},
    // Interface descriptor
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        0x00, // This is interface #0
        0x00, // This is setting #0 for interface
        2, // Interface has one endpoint
        0xFF, // Verdor specific interface class code
        0x00, // No interface subclass code
        0x00, // No interface protocol code
        0, // No string descriptor
    },
    // Endpoint descriptor in
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN, IN_EP),
        USBEndpointDescriptor_BULK,
        DEFAULT_EP_SIZE, //CHIP_USB_ENDPOINTS_MAXPACKETSIZE(4),
        // TODO: USB spec does not specify what this means for bulk?.
        //       Also it seems often to be set to 0 in bulk transfers?
        //       (see lsusb)
        0 // Maximum number of milliseconds between transaction attempts
    },
	// Endpoint descriptor out
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT, OUT_EP),
		USBEndpointDescriptor_BULK,
		DEFAULT_EP_SIZE, //CHIP_USB_ENDPOINTS_MAXPACKETSIZE(5),
        // TODO: USB spec does not specify what this means for bulk?.
        //       Also it seems often to be set to 0 in bulk transfers?
        //       (see lsusb)
		0 // Maximum number of milliseconds between transaction attempts

	}
};

static const unsigned char language_id_descriptor[] = {
    USBStringDescriptor_LENGTH(1),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_ENGLISH_US
};

static const unsigned char manufacturer_descriptor[] = {
    USBStringDescriptor_LENGTH(16),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('T'),
    USBStringDescriptor_UNICODE('i'),
    USBStringDescriptor_UNICODE('n'),
    USBStringDescriptor_UNICODE('k'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('f'),
    USBStringDescriptor_UNICODE('o'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('g'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE(' '),
    USBStringDescriptor_UNICODE('G'),
    USBStringDescriptor_UNICODE('m'),
    USBStringDescriptor_UNICODE('b'),
    USBStringDescriptor_UNICODE('H'),
};

#define SERIAL_NUMBER_LENGTH USBStringDescriptor_LENGTH(MAX_BASE58_STR_SIZE)

static const unsigned char product_descriptor[] = PRODUCT_DESCRIPTOR;
static unsigned char serial_number_descriptor[SERIAL_NUMBER_LENGTH] = {
	SERIAL_NUMBER_LENGTH,
	USBGenericDescriptor_STRING
};

void set_serial_number_descriptor(char *sn, const uint8_t length) {
	serial_number_descriptor[0] = USBStringDescriptor_LENGTH(length);

	for(uint8_t i = 0; i < MIN(MAX_BASE58_STR_SIZE, length); i++) {
		serial_number_descriptor[2+i*2] = sn[i];
		serial_number_descriptor[2+i*2+1] = 0;
	}
}

// List of all string descriptors used.
static const unsigned char *string_descriptors[] = {
    language_id_descriptor,
    manufacturer_descriptor,
    product_descriptor,
    serial_number_descriptor
};

// List of the standard descriptors used by the Mass Storage driver.
const USBDDriverDescriptors driver_descriptors = {
    &device_descriptor,
    (const USBConfigurationDescriptor *) &configuration_descriptor,
    0, // No full-speed device qualifier descriptor
    0, // No full-speed other speed configuration
    0, // No high-speed device descriptor
    0, // No high-speed configuration descriptor
    0, // No high-speed device qualifier descriptor
    0, // No high-speed other speed configuration descriptor
    string_descriptors,
    4 // Four string descriptors in array
};
