/* ESPurna: the ARM968E-S (ARMv5TE) port of FreeRTOS-Kernel V11.1.0 for the BK7231N (portable/GCC/ARM968 of the kernel
 * package, ESPurna regress-todo.md item 36). Built against the BDK's headers (include.h, uart_pub.h ...). From the
 * BDK's port of FreeRTOS 9.0.0 (LibreTiny library-freertos-port/beken-bdk, MIT): the same types, interrupt calls and
 * context layout (the context switch is the BDK's own assembler, driver/entry/boot_handlers.S: portSAVE_CONTEXT /
 * portRESTORE_CONTEXT keep pxCurrentTCB->pxTopOfStack and ulCriticalNesting).
 *
 * What differs from 9.0.0:
 * - critical sections: the BDK patched its 9.0.0 queue.c / tasks.c so that taskENTER_CRITICAL() opened a block and
 *   taskEXIT_CRITICAL() closed it (the saved CPSR in a local). V11 is unpatched: vPortEnterCritical() /
 *   vPortExitCritical() count the nesting in ulCriticalNesting, which the context switch saves per task, and restore
 *   the I/F bits the outermost enter found - a kernel call inside the BDK's GLOBAL_INT_DISABLE() or an interrupt
 *   handler leaves the interrupts as it found them, as before;
 * - a switch requested from an interrupt that interrupted another interrupt (FIQ over IRQ) is postponed to the next
 *   tick (port.c vPortYieldFromIsr): the assembler restores the interrupted pxCurrentTCB on that path. The BDK did this
 *   inside its patched vTaskSwitchContext() (preempt_delayed_schedule_handler); V11's is unpatched. */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* Type definitions. */
#define portCHAR                char
#define portFLOAT               float
#define portDOUBLE              double
#define portLONG                long
#define portSHORT               short
#define portSTACK_TYPE          uint32_t
#define portBASE_TYPE           long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
#define portMAX_DELAY           ( TickType_t ) 0xffffffffUL
/* a 32-bit load on a 32-bit CPU */
#define portTICK_TYPE_IS_ATOMIC 1

/* the CPSR bits the BDK and the port use */
#define portINITIAL_SPSR        ( ( StackType_t ) 0x1f ) /* System mode, ARM mode, interrupts enabled. */
#define portINSTRUCTION_SIZE    ( ( StackType_t ) 4 )

#define ARM968_SYS_MODE         0x1f
#define ARM968_MODE_MASK        0x1f
#define ARM968_IF_MASK          0xC0
#define ARM968_IRQ_ENABLE       0x80
#define ARM968_FIQ_ENABLE       0x40
#define ARM968_IRQ_MODE         0x12
#define ARM968_FIQ_MODE         0x11

#define portNO_CRITICAL_NESTING ( ( uint32_t ) 0 )

enum arm_mode {
    ARM_MODE_USR = 16,
    ARM_MODE_FIQ = 17,
    ARM_MODE_IRQ = 18,
    ARM_MODE_SVC = 19,
    ARM_MODE_MON = 22,
    ARM_MODE_ABT = 23,
    ARM_MODE_HYP = 26,
    ARM_MODE_UND = 27,
    ARM_MODE_1176_MON = 28,
    ARM_MODE_SYS = 31,

    ARM_MODE_THREAD = 0,
    ARM_MODE_USER_THREAD = 1,
    ARM_MODE_HANDLER = 2,

    ARMV8_64_EL0T = 0x0,
    ARMV8_64_EL1T = 0x4,
    ARMV8_64_EL1H = 0x5,
    ARMV8_64_EL2T = 0x8,
    ARMV8_64_EL2H = 0x9,
    ARMV8_64_EL3T = 0xC,
    ARMV8_64_EL3H = 0xD,

    ARM_MODE_ANY = -1
};

/* Hardware specifics. */
#define portSTACK_GROWTH        ( -1 )
#define portTICK_PERIOD_MS      ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT      8
#define portYIELD()             __asm volatile ( "SWI 0" ::: "memory" )
#define portNOP()               __asm volatile ( "NOP" )

