/* bricklib
 * Copyright (C) 2011-2012 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tc.h: Implementation of tc access
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

#include "tc.h"

#include <stdint.h>
#include "config.h"

void tc_channel_init(TcChannel *channel, const uint32_t mode) {
	channel->TC_CCR = TC_CCR_CLKDIS;
	channel->TC_IDR = 0xFFFFFFFF;
	channel->TC_SR;
	channel->TC_CMR = mode;
}
