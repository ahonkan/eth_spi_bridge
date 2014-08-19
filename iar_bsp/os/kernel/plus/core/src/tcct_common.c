/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       tcct_common.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains core common thread control / scheduling
*       functions with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCCT_Schedule                       Schedule the next thread
*       TCCT_Control_To_System              Transfers control to system
*       TCCT_Schedule_Unlock                Unprotect critical section
*       NU_Activate_HISR                    Activate a HISR
*       TCCT_HISR_Shell                     HISR execution shell
*       TCCT_Dispatch_LISR                  Dispatches non-nested LISR
*       TCCT_Dispatch_Nested_LISR           Dispatches nested LISR
*       [NU_Check_Stack]                  Check current stack
*                                           (conditionally compiled)
*       [TCCT_Current_Thread]               Returns a pointer to current
*                                           thread
*                                           (conditionally compiled)
*       [TCCT_Set_Execute_Task]             Sets TCD_Execute_Task under
*                                           protection from interrupts
*                                           (conditionally compiled)
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       timer.h                             Timer service constants
*       power_core.h                        Power Management Services
*       idle_scheduler.h                    Power API for Tick
*                                           Suppression
*
***********************************************************************/
/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/supplement/inc/error_management.h"
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE)) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))
#include        "services/power_core.h"
#include        "os/services/power/core/inc/idle_scheduler.h"
#endif
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references */
extern TC_HCB * volatile    TCD_Execute_HISR;
extern TC_HCB               *TCD_Active_HISR_Heads[TC_HISR_PRIORITIES+1];
extern TC_HCB               *TCD_Active_HISR_Tails[TC_HISR_PRIORITIES];
extern volatile INT         TCD_Highest_Priority_HISR;

static BOOLEAN              TCD_Return_To_Scheduler;

/***********************************************************************
*
*   FUNCTION
*
*      TCCT_Schedule
*
*   DESCRIPTION
*
*      This function waits for a thread to become ready.  Once a
*      thread is ready, this function initiates a transfer of control
*      to that thread.
*
*   CALLED BY
*
*       ESAL_GE_STK_Solicited_Switch        Solicited context switch
*       INC_Initialize                      Main initialization routine
*       TCCT_HISR_Shell                     Shell function for HISR
*                                           execution
*       TCCT_Interrupt_Context_Restore      Restore context after
*                                           interrupt service routine
*       TCCT_Signal_Exit                    Exit function for signals
*
*   CALLS
*
*       ESAL_GE_STK_Unsolicited_Restore     Restores context of
*                                           an unsolicited stack frame
*       ESAL_GE_STK_Solicited_Restore       Restores context of solicited
*                                           stack frame
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TCCT_Schedule(VOID)
{
    R1 TC_TCB   *current_thread;


    /* Execute required Nucleus PLUS timing test code (if enabled) */
    NU_PLUS_TIMING_TEST2();

    TCD_Return_To_Scheduler = NU_FALSE;

    /* Clear the current thread pointer variable */
    TCD_Current_Thread = NU_NULL;

    /* Set appropriate interrupt enable / disable value */
    TCC_INTERRUPTS_GLOBAL_ENABLE();

    /* Get pointer to TCD_Execute_HISR */
    current_thread = (TC_TCB *)TCD_Execute_HISR;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))
    /* Keep track of CPU timer counters for total and
       idle time */
    PMS_Update_Cpu_Utilization_Counters(NU_FALSE);
#endif
    /* Check if a HISR is ready to execute */
    if (!current_thread)
    {
        /* Get pointer to TCD_Execute_Task */
        current_thread = (TC_TCB *)TCD_Execute_Task;

        /* Check if a task is ready to execute */
        if (!current_thread)
        {
            /* Trace log */
            T_IDLE_ENTRY();

            /* NOTE:  If execution reaches here, an idle condition exists.  Only
                      an interrupt can cause a thread to be scheduled and control
                      will return to the top of the scheduler (not here) if this
                      occurs. */

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE))
            PMS_CPU_Idle();
#endif
            ESAL_GE_INT_WAIT();
        }

        else
        {
            TCC_Current_App_Task_Set(current_thread);

            /* Trace log */
            T_TASK_RUNNING((VOID*)current_thread);
        }
    }

    /* Disable interrupts for critical section */
    ESAL_GE_INT_FAST_ALL_DISABLE();

    /* Set the current thread pointer to the ready HISR / task */
    TCD_Current_Thread = (VOID *)current_thread;

    /* Increment this thread's scheduled count */
    current_thread -> tc_scheduled++;