/* The interrupts of the CPU: functions, the closed libraries of the BDK link against them (Thumb code calling ARM) */
#define CONTROL_IRQ_WITH_NORMAL_FUNCTION    1
extern void portENABLE_IRQ( void );
extern void portENABLE_FIQ( void );
extern int portDISABLE_FIQ( void );
extern int portDISABLE_IRQ( void );
extern int port_disable_interrupts_flag( void );
extern void port_enable_interrupts_flag( int val );

/* the BDK's own (driver/entry/arch.h has the same) */
#define GLOBAL_INT_DECLARATION()   uint32_t fiq_tmp, irq_tmp
#define GLOBAL_INT_DISABLE()       do{\
										fiq_tmp = portDISABLE_FIQ();\
										irq_tmp = portDISABLE_IRQ();\
									}while(0)


#define GLOBAL_INT_RESTORE()       do{                         \
                                        if(!fiq_tmp)           \
                                        {                      \
                                            portENABLE_FIQ();  \
                                        }                      \
                                        if(!irq_tmp)           \
                                        {                      \
                                            portENABLE_IRQ();  \
                                        }                      \
                                   }while(0)

extern uint32_t platform_is_in_interrupt_context( void );
extern uint32_t platform_is_in_fiq_context( void );
extern uint32_t platform_is_in_irq_context( void );
extern uint32_t platform_interrupt_compare( void );

#define portENABLE_INTERRUPTS()  do {                               \
                                     if( platform_is_in_fiq_context() ) \
                                         break;                     \
                                     if( platform_is_in_irq_context() ) { \
                                         portENABLE_FIQ();          \
                                         break;                     \
                                     }                              \
                                     portENABLE_IRQ();              \
                                     portENABLE_FIQ();              \
                                 } while( 0 )

#define portDISABLE_INTERRUPTS() do {                               \
                                     if( platform_is_in_fiq_context() ) \
                                         break;                     \
                                     if( platform_is_in_irq_context() ) { \
                                         portDISABLE_FIQ();         \
                                         break;                     \
                                     }                              \
                                     portDISABLE_FIQ();             \
                                     portDISABLE_IRQ();             \
                                 } while( 0 )

/* Critical sections: counted, the outermost one restores the I/F bits it found (port.c) */
void vPortEnterCritical( void );
void vPortExitCritical( void );
#define portENTER_CRITICAL()    vPortEnterCritical()
#define portEXIT_CRITICAL()     vPortExitCritical()

/* From an interrupt: the interrupt controller's enable register cleared and put back (the BDK's 9.0.0 port) */
static inline uint32_t port_interrupt_mask_from_isr( void )
{
    uint32_t val;

    val = *( ( volatile uint32_t * ) ( ( 0x00802000 + 17 * 4 ) ) );
    *( ( volatile uint32_t * ) ( ( 0x00802000 + 17 * 4 ) ) ) = 0x0;
    return val;
}

#define portSET_INTERRUPT_MASK_FROM_ISR()    port_interrupt_mask_from_isr()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedStatusValue ) \
    ( *( ( volatile uint32_t * ) ( ( 0x00802000 + 17 * 4 ) ) ) = ( uxSavedStatusValue ) )

/* A context switch requested by an interrupt handler: now, or at the next tick when this interrupt interrupted another
 * one (port.c) */
void vPortYieldFromIsr( void );
#define portYIELD_FROM_ISR( x )        do { if( ( x ) != pdFALSE ) { vPortYieldFromIsr(); } } while( 0 )
#define portEND_SWITCHING_ISR( x )     portYIELD_FROM_ISR( x )

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/* The tickless idle: the BDK's MCU power save */
extern int bk_wlan_mcu_suppress_and_sleep( unsigned int );
#define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime )    bk_wlan_mcu_suppress_and_sleep( ( xExpectedIdleTime ) )

/* The BDK's additions to the kernel API: heap_4.c, freertos_tasks_c_additions.h */
void * pvPortRealloc( void * pv, size_t size );
BaseType_t xTaskIsTaskFinished( void * xTask );

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
