/* ESPurna: the ARM968E-S port of FreeRTOS-Kernel V11.1.0 for the BK7231N (ESPurna regress-todo.md item 36), in the kernel
 * package next to the kernel (portable/GCC/ARM968, with the chip's FreeRTOSConfig.h): ESPurna and the Matter library
 * build against this one port and config. From the BDK's port
 * of FreeRTOS 9.0.0 (LibreTiny library-freertos-port/beken-bdk/port.c, MIT; the FreeRTOS ARM9 STR91x port). The
 * context switch, the vectors and the first task start are the BDK's assembler (driver/entry/boot_handlers.S), the
 * tick is fixups/clock_rtos.c. See portmacro.h for what changed against 9.0.0. */

#include <stdlib.h>

#include "include.h"
#include "uart_pub.h"
#include "fake_clock_pub.h"
#include "osk_revision.h"

#include <FreeRTOS.h>
#include <task.h>

/* ulCriticalNesting: the I/F bits of the CPSR the outermost vPortEnterCritical() found in the low byte (at their CPSR
 * positions, 0xC0), the nesting count from bit 8 up. The context switch saves and restores the whole word per task
 * (boot_handlers.S); a new task starts with 0 (pxPortInitialiseStack). Not 0 until the first task starts: the kernel's
 * critical sections before the scheduler must not enable the interrupts */
uint32_t ulCriticalNesting = ( uint32_t ) 9999 << 8;

/* A context switch an interrupt asked for while it interrupted another interrupt: done by the next tick
 * (fixups/clock_rtos.c, preempt_delayed_schedule_get_flag) */
uint32_t volatile g_preempt_delayed_schedule_flag = 0;

/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
#if ( CFG_TASK_WDG_ENABLED )
    extern void task_watchdog_feed( void );
    task_watchdog_feed();
#endif
}

/*-----------------------------------------------------------*/

/* The stack of a new task as portSAVE_CONTEXT of boot_handlers.S leaves it: the nesting, the SPSR, R0-R12, SP, LR,
 * the return address */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack, TaskFunction_t pxCode, void * pvParameters )
{
    StackType_t * pxOriginalTOS;

    pxOriginalTOS = pxTopOfStack;

    pxTopOfStack--;

    /* the return address, offset as within an IRQ */
    *pxTopOfStack = ( StackType_t ) pxCode + portINSTRUCTION_SIZE;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0xaaaaaaaa; /* R14 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxOriginalTOS; /* R13 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x12121212; /* R12 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x11111111; /* R11 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x10101010; /* R10 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x09090909; /* R9 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x08080808; /* R8 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x07070707; /* R7 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x06060606; /* R6 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x05050505; /* R5 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x04040404; /* R4 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x03030303; /* R3 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x02020202; /* R2 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x01010101; /* R1 */
    pxTopOfStack--;

    /* the parameter in R0 */
    *pxTopOfStack = ( StackType_t ) pvParameters;
    pxTopOfStack--;

    /* system mode, interrupts enabled, ARM or Thumb by the entry address */
    if( ( ( StackType_t ) pxCode & 0x01 ) != 0 )
    {
        *pxTopOfStack = ( StackType_t ) portINITIAL_SPSR | 0x20;
    }
    else
    {
        *pxTopOfStack = ( StackType_t ) portINITIAL_SPSR;
    }

    pxTopOfStack--;

    /* the critical nesting of the task: none */
    *pxTopOfStack = portNO_CRITICAL_NESTING;

    return pxTopOfStack;
}

/*-----------------------------------------------------------*/

extern void vPortStartFirstTask( void );

BaseType_t xPortStartScheduler( void )
{
    os_printf( "OSK Rev: %s\r\n", BEKEN_OSK_REV );

    /* the tick timer (fixups/clock_rtos.c); the interrupts are disabled here already */
    fclk_init();

    vPortStartFirstTask();

    /* not reached */
    return 0;
}

void vPortEndScheduler( void )
{
}

/*-----------------------------------------------------------*/

/* Both in the ITCM (the BDK's linker script, 4.5 KiB with the kernel's tick and switch): hand-written, no calls out to
 * the flash (each one a linker veneer in the ITCM) */
void __attribute__( ( naked ) ) vPortEnterCritical( void )
{
    __asm volatile ( "mrs   r1, cpsr\n"
                     "orr   r2, r1, #0xC0\n"
                     "msr   cpsr_c, r2\n"
                     "ldr   r0, =ulCriticalNesting\n"
                     "ldr   r2, [r0]\n"
                     "cmp   r2, #0x100\n"           /* outermost: keep the I/F bits it found */
                     "andlo r2, r1, #0xC0\n"
                     "add   r2, r2, #0x100\n"
                     "str   r2, [r0]\n"
                     "bx    lr\n"
                     ".ltorg\n" );
}