#if (NU_STACK_CHECKING == NU_TRUE) && (NU_STACK_FILL == NU_TRUE)

    /* Check if last byte in stack has correct pattern */
    if (ESAL_GE_MEM_READ8(current_thread -> tc_stack_start) != NU_STACK_FILL_PATTERN)
    {
        /* Call system error handler (stack overflow has occurred) */
        ERC_System_Error(NU_STACK_OVERFLOW);
    }

#endif  /* NU_STACK_CHECKING == NU_TRUE && NU_STACK_FILL == NU_TRUE */
    
    /* Sets up the process for the current task when processes are enabled */
    NU_PROCESS_SETUP();

    /* Restore context based on stack type */
    if (ESAL_GE_STK_TYPE_GET(current_thread -> tc_stack_pointer))
    {
        /* Call unsolicited stack restore function (control doesn't return
           from this call) */
        ESAL_GE_STK_Unsolicited_Restore(current_thread -> tc_stack_pointer);
    }
    else
    {
        /* Call solicited stack restore function (control doesn't return from
           this call) */
        ESAL_GE_STK_Solicited_Restore(current_thread -> tc_stack_pointer);
    }

    /* Code should never reach here.  This line ensures compiler doesn't try
       to optimize return from this function and cause a stack pointer
       problem. */
    ESAL_GE_STK_NO_RETURN();
}


/***********************************************************************
*
*   FUNCTION
*
*       TCCT_Control_To_System
*
*   DESCRIPTION
*
*       This function returns control from a thread to the system.
*       Note that this service is called in a solicited manner, i.e.
*       it is not called from an interrupt thread.  Registers required
*       by the compiler to be preserved across function boundaries are
*       saved by this routine.  Note that this is usually a sub-set of
*       the total number of available registers.
*
*   CALLED BY
*
*       Nucleus PLUS Services
*
*   CALLS
*
*       ESAL_GE_STK_Solicited_Switch        Performs solicited
*                                           context switch
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TCCT_Control_To_System(VOID)
{
    R1  TC_TCB  *current_thread;


    /* Disable interrupts for critical section */
    TCC_INTERRUPTS_DISABLE();

    /* Get pointer to current thread */
    current_thread = (TC_TCB *)TCD_Current_Thread;

    /* Clear any locks the thread may have */
    TCD_Schedule_Lock = NU_FALSE;

    /* Perform context switch and transfer control to scheduler */
    ESAL_GE_STK_Solicited_Switch(NU_NULL,
                                 TCCT_Schedule,
                                 &current_thread -> tc_stack_pointer);

    /* Obtain the schedule lock again to finish the critical section */
    TCD_Schedule_Lock = NU_TRUE;

    /* Restore interrupt lockout as required */
    TCC_INTERRUPTS_RESTORE();
}

/***********************************************************************
*
*   FUNCTION
*
*       TCCT_Schedule_Unlock
*
*   DESCRIPTION
*
*       This function clears the scheduler flag and checks to see if
*       the scheduler needs to be called.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID TCCT_Schedule_Unlock(VOID)
{
    R1  TC_TCB  *current_thread;

#if (CFG_NU_OS_KERN_PLUS_CORE_DEBUG_SCHED_LOCK == NU_TRUE)

    /* Ensure lock set (if not set, invalid usage / nesting) */
    if (TCD_Schedule_Lock != NU_TRUE)
    {
        /* System error */
        ERC_System_Error(NU_INVALID_LOCK_USAGE);
    }

