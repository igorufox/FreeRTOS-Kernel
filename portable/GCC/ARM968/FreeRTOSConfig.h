/* ESPurna: FreeRTOS-Kernel V11.1.0 on the BK7231N (ARM968E-S, ARMv5TE), ESPurna regress-todo.md item 36 - the config of
 * the port portable/GCC/ARM968 of the kernel package: ESPurna and the Matter library build with this one file (the sizes
 * of the kernel's structures, the tick, the priorities have to match). The one kernel source
 * of ESPurna's FreeRTOS chips (package framework-freertos-kernel, the same files as the ESP8266's). The values are the
 * ones the boards ran with on the BDK's FreeRTOS 9.0.0 (LibreTiny library-freertos-port/beken-bdk/FreeRTOSConfig.h);
 * new are only the names V11 asks for, configASSERT and the trace of blocking calls (as the ESP8266 port).
 *
 * The BDK's sources include this file directly (sys_rtos.h), before FreeRTOS.h: macros only, no kernel types outside
 * of them. */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* rtos_pub.h of the BDK picks its _xTaskCreate() form by it (7: signed char names) */
#define FreeRTOS_VERSION_MAJOR                      11

/* Tick: 2 ms (FCLK_DURATION_MS of the BDK), the timer of fixups/clock_rtos.c */
#define configCPU_CLOCK_HZ                          ( ( unsigned long ) 120000000 )
#define configTICK_RATE_HZ                          ( ( TickType_t ) 500 )
#define configTICK_TYPE_WIDTH_IN_BITS               TICK_TYPE_WIDTH_32_BITS

#define configKERNEL_INTERRUPT_PRIORITY             255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY        191

/* Timers */
#define configUSE_TIMERS                            1
#define configTIMER_TASK_PRIORITY                   ( 2 )
#define configTIMER_QUEUE_LENGTH                    ( 16 )
#define configTIMER_TASK_STACK_DEPTH                ( 3072 / 4 )

/* Tasks; StackType_t is 32 bits, depths in words */
#define configMAX_PRIORITIES                        ( 10 )
#define configUSE_PREEMPTION                        1
#define configMINIMAL_STACK_SIZE                    ( 512 / 2 )
#define configMAX_TASK_NAME_LEN                     ( 16 )
#define configIDLE_SHOULD_YIELD                     1
#define configUSE_CO_ROUTINES                       0
#define configMAX_CO_ROUTINE_PRIORITIES             ( 2 )
/* the BDK's MCU power save sleeps in the idle task (portSUPPRESS_TICKS_AND_SLEEP) */
#define configUSE_TICKLESS_IDLE                     1

/* Hooks */
#define configUSE_IDLE_HOOK                         1
#define configUSE_TICK_HOOK                         0
#define configUSE_MALLOC_FAILED_HOOK                1
#define configCHECK_FOR_STACK_OVERFLOW              2

/* Memory: the port's heap_4.c, the RAM the linker leaves (_empty_ram .. 192 KiB) */
#define configSUPPORT_DYNAMIC_ALLOCATION            1
#define configSUPPORT_STATIC_ALLOCATION             0
#define configDYNAMIC_HEAP_SIZE                     1
#define configTOTAL_HEAP_SIZE                       ( ( size_t ) ( 80 * 1024 ) )

/* Queues, semaphores, mutexes */
#define configQUEUE_REGISTRY_SIZE                   0
#define configUSE_COUNTING_SEMAPHORES               1
#define configUSE_MUTEXES                           1

/* Utilities */
#define configUSE_TRACE_FACILITY                    1
#define configUSE_STATS_FORMATTING_FUNCTIONS        1
#define configGENERATE_RUN_TIME_STATS               0

/* The API in the image */
#define INCLUDE_vTaskPrioritySet                    1
#define INCLUDE_uxTaskPriorityGet                   1
#define INCLUDE_vTaskDelete                         1
#define INCLUDE_vTaskSuspend                        1
#define INCLUDE_xTaskDelayUntil                     1
#define INCLUDE_vTaskDelay                          1
#define INCLUDE_xTaskAbortDelay                     1
#define INCLUDE_xTaskGetCurrentTaskHandle           1
#define INCLUDE_xTaskGetSchedulerState              1
#define INCLUDE_uxTaskGetStackHighWaterMark         1
#define INCLUDE_eTaskGetState                       1
#ifndef INCLUDE_xTaskGetHandle
#define INCLUDE_xTaskGetHandle                      1
#endif

