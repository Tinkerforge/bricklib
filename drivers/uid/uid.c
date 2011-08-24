#include "uid.h"

#include <stddef.h>
#include <stdio.h>
#include <pio/pio.h>
#include <efc/efc.h>

#define MAX_BASE58_STR_SIZE 13
const char BASE58_STR[] = \
	"123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

__attribute__ ((section (".ramfunc")))
uint64_t uid_get_uid64(void) {
    // Write "Start Read" unique identifier command (STUI)
	EFC->EEFC_FCR = (0x5A << 24) | EFC_FCMD_STUI;
	 // If STUI is performed, FRDY is cleared
	while ((EFC->EEFC_FSR & EEFC_FSR_FRDY) == EEFC_FSR_FRDY);

    // The Unique Identifier has 128 bits.
	// We only use the last 64 bits.
    uint64_t uid = *(((uint64_t *)IFLASH_ADDR) + 1);

    // Write "Stop Read" unique identifier command (SPUI)
    EFC->EEFC_FCR = (0x5A << 24) | EFC_FCMD_SPUI;
    // If SPUI is performed, FRDY is set
    while ((EFC->EEFC_FSR & EEFC_FSR_FRDY) != EEFC_FSR_FRDY);

    return uid;
}

char uid_get_serial_char_from_num(uint8_t num) {
	if(num < 25) {
		return 'a' + num;
	} else if(num < 50) {
		return 'A' + (num-25);
	} else if(num < 60) {
		return '0' + (num-50);
	}

	return '?';
}

void uid_to_serial_number(uint64_t value, char *str) {
	char reverse_str[MAX_BASE58_STR_SIZE] = {0};
	int i = 0;
	while(value >= 58) {
		uint64_t mod = value % 58;
		reverse_str[i] = BASE58_STR[mod];
		value = value/58;
		i++;
	}

	reverse_str[i] = BASE58_STR[value];
	int j = 0;
	i = 0;
	while(reverse_str[MAX_BASE58_STR_SIZE-1 - i] == '\0') {
		i++;
	}
	for(j = 0; j < MAX_BASE58_STR_SIZE; j++) {
		if(MAX_BASE58_STR_SIZE - i >= 0) {
			str[j] = reverse_str[MAX_BASE58_STR_SIZE-1 - i];
		} else {
			str[j] = '\0';
		}
		i++;
	}
}
