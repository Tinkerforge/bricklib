/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/**
 * \file
 * Interface for default exception handlers.
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "config.h"

/*----------------------------------------------------------------------------
 *        Types
 *----------------------------------------------------------------------------*/

/* Function prototype for exception table items (interrupt handler). */
typedef void( *IntFunc )( void );

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

extern void IrqHandlerNotUsed(void);

extern __attribute__ ((weak)) void NMI_Handler(void);
extern __attribute__ ((weak)) void HardFault_Handler(void);
extern __attribute__ ((weak)) void MemManage_Handler(void);
extern __attribute__ ((weak)) void BusFault_Handler(void);
extern __attribute__ ((weak)) void UsageFault_Handler(void);
extern __attribute__ ((weak)) void SVC_Handler(void);
extern __attribute__ ((weak)) void DebugMon_Handler(void);
extern __attribute__ ((weak)) void PendSV_Handler(void);
extern __attribute__ ((weak)) void SysTick_Handler(void);

extern __attribute__ ((weak)) void SUPC_IrqHandler(void);
extern __attribute__ ((weak)) void RSTC_IrqHandler(void);
extern __attribute__ ((weak)) void RTC_IrqHandler(void);
extern __attribute__ ((weak)) void RTT_IrqHandler(void);
extern __attribute__ ((weak)) void WDT_IrqHandler(void);
extern __attribute__ ((weak)) void PMC_IrqHandler(void);
extern __attribute__ ((weak)) void EEFC_IrqHandler(void);
extern __attribute__ ((weak)) void UART0_IrqHandler(void);
extern __attribute__ ((weak)) void UART1_IrqHandler(void);
extern __attribute__ ((weak)) void SMC_IrqHandler(void);
extern __attribute__ ((weak)) void PIOA_IrqHandler(void);
extern __attribute__ ((weak)) void PIOB_IrqHandler(void);
extern __attribute__ ((weak)) void PIOC_IrqHandler(void);
extern __attribute__ ((weak)) void USART0_IrqHandler(void);
extern __attribute__ ((weak)) void USART1_IrqHandler(void);
extern __attribute__ ((weak)) void MCI_IrqHandler(void);
extern __attribute__ ((weak)) void TWI0_IrqHandler(void);
extern __attribute__ ((weak)) void TWI1_IrqHandler(void);
extern __attribute__ ((weak)) void SPI_IrqHandler(void);
extern __attribute__ ((weak)) void SSC_IrqHandler(void);
extern __attribute__ ((weak)) void TC0_IrqHandler(void);
extern __attribute__ ((weak)) void TC1_IrqHandler(void);
extern __attribute__ ((weak)) void TC2_IrqHandler(void);
extern __attribute__ ((weak)) void TC3_IrqHandler(void);
extern __attribute__ ((weak)) void TC4_IrqHandler(void);
extern __attribute__ ((weak)) void TC5_IrqHandler(void);
extern __attribute__ ((weak)) void ADC_IrqHandler(void);
extern __attribute__ ((weak)) void DAC_IrqHandler(void);
extern __attribute__ ((weak)) void PWM_IrqHandler(void);
extern __attribute__ ((weak)) void CRCCU_IrqHandler(void);
extern __attribute__ ((weak)) void ACC_IrqHandler(void);
extern __attribute__ ((weak)) void USBD_IrqHandler(void);

#endif /* #ifndef EXCEPTIONS_H */
