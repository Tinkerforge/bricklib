/* bricklib
 * Copyright (C) 2009-2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb.h: Communication interface implementation for USB
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

#ifndef USB_H
#define USB_H

#include "usb_descriptors.h"

#include <usb/USBD.h>
#include <pio/pio.h>

#include "bricklib/com/com_messages.h"
#include "bricklib/com/com.h"
#include "bricklib/com/com_common.h"

#define NUM_RECEIVE_TRIES 10000
#define NUM_SEND_TRIES 100000
#define NUM_CALLBACK_TRIES 10000


void usb_send_callback(void *arg,
                       uint8_t status,
                       uint32_t transferred,
                       uint32_t remaining);

void usb_recv_callback(void *arg,
                       uint8_t status,
                       uint32_t transferred,
                       uint32_t remaining);

// buffered send
//void usb_send_loop(void *parameters);

uint16_t usb_send_implementation(const void *data, const uint16_t length, const uint8_t ep) __attribute__((warn_unused_result));
uint16_t usb_recv_implementation(void *data, const uint16_t length, const uint8_t ep) __attribute__((nonnull(1), warn_unused_result));

uint16_t usb_send_ee(const void *data, const uint16_t length) __attribute__((warn_unused_result));
uint16_t usb_recv_ee(void *data, const uint16_t length) __attribute__((nonnull(1), warn_unused_result));

uint16_t usb_send(const void *data, const uint16_t length) __attribute__((warn_unused_result));
uint16_t usb_recv(void *data, const uint16_t length) __attribute__((nonnull(1), warn_unused_result));

void usb_isr_vbus(const Pin *pin);
void usb_detect_configure(void);
void usb_configure_clock_48mhz(void);
bool usb_is_connected(void);
bool usb_init();
void usb_message_loop(void *parameters);

void usb_message_loop_return(char *data, uint16_t length);

void USBDCallbacks_RequestReceived(const USBGenericRequest *request);

#endif
