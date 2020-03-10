/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * syscalls.c: Implementation of syscalls (_sbrk in particular)
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

#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bricklib/drivers/usart/uart_console.h"

#include "bricklib/logging/logging.h"

// Adresses provided by Linker Script
//extern int32_t  _end ;
extern int32_t  _start_heap;
extern int32_t  _end_heap;
	
caddr_t syscalls_heap = NULL;

// sbrk based on http://e2e.ti.com/support/microcontrollers/stellaris_arm_cortex-m3_microcontroller/f/473/t/44452.aspx
// Uses 4 byte alignment
caddr_t _sbrk (int incr) {
	caddr_t prev_heap;
	caddr_t next_heap;

	if (syscalls_heap == NULL) {
		syscalls_heap = (caddr_t)&_start_heap;
	}

	prev_heap = syscalls_heap;

	// Return data aligned to 4 bytes
	next_heap = (caddr_t)(((unsigned int)(syscalls_heap + incr) + 3) & ~3);

	// current stack pointer
	register caddr_t stack_pointer asm ("sp");

	// Collision with stack pointer? -> no more memory -> return NULL
	if((next_heap > stack_pointer) || (next_heap > (caddr_t)&_end_heap)) {
		return NULL;
	} else {
		syscalls_heap = next_heap;
		return (caddr_t)prev_heap;
	}
}

int link(char *old, char *new) {
	return -1;
}

int _close(int file) {
	return -1;
}

int _fstat(int file, struct stat *st) {
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file) {
	return 1;
}

int _lseek(int file, int ptr, int dir) {
	return 0;
}

int _read(int file, char *ptr, int len) {
    return 0;
}

int _write(int file, char *ptr, int len) {
	// Use logging feature of Bricks for write syscall
#ifdef LOGGING_SERIAL
	logging_impl(ptr, (uint8_t)len);
#endif
	return len;
}

void _exit( int status ) {
	logi("Exiting with status %d.\n", status);
	while(true);
}

void _kill(int pid, int sig) {
	return;
}

int _getpid (void) {
	return -1;
}