/* The port's additions to tasks.c (xTaskIsTaskFinished() of the BDK's layer, the TCB offsets the assembler of the
 * BDK and rtcmem_beken.cpp read) */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H   1

/* The kernel's list.h is read at the end of this file (below), before FreeRTOS.h gives these their defaults: here, at
 * their default values */
#define configNUMBER_OF_CORES                       1
#define configUSE_MINI_LIST_ITEM                    1
#define configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES   0
#define configENABLE_BACKWARD_COMPATIBILITY         1

/* configASSERT: a broken kernel invariant prints the address of the failed check and stops (port.c vPortAssert; the
 * line: addr2line on firmware.elf). No arguments: a check is one call, the kernel functions the BDK's linker script
 * puts in the 4.5 KiB ITCM (xTaskIncrementTick, xTaskResumeAll, vTaskSwitchContext) have to fit there (with
 * __FILE__, __LINE__ and the caller they were 144 bytes over). No list integrity bytes: they would move the TCB fields
 * boot_handlers.S and rtcmem_beken.cpp read */
#ifndef __ASSEMBLER__
void vPortAssert( void );
#endif
#define configASSERT( x )    do { if( !( x ) ) { vPortAssert(); } } while( 0 )

/* A blocking call from an interrupt or with the scheduler suspended by somebody else (port.c vPortTraceBlocking), as
 * on the ESP8266: V11 asserts these where 9.0.0 let them pass */
#ifndef __ASSEMBLER__
void vPortTraceBlocking( const char * what, void * object, void * caller );
#endif
#define traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue )    vPortTraceBlocking( "queue receive", ( pxQueue ), __builtin_return_address( 0 ) )
#define traceBLOCKING_ON_QUEUE_SEND( pxQueue )       vPortTraceBlocking( "queue send", ( pxQueue ), __builtin_return_address( 0 ) )
#define traceBLOCKING_ON_QUEUE_PEEK( pxQueue )       vPortTraceBlocking( "queue peek", ( pxQueue ), __builtin_return_address( 0 ) )
#define traceTASK_DELAY()                            vPortTraceBlocking( "task delay", NULL, __builtin_return_address( 0 ) )

#endif /* FREERTOS_CONFIG_H */

/* Two list.h with one guard: the kernel's (List_t) and the BDK supplicant's (func/wpa_supplicant_2_9/src/utils/list.h,
 * struct dl_list), both `#ifndef LIST_H`. The BDK's sources and core_beken.cpp include both, in either order, and the
 * second one came out empty (LibreTiny renamed the kernel's to freertos_list.h; the kernel files stay unchanged here).
 * Both are read here, once, when FreeRTOS.h includes this file (outside the guard above: the BDK includes this file
 * before FreeRTOS.h too, sys_rtos.h), long before task.h asks for the kernel's: the supplicant's first unless already
 * read, then the guard dropped and the kernel's read by its full path (scripts/beken_bdk.py FREERTOS_KERNEL_LIST_H,
 * BDK_DL_LIST_H). Every later list.h of either is empty, both are defined. What the kernel's list.h needs before
 * FreeRTOS.h has it: the types (portmacro.h), PRIVILEGED_FUNCTION (mpu_wrappers.h, no MPU: empty), and the rename of
 * configENABLE_BACKWARD_COMPATIBILITY that FreeRTOS.h makes at its end (pxContainer -> pvContainer) */
#if defined( INC_FREERTOS_H ) && !defined( ESPURNA_FREERTOS_LISTS ) && defined( FREERTOS_KERNEL_LIST_H ) && defined( BDK_DL_LIST_H )
    #define ESPURNA_FREERTOS_LISTS
    #include "portmacro.h"
    #ifndef DL_LIST_HEAD_INIT
        #include BDK_DL_LIST_H
    #endif
    #include "mpu_wrappers.h"
    #define pxContainer    pvContainer
    #undef LIST_H
    #include FREERTOS_KERNEL_LIST_H
#endif