void __attribute__( ( naked ) ) vPortExitCritical( void )
{
    __asm volatile ( "ldr   r0, =ulCriticalNesting\n"
                     "ldr   r2, [r0]\n"
                     "cmp   r2, #0x100\n"           /* not in a critical section */
                     "bxlo  lr\n"
                     "sub   r2, r2, #0x100\n"
                     "str   r2, [r0]\n"
                     "cmp   r2, #0x100\n"           /* still nested */
                     "bxhs  lr\n"
                     "mrs   r1, cpsr\n"              /* the outermost: the I/F bits its enter found */
                     "bic   r1, r1, #0xC0\n"
                     "orr   r1, r1, r2\n"
                     "msr   cpsr_c, r1\n"
                     "bx    lr\n"
                     ".ltorg\n" );
}

/*-----------------------------------------------------------*/

uint32_t platform_is_in_irq_enable( void )
{
    uint32_t interrupt;

    __asm volatile ( "MRS %0,CPSR\n"
                     "AND %0,%0,#0xC0\n"
                     : "=r" ( interrupt ) : : "memory" );
    return( !( interrupt & ARM968_IRQ_ENABLE ) );
}

uint32_t platform_is_in_fiq_enable( void )
{
    uint32_t interrupt;

    __asm volatile ( "MRS %0,CPSR\n"
                     "AND %0,%0,#0xC0\n"
                     : "=r" ( interrupt ) : : "memory" );
    return( !( interrupt & ARM968_FIQ_ENABLE ) );
}

void printf_lr_register( void )
{
    uint32_t value;

    __asm volatile ( "MOV %0,lr\n" : "=r" ( value ) : : "memory" );
    os_printf( "lr:%x\r\n", value );
}

uint32_t platform_is_in_irq_context( void )
{
    uint32_t mode;

    __asm volatile ( "MRS %0,CPSR\n"
                     "AND %0,%0,#0x1f\n"
                     : "=r" ( mode ) : : "memory" );
    return( ARM968_IRQ_MODE == mode );
}

uint32_t platform_is_in_fiq_context( void )
{
    uint32_t mode;

    __asm volatile ( "MRS %0,CPSR\n"
                     "AND %0,%0,#0x1f\n"
                     : "=r" ( mode ) : : "memory" );
    return( ARM968_FIQ_MODE == mode );
}

uint32_t platform_spsr_content( void )
{
    uint32_t mode;

    __asm volatile ( "MRS %0,SPSR\n" : "=r" ( mode ) : : "memory" );
    return mode;
}

uint32_t platform_sp_content( void )
{
    uint32_t val;

    __asm volatile ( "mov %0,SP\n" : "=r" ( val ) : : "memory" );
    return val;
}

uint32_t platform_cpsr_content( void )
{
    uint32_t mode;

    __asm volatile ( "MRS %0,CPSR\n" : "=r" ( mode ) : : "memory" );
    return mode;
}

uint32_t platform_interrupt_compare( void )
{
    const uint32_t status = platform_cpsr_content();
    const uint32_t sta_irq = ( ( status & 0x80 ) >> 7 );
    const uint32_t sta_fiq = ( ( status & 0x40 ) >> 6 );

    return ( sta_fiq == sta_irq ) ? 1 : 0;
}

uint32_t platform_is_fiq_disable( void )
{
    return platform_cpsr_content() & 0x40;
}

uint32_t platform_is_irq_disable( void )
{
    return platform_cpsr_content() & 0x80;
}

/* in the ITCM: one read of the CPSR, no calls */
uint32_t platform_is_in_interrupt_context( void )
{
    uint32_t mode;

    __asm volatile ( "MRS %0,CPSR\n"
                     "AND %0,%0,#0x1f\n"
                     : "=r" ( mode ) : : "memory" );
    return( ( ARM968_FIQ_MODE == mode ) || ( ARM968_IRQ_MODE == mode ) );
}

void platform_enable_intc_at_restore_context( void )
{
    if( !platform_is_in_interrupt_context() )
    {
        portENABLE_IRQ();
    }

    if( !platform_is_in_fiq_context() )
    {
        portENABLE_FIQ();
    }
}

void rtos_stack_overflow( char * taskname )
{
    os_printf( "stack overflow: %s\r\n", taskname );

    while( 1 )
    {
    }
}

/*-----------------------------------------------------------*/

/* The postponed switch (the BDK's preempt_delayed_schedule_* of CFG_BDK_VERSION < 30045, its tick handler polls the
 * flag) */
void preempt_delayed_schedule_set_flag( void )
{
    g_preempt_delayed_schedule_flag = 1;
}

void preempt_delayed_schedule_clear_flag( void )
{
    g_preempt_delayed_schedule_flag = 0;
}

