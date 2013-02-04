/* bricklib
 * Copyright (C) 2010-2013 Olaf Lüke <olaf@tinkerforge.com>
 *
 * logging.h: Logging functionality for lcd and serial console
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

#ifndef LOGGING_H
#define LOGGING_H

#include "config.h"
#include <stdio.h>

#define LOGGING_DEBUG    0
#define LOGGING_INFO     1
#define LOGGING_WARNING  2
#define LOGGING_ERROR    3
#define LOGGING_FATAL    4
#define LOGGING_NONE     5

#ifndef LOGGING_LEVEL
#define LOGGING_LEVEL LOGGING_NONE
#endif

#define LOG_NUM_TYPES 4

#define LCD_BORDER_X 10
#define LCD_BORDER_Y 10

#define LCD_NUM_CHARS 18
#define LCD_NUM_LINES 15
#define LCD_LINE_HEIGHT 20

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#if defined(LOGGING_LCD) || defined(LOGGING_SERIAL)

#ifdef LOGGING_LCD

typedef struct LCDBuffer {
	char msg[LCD_NUM_CHARS+1];
	int color;
} LCDBuffer;

int incr_index(const int index);
void logging_draw_line(const char *msg, const int color, const int line);
void logging_redraw(void);
void logging_lcd(const int type, const char *msg);
#endif

#ifdef LOGGING_SERIAL
//void logging_serial(const int type, const char *msg);
void logging_serial(const char *msg, const uint8_t length);
#endif

void logging(const int type, const char *msg);
void logging_impl(const char *msg, const uint8_t length);
#else

#define logging(a, b) {};

#endif

void logging_init(void);

#if LOGGING_LEVEL <= LOGGING_DEBUG
#define logd(str,  ...) do{ printf("<D> " str, ##__VA_ARGS__); }while(0)
#define logwohd(str,  ...) do{ printf(str, ##__VA_ARGS__); }while(0)
#else
#define logd(str,  ...) {}
#define logwohd(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_INFO
#define logi(str,  ...) do{ printf("<I> " str, ##__VA_ARGS__); }while(0)
#define logwohi(str,  ...) do{ printf(str, ##__VA_ARGS__); }while(0)
#else
#define logi(str,  ...) {}
#define logwohi(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_WARNING
#define logw(str,  ...) do{ printf("<W> " str, ##__VA_ARGS__); }while(0)
#define logwohw(str,  ...) do{ printf(str, ##__VA_ARGS__); }while(0)
#else
#define logw(str,  ...) {}
#define logwohw(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_ERROR
#define loge(str,  ...) do{ printf("<E> " str, ##__VA_ARGS__); }while(0)
#define logwohe(str,  ...) do{ printf(str, ##__VA_ARGS__); }while(0)
#else
#define loge(str,  ...) {}
#define logwohe(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_FATAL
#define logf(str,  ...) do{ printf("<F> " str, ##__VA_ARGS__); }while(0)
#define logwohf(str,  ...) do{ printf(str, ##__VA_ARGS__); }while(0)
#else
#define logf(str,  ...) {}
#define logwohf(str,  ...) {}
#endif


#if(DEBUG_STARTUP)
#define logsd(str, ...) do{logd("s: " str, ##__VA_ARGS__);}while(0)
#define logsi(str, ...) do{logi("s: " str, ##__VA_ARGS__);}while(0)
#define logsw(str, ...) do{logw("s: " str, ##__VA_ARGS__);}while(0)
#define logse(str, ...) do{loge("s: " str, ##__VA_ARGS__);}while(0)
#define logsf(str, ...) do{logf("s: " str, ##__VA_ARGS__);}while(0)
#else
#define logsd(str, ...) {}
#define logsi(str, ...) {}
#define logsw(str, ...) {}
#define logse(str, ...) {}
#define logsf(str, ...) {}
#endif

#endif
