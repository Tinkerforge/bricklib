/* bricklib
 * Copyright (C) 2010 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_common.c: Functions common to all SPI communication
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

#include "spi_common.h"

#include <stdint.h>
#include <spi/spi.h>
#include <pio/pio.h>

//static const Pin spi_pins[] = {PINS_SPI, PIN_SPI_NPCS0_PA11};

void spi_configure(void) {
	//PIO_Configure(spi_pins, PIO_LISTSIZE(spi_pins));
}

void spi_transceive_dma(void *data_send1, void *data_recv1, uint16_t len1,
                        void *data_send2, void *data_recv2, uint16_t len2) {
    SPI_PdcSetTx(SPI, data_send1, len1, data_send2, len2);
    SPI_PdcSetRx(SPI, data_recv1, len1, data_recv2, len2);

    // Enable RX and TX DMA transfer requests
    SPI_PdcEnableRx(SPI);
    SPI_PdcEnableTx(SPI);
}