#endif

    /* Clear the lock */
    TCD_Schedule_Lock = NU_FALSE;

    if (TCD_Return_To_Scheduler)
    {
        /* Disable interrupts for critical section */
        TCC_INTERRUPTS_DISABLE();

        /* Get pointer to current thread */
        current_thread = (TC_TCB *)TCD_Current_Thread;

        /* Perform context switch and transfer control to scheduler */
        ESAL_GE_STK_Solicited_Switch(NU_NULL,
                                     TCCT_Schedule,
                                     &current_thread -> tc_stack_pointer);

        /* Restore interrupt lockout as required */
        TCC_INTERRUPTS_RESTORE();
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Activate_HISR
*
*   DESCRIPTION
*
*       This function activates the specified HISR.  If the HISR is
*       already activated, the HISR's activation count is simply
*       incremented.  Otherwise, the HISR is placed on the appropriate
*       HISR priority list in preparation for execution.
*
*   CALLED BY
*
*       Application LISRs
*       TCCE_Activate_HISR                  Performs error checking on activate
*                                           HISR function.
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       hisr                                Pointer to HISR to activate
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       NU_INVALID_HISR                     Invalid HISR pointer
*
***********************************************************************/
STATUS NU_Activate_HISR(R1 NU_HISR *hisr)
{
    R2 TC_HCB       **hisr_tail;
    R3 TC_HCB       **hisr_head;
    R4 INT          hisr_priority;
    STATUS          status = NU_SUCCESS;
    ESAL_GE_INT_CONTROL_VARS
    NU_SUPERV_USER_VARIABLES

    /* Check each parameter.  */

    /* Invalid HISR control block pointer.  */
    NU_ERROR_CHECK((hisr == NU_NULL), status, NU_INVALID_HISR);

    /* Invalid HISR control block pointer.  */
    NU_ERROR_CHECK((hisr -> tc_id != TC_HISR_ID), status, NU_INVALID_HISR);

    /* HISR activation count roll-over */
    NU_ERROR_CHECK((hisr -> tc_activation_count >= TC_MAX_HISR_ACTIVATIONS), status, NU_HISR_ACTIVATION_COUNT_ERROR);

    if (status == NU_SUCCESS)
    {
        NU_SUPERVISOR_MODE_ISR();

        /* Disable interrupts for critical section */
        ESAL_GE_INT_ALL_DISABLE();

        /* Check if this is the first activation */
        if (++hisr -> tc_activation_count == 1)
        {
            /* Get hisr's priority */
            hisr_priority = (INT)hisr -> tc_priority;

            /* Get head and tail pointer for this hisr's priority */
            hisr_head = &TCD_Active_HISR_Heads[hisr_priority];
            hisr_tail = &TCD_Active_HISR_Tails[hisr_priority];

            /* Determine if there is something in the given priority list */
            if (*hisr_tail)
            {
                /* Something is already on this list - add hisr to end of list */
                (*hisr_tail) -> tc_active_next = hisr;
                *hisr_tail = hisr;
            }
            else
            {
                /* Nothing is on this list - point head and tail to hisr */
                *hisr_head = hisr;
                *hisr_tail = hisr;

                /* Check if this hisr is the new highest priority hisr */
                if (hisr_priority < TCD_Highest_Priority_HISR)
                {
                    /* Set TCD_Execute_HISR to this hisr */
                    TCD_Execute_HISR = hisr;

                    /* Set highest priority variable to this hisr's priority */
                    TCD_Highest_Priority_HISR = hisr_priority;

                    if (!TCD_Schedule_Lock)
                    {
                        /* Only inform the abstraction layer if we are in LISR context */
                        if (ESAL_GE_ISR_EXECUTING() == NU_TRUE)
                        {
                            /* Inform the abstraction layer that an unsolicited context
                               switch is required */
                            ESAL_GE_STK_UNSOL_SWITCH_ENABLE();
                        }
                    }
                    else
                    {
                        /* Signal that the scheduler should be called
                           once it is unlocked for HISR's to run */
                        TCD_Return_To_Scheduler = NU_TRUE;
                    }
                }   /* if new highest priority hisr */

            }   /* if hisr tail pointer not NULL */

        }   /* if first hisr activation */

        /* Trace log */
        T_HISR_ACTIVATED((VOID*)hisr, status);

        /* Restore interrupts to entry level */
        ESAL_GE_INT_ALL_RESTORE();

        NU_USER_MODE_ISR();
    }
    else
    {
        /* Trace log */
        T_HISR_ACTIVATED((VOID*)hisr, status);
    }

    /* Return success */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       TCCT_HISR_Shell
*
*   DESCRIPTION
*
*       This function is the execution shell of each and every HISR.
*       If the HISR has completed its processing, this shell routine
*       exits back to the system.  Otherwise, it sequentially calls the
*       HISR routine until the activation count goes to zero.
*
*   CALLED BY
*
*       HISR Scheduling
*
*   CALLS
*
*       ESAL_GE_STK_Solicited_Set           Sets the solicited
*                                           stack frame as required
*       hisr -> tc_entry                    Actual entry function of
*                                           HISR
*       TCCT_Schedule                       Schedule the next thread
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID    TCCT_HISR_Shell(VOID)
{
    R1 TC_HCB           **hisr_head;
    R2 TC_HCB           **hisr_tail;
    R3 INT              hisr_priority;
    NU_SUPERV_USER_VARIABLES

    /* Trace log */
    if(TCD_Execute_Task != NULL)
    {
        T_TASK_READY((VOID*)TCD_Execute_Task);
    }

    /* Trace log */
    T_HISR_RUNNING((VOID*)TCC_CURRENT_HISR_PTR);

    /* Loop until all hisr activations complete */
    while (TCC_CURRENT_HISR_PTR -> tc_activation_count)
    {
        /* Set appropriate interrupt enable / disable value */
        TCC_INTERRUPTS_GLOBAL_ENABLE();

        /* Switch to user mode */
        NU_USER_MODE();

        /* Call the hisr's entry function */
        (*( TCC_CURRENT_HISR_PTR -> tc_entry)) ();

        /* Switch back to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Disable interrupts for critical section */
        ESAL_GE_INT_FAST_ALL_DISABLE();

        /* Decrement activation count */
        TCC_CURRENT_HISR_PTR -> tc_activation_count--;
    }

    /* Trace log */
    T_HISR_STOPPED((VOID*)TCC_CURRENT_HISR_PTR);

    /* Get the priority of this HISR */
    hisr_priority = TCC_CURRENT_HISR_PTR -> tc_priority;

    /* Get head and tail pointer for this hisr's priority */
    hisr_head = &TCD_Active_HISR_Heads[hisr_priority];
    hisr_tail = &TCD_Active_HISR_Tails[hisr_priority];

    /* Check if this is the only hisr in it's priority list */
    if (TCC_CURRENT_HISR_PTR == *hisr_tail)
    {
        /* This is the only hisr in it's priority list -
           remove hisr from priority list */
        *hisr_head = NU_NULL;
        *hisr_tail = NU_NULL;

        /* Ensure a higher priority HISR was not activated
           during the current HISR's execution */
        if (hisr_priority == TCD_Highest_Priority_HISR)
        {
            /* Start search for highest priority ready hisr.  This search
               starts at the priority immediately after that current
               executing hisr's priority. */
            TCD_Highest_Priority_HISR++;

            /* Loop through all remaining hisr priorities to find highest
               priority ready hisr */
            while ((!TCD_Active_HISR_Heads[TCD_Highest_Priority_HISR]) &&
                   (TCD_Highest_Priority_HISR < TC_HISR_PRIORITIES))
            {
                /* Check next priority */
                TCD_Highest_Priority_HISR++;
            }

        }

        /* Set execute hisr pointer */
        TCD_Execute_HISR = TCD_Active_HISR_Heads[TCD_Highest_Priority_HISR];
    }
    else
    {
        /* Not the only hisr in it's priority list -
           remove current hisr from this priority list */
        *hisr_head = TCC_CURRENT_HISR_PTR -> tc_active_next;

        /* Set the execute hisr variable to next hisr in this
           priority list */
        TCD_Execute_HISR =  *hisr_head;
    }

    /* Switch to the system stack
       NOTE: Necessary since the current hisr's stack frame is going to be reset.
             Continuing execution using the current hisr's stack space during
             this reset operation may cause problems. */
    ESAL_GE_STK_SYSTEM_SP_SET();

    /* Re-set hisr stack frame */
    TCC_CURRENT_HISR_PTR -> tc_stack_pointer =
                    ESAL_GE_STK_SOLICITED_RESET(TCC_CURRENT_HISR_PTR -> tc_stack_start,
                                                TCC_CURRENT_HISR_PTR -> tc_stack_end,
                                                TCCT_HISR_Shell);

    /* Transfer control to the scheduler */
    TCCT_Schedule();

    /* Code should never reach here.  This line ensures compiler doesn't try
       to optimize return from this function and cause a stack pointer
       problem. */
    ESAL_GE_STK_NO_RETURN();
}


/************************************************************************
*
*   FUNCTION
*
*       TCCT_Dispatch_LISR
*
*   DESCRIPTION
*
*       This function is the non-nested interrupt service routine for
*       Nucleus PLUS.  It utilizes the Embedded Software Abstraction
*       Layer components to execute the appropriate interrupt service
*       routine and perform post-interrupt processing necessary for
*       the OS.  This handler is registered with ESAL during
*       initialization.
*
*   CALLED BY
*
*       ESAL                                Embedded Software Abstraction
*                                           Layer
*
*   CALLS
*
*       Interrupt Service Routine
*       TCCT_Schedule
*
*   INPUTS
*
*       vector                              Interrupt vector number
*       stack_pointer                       Stack pointer of executing
*                                           thread
*
*   OUTPUTS
*
*       pointer to stack pointer in thread's control block
*
*************************************************************************/
VOID    **TCCT_Dispatch_LISR(INT vector, VOID *stack_pointer)
{
    /* Indicate that ISR is started */
    ESAL_GE_ISR_START();
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE))
    PMS_CPU_Wakeup(vector);
#endif
    /* Execute ISR Handler for this vector */
    ESAL_GE_ISR_HANDLER_EXECUTE(vector);

    /* Indicate that ISR is ended */
    ESAL_GE_ISR_END();

    /* Check if a thread is currently executing */
    if (!TCD_Current_Thread)
    {
        /* Inform the abstraction layer that a unsolicited context
           switch is no longer required (no thread is executing, so
           no context switch is required) */
        ESAL_GE_STK_UNSOL_SWITCH_DISABLE();

        /* Return to the scheduler from this interrupt service routine.
           NOTE - this call does not return. */
        ESAL_GE_ISR_OS_RETURN(TCCT_Schedule);

        /* Return to caller with NULL pointer (should never reach here). */
        return (NU_NULL);
    }

    /* Save stack pointer in thread's control block */
    ((TC_TCB *)TCD_Current_Thread) -> tc_stack_pointer = stack_pointer;

    /* Return a pointer to the stack pointer of the current thread */
    return (&((TC_TCB *)TCD_Current_Thread) -> tc_stack_pointer);
}


