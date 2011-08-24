/* bricklib
 * Copyright (C) 2011 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sqrt.c: Different sqrt implementations
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

#include "sqrt.h"

uint32_t sqrt_integer_precise(uint32_t x) {
	register uint32_t xr;  // result register
	register uint32_t q2;  // scan-bit register
	register uint32_t f;   // flag (one bit)

	xr = 0;                // clear result
	q2 = 0x40000000L;      // higest possible result bit
	do {
		if((xr + q2) <= x) {
			x -= xr + q2;
			f = 1;         // set flag
		} else {
			f = 0;         // clear flag
		}
		xr >>= 1;
		if(f) {
			xr += q2;      // test flag
		}
	} while(q2 >>= 2);     // shift twice
	if(xr < x) {
		return xr +1;      // add for rounding
	} else{
		return xr;
	}
}



uint32_t sqrt_integer_fast(uint32_t x) {
	// TODO:
	return 0;
}

/*

#define iter1(N) \
    try = root + (1 << (N)); \
    if (n >= try << (N))   \
    {   n -= try << (N);   \
        root |= 2 << (N); \
    }

uint32_t sqrt (uint32_t n)
{
    uint32_t root = 0, try;
    iter1 (15);    iter1 (14);    iter1 (13);    iter1 (12);
    iter1 (11);    iter1 (10);    iter1 ( 9);    iter1 ( 8);
    iter1 ( 7);    iter1 ( 6);    iter1 ( 5);    iter1 ( 4);
    iter1 ( 3);    iter1 ( 2);    iter1 ( 1);    iter1 ( 0);
    return root >> 1;
}
 */