uint32_t preempt_delayed_schedule_get_flag( void )
{
    return g_preempt_delayed_schedule_flag;
}

/* From an interrupt handler (IRQ or FIQ mode): the switch now when the handler interrupted a task (system mode, the
 * assembler saved its context and restores the new pxCurrentTCB), at the next tick otherwise (an FIQ over an IRQ
 * handler: that path of boot_handlers.S restores the interrupted pxCurrentTCB) */
void vPortYieldFromIsr( void )
{
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    if( ( platform_spsr_content() & ARM968_MODE_MASK ) == ARM_MODE_SYS )
    {
        vTaskSwitchContext();
    }
    else
    {
        preempt_delayed_schedule_set_flag();
    }

    GLOBAL_INT_RESTORE();
}

/*-----------------------------------------------------------*/

/* configASSERT() of the kernel (FreeRTOSConfig.h): the address of the failed check (addr2line -e firmware.elf gives
 * the file and the line), the task, the critical nesting, the context; then abort() and the crash record */
void __attribute__( ( noinline, noreturn ) ) vPortAssert( void )
{
    void * const site = __builtin_return_address( 0 );

    portDISABLE_FIQ();
    portDISABLE_IRQ();
    bk_printf( "FreeRTOS assert at %p task %s crit %x isr %u\r\n", site,
               xTaskGetCurrentTaskHandle() ? pcTaskGetName( NULL ) : "-", ( unsigned ) ulCriticalNesting,
               ( unsigned ) platform_is_in_interrupt_context() );
    abort();
}

/* A blocking call from an interrupt, or with the scheduler suspended by somebody else (the kernel suspends it once
 * itself right before these trace points): printed, the first 40 (FreeRTOSConfig.h) */
extern UBaseType_t uxPortSchedulerSuspended( void );
static uint32_t s_trace_blocking;

void vPortTraceBlocking( const char * what, void * object, void * caller )
{
    const int suspended = uxPortSchedulerSuspended() >= 2;
    const int isr = platform_is_in_interrupt_context() != 0;

    if( ( suspended || isr ) && ( s_trace_blocking < 40 ) )
    {
        ++s_trace_blocking;
        bk_printf( "FreeRTOS blocking %s %p from %p task %s susp %d isr %d\r\n", what, object, caller,
                   pcTaskGetName( NULL ), suspended, isr );
    }
}

/*-----------------------------------------------------------*/

/* The interrupt calls, functions (portmacro.h CONTROL_IRQ_WITH_NORMAL_FUNCTION): the closed libraries link to them */
void port_enable_interrupts_flag( int val )
{
    unsigned long cpsr_val;
    unsigned long mask;

    mask = val & ARM968_IF_MASK;
    cpsr_val = platform_cpsr_content();
    cpsr_val &= ( ~ARM968_IF_MASK );
    cpsr_val += mask;
    __asm volatile ( "msr cpsr_c, %0" : : "r" ( cpsr_val ) : "memory" );
}

void portENABLE_IRQ( void )
{
    unsigned long temp;

    __asm volatile ( "mrs %0, cpsr @ local_irq_enable\n"
                     "bic %0, %0, #0x80\n"
                     "msr cpsr_c, %0"
                     : "=r" ( temp ) : : "memory" );
}

void portENABLE_FIQ( void )
{
    unsigned long temp;

    __asm volatile ( "mrs %0, cpsr @ local_irq_enable\n"
                     "bic %0, %0, #0x40\n"
                     "msr cpsr_c, %0"
                     : "=r" ( temp ) : : "memory" );
}

int portDISABLE_FIQ( void )
{
    unsigned long temp;
    unsigned long mask;

    __asm volatile ( "mrs %1, cpsr @ local_irq_disable\n"
                     "orr %0, %1, #0x40\n"
                     "msr cpsr_c, %0"
                     : "=r" ( temp ), "=r" ( mask ) : : "memory" );
    return( !!( mask & 0x40 ) );
}

int port_disable_interrupts_flag( void )
{
    unsigned long temp;
    unsigned long mask;

    __asm volatile ( "mrs %1, cpsr @ local_irq_disable\n"
                     "orr %0, %1, #0xC0\n"
                     "msr cpsr_c, %0"
                     : "=r" ( temp ), "=r" ( mask ) : : "memory" );
    return( mask & 0xC0 );
}

int portDISABLE_IRQ( void )
{
    unsigned long temp;
    unsigned long mask;

    __asm volatile ( "mrs %1, cpsr @ local_irq_disable\n"
                     "orr %0, %1, #0x80\n"
                     "msr cpsr_c, %0"
                     : "=r" ( temp ), "=r" ( mask ) : : "memory" );
    return( !!( mask & 0x80 ) );
}
