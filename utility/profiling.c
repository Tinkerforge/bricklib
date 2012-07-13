/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * profiling.c: Port of FreeRTOS profiling feature
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


#include "profiling.h"

#include "config.h"

#ifdef PROFILING
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>

#include "bricklib/drivers/tc/tc.h"
#include "bricklib/logging/logging.h"

volatile unsigned long ulHighFrequencyTimerTicks = 0UL;

void profiling_init(void) {
    // Enable peripheral clock.
    PMC->PMC_PCER0 = 1 << ID_TC0;

    // Configure TC for PROFILING_FREQUENCY frequency and trigger on RC compare.
    uint32_t div;
    uint32_t tcclks;

    const uint32_t divisors[5] = {2, 8, 32, 128,BOARD_MCK / 32768};

    uint32_t index = 0;

    // Satisfy lower bound
    while(PROFILING_FREQUENCY < ((BOARD_MCK / divisors[index]) / 65536)) {
        index++;
    }

    // Try to maximize DIV while satisfying upper bound
    while(index < 4) {
        if (PROFILING_FREQUENCY > (BOARD_MCK / divisors[index + 1])) {
            break;
        }
        index++;
    }

    div = divisors[index];
    tcclks = index;

    tc_channel_init(&TC0->TC_CHANNEL[0], tcclks | TC_CMR_CPCTRG);
    TC0->TC_CHANNEL[0].TC_RC = (BOARD_MCK / div) / PROFILING_FREQUENCY;

    // Configure and enable interrupt on RC compare
    TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;

    NVIC_DisableIRQ(TC0_IRQn);
    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_SetPriority(TC0_IRQn, PRIORITY_PROFILING_TC0);
    NVIC_EnableIRQ(TC0_IRQn);

    tc_channel_start(&TC0->TC_CHANNEL[0]);
}

void TC0_IrqHandler(void) {
	// Clear status bit to acknowledge interrupt
	volatile uint32_t ack = TC0->TC_CHANNEL[0].TC_SR;
	ulHighFrequencyTimerTicks++;
	if(ulHighFrequencyTimerTicks == PROFILING_FREQUENCY*PROFILING_TIME) {
		signed char stats[1000];
		vTaskGetRunTimeStats(stats);
		logi("%s", stats);
	}
}

#endif
