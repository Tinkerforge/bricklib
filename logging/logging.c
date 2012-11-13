/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * logging.c: Logging functionality for lcd and serial console
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

#include "logging.h"
#include "config.h"

#include "bricklib/drivers/pio/pio.h"

#if defined(LOGGING_LCD) || defined(LOGGING_SERIAL)
#include <string.h>
#include <stdio.h>

#include "bricklib/drivers/usart/uart_console.h"


#ifdef LOGGING_LCD
#include <lcd/color.h>
#include <lcd/draw.h>
#include <lcd/lcdd.h>
static unsigned int lcd_index = 8;

static LCDBuffer lcd_buffer[LCD_NUM_LINES] = {
	{"Welcome", COLOR_SKYBLUE},
	{" ", COLOR_WHITE},
	{"Board:", COLOR_LIGHTGREEN},
	{BOARD_NAME, COLOR_LIGHTGREEN},
	{" ", COLOR_WHITE},
	{"Compiled:", COLOR_MAGENTA},
	{__DATE__, COLOR_MAGENTA},
	{__TIME__, COLOR_MAGENTA},
	{" ", COLOR_WHITE},
	{" ", COLOR_WHITE},
	{" ", COLOR_WHITE},
	{" ", COLOR_WHITE},
	{" ", COLOR_WHITE},
	{" ", COLOR_WHITE},
	{" ", COLOR_WHITE}
};

static const int log_type_to_color[LOG_NUM_TYPES] = {
	COLOR_SKYBLUE,
	COLOR_WHITE,
	COLOR_YELLOW,
	COLOR_RED
};

int incr_index(const int index) {
	return (index+1) % LCD_NUM_LINES;
}

void logging_draw_line(const char *msg, const int color, const int line) {
	LCDD_DrawString(LCD_BORDER_X,
	                LCD_BORDER_Y+line*LCD_LINE_HEIGHT,
	                (unsigned char*)msg,
	                color);
}

void logging_redraw(void) {
    LCDD_Fill(COLOR_BLACK);

    int i = lcd_index;
	for(int line = 0; line < LCD_NUM_LINES; line++) {
		i = incr_index(i);
		logging_draw_line(lcd_buffer[i].msg, lcd_buffer[i].color, line);
	}
}


void logging_lcd(const int type, const char *msg) {
	int i = incr_index(lcd_index);

	strncpy(lcd_buffer[i].msg, msg, MIN(LCD_NUM_CHARS, strlen(msg)));

	lcd_buffer[i].msg[MIN(LCD_NUM_CHARS, strlen(msg))] ='\0';
	lcd_buffer[i].color = log_type_to_color[type];

	lcd_index = i;

	logging_redraw();
}
#endif


#ifdef LOGGING_LCD
static const char * const log_type_to_string[LOG_NUM_TYPES] = {
	"D: ",
	"I: ",
	"W: ",
	"E: "
};
#endif


void logging_serial(const char *msg, const uint8_t length) {
	for(uint8_t i = 0; i < length; i++) {
		UART_PutChar(msg[i]);
	}
}

#endif

void logging_impl(const char *msg, const uint8_t length) {
#ifdef LOGGING_LCD
	logging_lcd(type, msg);
#endif
#ifdef LOGGING_SERIAL
	logging_serial(msg, length);
#endif

}

void logging_init(void) {
#ifdef LOGGING_LCD
	LCDD_Initialize();
	LCDD_On();
	logging_redraw();
#endif
#ifdef LOGGING_SERIAL
    UART_Configure(CONSOLE_BAUDRATE, BOARD_MCK);
#else
    Pin console_pins[] = CONSOLE_PINS;
    console_pins[0].type = PIO_INPUT;
    console_pins[1].type = PIO_INPUT;
    PIO_Configure(console_pins, PIO_LISTSIZE(console_pins));
#endif
}

