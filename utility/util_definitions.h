/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * util_definitions.h: General usefull macros
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

#ifndef UTIL_H
#define UTIL_H

//#include <FreeRTOS.h>

#define TASK_DELAY_MS(ms) vTaskDelay(ms/portTICK_RATE_MS)
#define TASK_DELAY_UNTIL_MS(last, ms) vTaskDelayUntil(last, ms/portTICK_RATE_MS)

#define SLEEP_NS(x) \
	do { \
		uint32_t i = (((BOARD_MCK)/1000000)*(x))/3000; \
		__ASM volatile ( \
			"PUSH {R0}\n" \
			"MOV R0, %0\n" \
			"1:\n" \
			"SUBS R0, #1\n" \
			"BNE.N 1b\n" \
			"POP {R0}\n" \
			:: "r" (i) \
		); \
	} while(0)

#define SLEEP_US(x) \
	do { \
		uint32_t i = ((BOARD_MCK)/1000000)*(x)/3; \
		__ASM volatile ( \
			"PUSH {R0}\n" \
			"MOV R0, %0\n" \
			"1:\n" \
			"SUBS R0, #1\n" \
			"BNE.N 1b\n" \
			"POP {R0}\n" \
			:: "r" (i) \
		); \
	} while(0)

#define SLEEP_MS(x) \
	do { \
		uint32_t i = ((BOARD_MCK)/1000)*(x)/3; \
		__ASM volatile ( \
			"PUSH {R0}\n" \
			"MOV R0, %0\n" \
			"1:\n" \
			"SUBS R0, #1\n" \
			"BNE.N 1b\n" \
			"POP {R0}\n" \
			:: "r" (i) \
		); \
	} while(0)

#ifndef ABS
	#define ABS(a) (((a) < 0) ? (-(a)) : (a))
#endif
#ifndef MIN
	#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
	#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef BETWEEN
	#define BETWEEN(min, value, max)  (MIN(max, MAX(value, min)))
#endif

#define SCALE(val_a, min_a, max_a, min_b, max_b) \
	(((((val_a) - (min_a))*((max_b) - (min_b)))/((max_a) - (min_a))) + (min_b))

#define DISABLE_RESET_BUTTON() \
	do { \
		REG_RSTC_MR = (REG_RSTC_MR |RSTC_MR_KEY(0xA5)) & ~RSTC_MR_URSTEN; \
	} while(0)

#define ENABLE_RESET_BUTTON() \
	do { \
		REG_RSTC_MR = (REG_RSTC_MR | RSTC_MR_KEY(0xA5)) | RSTC_MR_URSTEN; \
	} while(0)

#endif
