#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

#include "config.h"

// FreeRTOS configurations
#define configUSE_PREEMPTION			0
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
//#define configTOTAL_HEAP_SIZE           4000
#define configCPU_CLOCK_HZ				( ( unsigned long ) BOARD_MCK )
#define configTICK_RATE_HZ				( ( portTickType ) 1000 )
#define configMINIMAL_STACK_SIZE		( ( unsigned short ) 70 ) // available to the idle task
#define configMAX_TASK_NAME_LEN			( 7 )
#define configUSE_TRACE_FACILITY		0
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			0
#define configUSE_CO_ROUTINES 			0
#define configUSE_MUTEXES				1
#define configUSE_RECURSIVE_MUTEXES		0
#define configUSE_COUNTING_SEMAPHORES   0
#define configCHECK_FOR_STACK_OVERFLOW	2

#ifdef PROFILING
#include "bricklib/utility/profiling.h"
#define configGENERATE_RUN_TIME_STATS   1
#endif

#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 2 )
#define configMAX_CO_ROUTINE_PRIORITIES     0
#define configQUEUE_REGISTRY_SIZE			0

// API function includes: 1 means API function is included
#define INCLUDE_vTaskPrioritySet			0
#define INCLUDE_uxTaskPriorityGet			0
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources		0
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_uxTaskGetStackHighWaterMark	1


// Priority 15, or 255 (only the top four bits are implemented).
#define configKERNEL_INTERRUPT_PRIORITY 		( 0x0f << 4 )
// Priority 5, or 80  (only the top four bits are implemented).
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( 5 << 4 )


#endif