/************************************************************************
*
*   FUNCTION
*
*       TCCT_Dispatch_Nested_LISR
*
*   DESCRIPTION
*
*       This function is the nested interrupt service routine for
*       Nucleus PLUS.  It utilizes the Embedded Software Abstraction
*       Layer components to execute the appropriate interrupt service
*       routine and perform post-interrupt processing necessary for
*       the OS.  This handler is registered with ESAL during
*       initialization.
*
*   CALLED BY
*
*       ESAL                                Embedded Software Abstraction
*                                           Layer
*
*   CALLS
*
*       Interrupt Service Routine
*
*   INPUTS
*
*       vector                              Interrupt vector number
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID    TCCT_Dispatch_Nested_LISR(INT vector)
{
    /* Indicate that a nested ISR has started */
    ESAL_GE_ISR_START();
#ifdef CFG_NU_OS_SVCS_PWR_CORE_ENABLE
    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE)
    PMS_CPU_Wakeup(vector);
    #endif
#endif
    /* Execute ISR Handler for this vector */
    ESAL_GE_ISR_HANDLER_EXECUTE(vector);

    /* Indicate that a nested ISR has ended */
    ESAL_GE_ISR_END();
}


#if (NU_STACK_CHECKING == NU_TRUE)
/***********************************************************************
*
*   FUNCTION
*
*       NU_Check_Stack
*
*   DESCRIPTION
*
*       This function checks the current stack for overflow conditions.
*       Additionally, this function keeps track of the minimum amount
*       of stack space for the calling thread and returns the current
*       available stack space.
*
*   CALLED BY
*
*       Applications
*       Nucleus PLUS Services
*
*   CALLS
*
*       ERC_System_Error                    System error handler
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       available bytes in stack
*
***********************************************************************/
UNSIGNED  NU_Check_Stack(VOID)
{
    R1 TC_TCB       *thread_ptr;
    R2 VOID         *current_sp;
    R3 UNSIGNED     remaining = 0;
    NU_SUPERV_USER_VARIABLES

    /* Do not execute stack checking code from interrupt context */
    if (!ESAL_GE_ISR_EXECUTING())
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE_ISR();

        if (TCD_Current_Thread != NU_NULL)
        {
            /* Pickup the current thread pointer */
            thread_ptr = (TC_TCB *) TCD_Current_Thread;

            /* Get thread's current stack pointer */
            current_sp = ESAL_GE_RTE_SP_READ();

            /* Calculate the amount of available space on the stack */
            remaining = (UNSIGNED)((VOID_CAST)current_sp -
                                   (VOID_CAST)(thread_ptr -> tc_stack_start));

            /* Check if stack pointer is overflowed (or about to overflow) */
            if ( (current_sp < thread_ptr -> tc_stack_start) ||
                 (remaining < ESAL_GE_STK_MAX_FRAME_SIZE) )
            {
                /* Stack overflow condition exits (or is about to occur) */
                ERC_System_Error(NU_STACK_OVERFLOW);
            }
            /* Check for stack underflow */
            else if (current_sp > thread_ptr -> tc_stack_end)
            {
                /* Stack underflow condition exits */
                ERC_System_Error(NU_STACK_UNDERFLOW);
            }

            /* Check for new minimum stack size */
            if (remaining < thread_ptr -> tc_stack_minimum)
            {
                /* Save the new stack minimum */
                thread_ptr -> tc_stack_minimum = remaining;
            }
        }
        else
        {
            /* Get the current SP */
            current_sp = ESAL_GE_RTE_SP_READ();

            /* Calculate the amount of available space on the stack */
            remaining = (UNSIGNED)((VOID_CAST)current_sp -
                                   (VOID_CAST)ESAL_GE_STK_System_SP_Start_Get());
        }

        /* Return to user mode */
        NU_USER_MODE_ISR();
    }

    /* Return the remaining number of bytes on the stack */
    return(remaining);
}
#endif  /* NU_STACK_CHECKING == NU_TRUE */


