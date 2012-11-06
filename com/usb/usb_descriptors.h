/* bricklib
 * Copyright (C) 2010-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_descriptors.h: USB descriptor definitions
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

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <usb/USBDescriptors.h>
#include <stdint.h>

#define DEFAULT_EP_SIZE 80

#define IN_EP     4
#define OUT_EP    5

typedef struct BrickConfigurationDescriptors {
    USBConfigurationDescriptor configuration;
    USBInterfaceDescriptor interface;
    USBEndpointDescriptor endpoint_in;
    USBEndpointDescriptor endpoint_out;
} EEConfigurationDescriptors;

void set_serial_number_descriptor(char *sn, uint8_t length);

#endif
