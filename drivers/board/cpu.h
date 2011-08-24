/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * cpu.h: CPU specific configuration valid for all bricks
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

#ifndef CPU_H
#define CPU_H

#include "config.h"

#if defined(at91sam3s4c) || defined(at91sam3s2c) || defined(at91sam3s1c)
#define PORT_A
#define PORT_B
#define PORT_C
#define NUM_PIN_ON_PORT_A 32
#define NUM_PIN_ON_PORT_B 15
#define NUM_PIN_ON_PORT_C 32
#elif defined(at91sam3s4b) || defined(at91sam3s2b) || defined(at91sam3s1b)
#define PORT_A
#define PORT_B
#define NUM_PIN_ON_PORT_A 32
#define NUM_PIN_ON_PORT_B 15
#elif defined(at91sam3s4a) || defined(at91sam3s2a) || defined(at91sam3s1a)
#define PORT_A
#define PORT_B
#define NUM_PIN_ON_PORT_A 21
#define NUM_PIN_ON_PORT_B 13
#endif

#endif
