/* ESPurna: the BK7231N port's additions to tasks.c of FreeRTOS-Kernel V11.1.0 (configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H,
 * included at the end of tasks.c: the kernel's own lists are visible here, the kernel files stay unchanged). In the
 * port's folder (portable/GCC/ARM968), which is on the include path of whoever builds the kernel with it. */

#include <stddef.h>

/* The BDK's layer (rtos_delete_thread, rtos_thread_join): the task is in none of the kernel's lists any more - deleted
 * (waiting for the idle task to free it, or freed). The BDK's patched 9.0.0 tasks.c had it, the same test */
BaseType_t xTaskIsTaskFinished( void * xTask )
{
    const TCB_t * const pxTCB = ( const TCB_t * ) xTask;
    BaseType_t xFinished = pdTRUE;
    UBaseType_t uxPriority;

    configASSERT( xTask );

    if( pxCurrentTCB == pxTCB )
    {
        return pdFALSE;
    }

    taskENTER_CRITICAL();
    {
        if( ( listIS_CONTAINED_WITHIN( &xSuspendedTaskList, &( pxTCB->xStateListItem ) ) != pdFALSE ) ||
            ( listIS_CONTAINED_WITHIN( &xDelayedTaskList1, &( pxTCB->xStateListItem ) ) != pdFALSE ) ||
            ( listIS_CONTAINED_WITHIN( &xDelayedTaskList2, &( pxTCB->xStateListItem ) ) != pdFALSE ) ||
            ( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) != pdFALSE ) )
        {
            xFinished = pdFALSE;
        }

        for( uxPriority = 0; ( xFinished != pdFALSE ) && ( uxPriority < ( UBaseType_t ) configMAX_PRIORITIES ); uxPriority++ )
        {
            if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxPriority ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
            {
                xFinished = pdFALSE;
            }
        }
    }
    taskEXIT_CRITICAL();

    return xFinished;
}

/* port.c vPortTraceBlocking(): the nesting of vTaskSuspendAll() */
UBaseType_t uxPortSchedulerSuspended( void )
{
    return uxSchedulerSuspended;
}

/* What reads the TCB by offset:
 * - driver/entry/boot_handlers.S (portSAVE_CONTEXT / portRESTORE_CONTEXT, vPortStartFirstTask): pxTopOfStack, the first
 *   word;
 * - espurna/arch/beken/rtcmem_beken.cpp (the stall report): pxTopOfStack, and the list an event item waits in
 *   (xEventListItem.pxContainer, offset 40: a ListItem_t of 5 words, no integrity bytes) */
_Static_assert( offsetof( TCB_t, pxTopOfStack ) == 0, "boot_handlers.S: pxTopOfStack is not the first word of the TCB" );
_Static_assert( offsetof( TCB_t, xEventListItem ) + offsetof( ListItem_t, pxContainer ) == 40,
                "rtcmem_beken.cpp TcbEventContainer: the event list of a task is not at offset 40 of the TCB" );
_Static_assert( sizeof( List_t ) == 28, "rtcmem_beken.cpp QueueReceiveList: a List_t is not 7 words (configUSE_MINI_LIST_ITEM 0)" );
