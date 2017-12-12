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

#ifndef TC_H
#define TC_H

#include <stdint.h>
#include "config.h"

void tc_channel_init(TcChannel *channel, const uint32_t mode);

#define tc_channel_start(ch) (ch)->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG
#define tc_channel_stop(ch)  (ch)->TC_CCR = TC_CCR_CLKDIS

#define tc_channel_is_enabled(ch)          ((ch)->TC_SR & TC_SR_CLKSTA)
#define tc_channel_interrupt_ack(ch)       (ch)->TC_SR
#define tc_channel_interrupt_set(ch, mode) (ch)->TC_IER = (mode)

#endif


