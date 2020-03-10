/* bricklib
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bricklet_xmc.h: Functions to write bootloader/bootstrapper to XMC1xxx
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

#ifndef BRICKLET_XMC_H
#define BRICKLET_XMC_H

#include <stdint.h>
#include <stdbool.h>

uint32_t bricklet_xmc_flash_data(const uint8_t *data);
uint32_t bricklet_xmc_flash_config(uint32_t config, uint32_t parameter1, uint32_t parameter2, const uint8_t *data);

#endif