#if (NU_PTR_ACCESS > 1)
/***********************************************************************
*
*   FUNCTION
*
*       TCCT_Current_Thread
*
*   DESCRIPTION
*
*       This function returns the current thread pointer.
*
*   CALLED BY
*
*       Application
*       NU_Allocate_Memory                  Allocates memory from a
*                                           dynamic memory pool
*       NU_Allocate_Aligned_Memory          Allocates memory from a
*                                           dynamic memory pool
*       NU_Retrieve_Events                  Retrieves various combinations
*                                           of event flags from a event group
*       NU_Receive_From_Mailbox             Receives a message
*                                           from a mailbox
*       NU_Send_To_Mailbox                  Sends a 4-word message
*                                           to a mailbox
*       NU_Broadcast_To_Mailbox             Sends a message to all mailboxes
*       NU_Receive_From_Pipe                Receives a message from a pipe
*       NU_Send_To_Pipe                     Sends a message to a pipe
*       NU_Broadcast_To_Pipe                Sends a message to all pipes
*       NU_Send_To_Front_Of_Pipe            Sends a message to the front
*                                           of a message pipe
*       NU_Allocate_Partition               Allocates a memory partition
*                                           from a memory partition pool
*       NU_Receive_From_Queue               Receives a message from a queue
*       NU_Send_To_Queue                    Sends a message to a queue
*       NU_Broadcast_To_Queue               Sends a message to all queues
*       NU_Send_To_Front_Of_Queue           Sends a message to the front
*                                           of a queue
*       NU_Obtain_Semaphore                 Obtains an semaphore
*
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Pointer to current thread
*
***********************************************************************/
VOID  *TCCT_Current_Thread(VOID)
{
    R1  VOID        *current_thread;
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts for critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Get pointer to current executing thread */
    current_thread = TCD_Current_Thread;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return the current thread pointer */
    return (current_thread);
}


/***********************************************************************
*
*   FUNCTION
*
*       TCCT_Set_Execute_Task
*
*   DESCRIPTION
*
*       This function sets the current task to execute variable under
*       protection against interrupts.
*
*   CALLED BY
*
*       NU_Relinquish                       Moves the calling task to the
*                                           end of other tasks at the same
*                                           priority level
*       TCC_Resume_Task                     Resumes a task
*       TCC_Signal_Shell                    Processes signals by calling
*                                           the task supplied signal
*       TCC_Suspend_Task                    Suspends a task.
*       TCC_Time_Slice                      Moves a task to the end of the
*                                           other tasks at the same
*                                           priority level
*                                           handling function
*       NU_Change_Preemption                Changes the preemption posture
*                                           of the calling task
*       TCS_Change_Priority                 Changes the priority of a task
*
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       task                                Pointer to task control
*                                           block
*
*   OUTPUTS
*
*       TCD_Execute_Task                    Modified variable
*
***********************************************************************/
VOID  TCCT_Set_Execute_Task(R1 TC_TCB *task)
{
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts for critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Now setup the TCD_Execute_Task pointer */
    TCD_Execute_Task = task;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();
}

#endif  /* (NU_PTR_ACCESS > 1) */

