/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * board_lowlevel.c: Initialize external or internal main oscillator
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

// Based on original Atmel board_lowlevel.c

#include "config.h"

// Oscillator at 16MHz
#if (BOARD_MAINOSC == 16000000)
// Main Clock at 64MHz
#if (BOARD_MCK == 64000000)
#define BOARD_OSCOUNT   (CKGR_MOR_MOSCXTST(0x8))

// Make 128 Mhz clock with prescaler set to 2 (see datasheet p. 1065):
// For Master Clock Frequency between 60 MHz and 64 MHz, the PLLCK must
// be set and used between 120 MHz and 128 MHz with the prescaler set
// at 2 (PRES field in PMC_MCKR).

#define BOARD_PLLAR     (CKGR_PLLAR_STUCKTO1 \
                       | CKGR_PLLAR_MULA(0x7) \
                       | CKGR_PLLAR_PLLACOUNT(0x1) \
                       | CKGR_PLLAR_DIVA(0x1))
#define BOARD_MCKR      (PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)
#elif (BOARD_MCK == 48000000)
#define BOARD_OSCOUNT   (CKGR_MOR_MOSCXTST(0x8))
#define BOARD_PLLAR     (CKGR_PLLAR_STUCKTO1 \
                       | CKGR_PLLAR_MULA(0x05) \
                       | CKGR_PLLAR_PLLACOUNT(0x1) \
                       | CKGR_PLLAR_DIVA(0x1))
#define BOARD_MCKR      (PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)
#endif
#endif

void low_level_init( void ) {
	// Set 3 FWS for Embedded Flash Access
	EFC->EEFC_FMR = EEFC_FMR_FWS(3);

#ifdef BOARD_OSC_EXTERNAL
	// Initialize external main oscillator
	if(!(PMC->CKGR_MOR & CKGR_MOR_MOSCSEL)) {
		PMC->CKGR_MOR = CKGR_MOR_KEY(0x37) |
		                BOARD_OSCOUNT |
		                CKGR_MOR_MOSCRCEN |
		                CKGR_MOR_MOSCXTEN;

		while(!(PMC->PMC_SR & PMC_SR_MOSCXTS));
	}

	PMC->CKGR_MOR = CKGR_MOR_KEY(0x37) | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCSEL;
#else
	// Initialize internal main oscillator
	PMC->CKGR_MOR = CKGR_MOR_KEY(0x37) | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCRCF_12MHZ;
#endif
	while(!(PMC->PMC_SR & PMC_SR_MOSCSELS));

	PMC->PMC_MCKR = (PMC->PMC_MCKR & ~(uint32_t)PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	while(!(PMC->PMC_SR & PMC_SR_MCKRDY));

	// Initialize PLLA
	PMC->CKGR_PLLAR = BOARD_PLLAR;
	while(!(PMC->PMC_SR & PMC_SR_LOCKA));

	// Switch from internal slow clock (32khz) to main clock
	PMC->PMC_MCKR = (BOARD_MCKR & ~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	while(!(PMC->PMC_SR & PMC_SR_MCKRDY));

	PMC->PMC_MCKR = BOARD_MCKR ;
	while(!(PMC->PMC_SR & PMC_SR_MCKRDY));

}
