/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ringbuffer.h: General ringbuffer implementation
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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t overflows;
	uint16_t start;
	uint16_t end;
	uint16_t size;
	uint16_t low_watermark;
	uint8_t *buffer;
} Ringbuffer;

uint16_t ringbuffer_get_used(Ringbuffer *rb);
uint16_t ringbuffer_get_free(Ringbuffer *rb);
bool ringbuffer_is_empty(Ringbuffer *rb);
bool ringbuffer_is_full(Ringbuffer *rb);
bool ringbuffer_add(Ringbuffer *rb, const uint8_t data);
void ringbuffer_remove(Ringbuffer *rb, const uint16_t num);
bool ringbuffer_get(Ringbuffer *rb, uint8_t *data);
void ringbuffer_init(Ringbuffer *rb, const uint16_t size, uint8_t *buffer);
void ringbuffer_print(Ringbuffer *rb);

#endif
