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
*       tcc_signal.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core signal shell routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCC_Signal_Shell                    Signal execution shell
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#ifdef  CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
#include        "os/kernel/process/core/proc_core.h"
#endif
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern TC_TCB               *TCD_Priority_List[TC_PRIORITIES+1];
extern UNSIGNED             TCD_Priority_Groups;
extern DATA_ELEMENT         TCD_Sub_Priority_Groups[TC_MAX_GROUPS];
extern UNSIGNED_CHAR        TCD_Lowest_Set_Bit[];
extern INT                  TCD_Highest_Priority;

/* Define external inner-component function calls that are not available to
   other components.  */

VOID            TCCT_Signal_Exit(VOID);

/************************************************************************
*
*   FUNCTION
*
*       TCC_Signal_Shell
*
*   DESCRIPTION
*
*       This function processes signals by calling the task supplied
*       signal handling function.  When signal handling is completed,
*       the task is placed in the appropriate state.
*
*   CALLED BY
*
*       NU_Control_Signals                  Control signals
*       NU_Register_Signal_Handler          Register signal handler
*       Nu_Send_Signals                     Send signals to a task
*
*   CALLS
*
*       task's signal handling routine
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect against other access
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TCCT_Signal_Exit                    Signal handling exit routine
*       TCCT_Schedule_Unlock                Release protection
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
VOID  TCC_Signal_Shell(VOID)
{
    R2 UNSIGNED     signals;                /* Signals to send to task   */
    INT             index;                  /* Working index variable    */
    DATA_ELEMENT    temp;                   /* Temporary variable        */
    R1 TC_TCB       *task;                  /* Task pointer              */

#if defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE)
    VOID            *task_return_addr;      /* save/restore return addr  */
#endif

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Point at the current task.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Protect against simultaneous access.  */
    TCCT_Schedule_Lock();

    /* Process while there are signals to handle.  */
    while (task -> tc_signals & task -> tc_enabled_signals)
    {

        /* Pickup the signals and clear them.  */
        signals =  task -> tc_signals;
        task -> tc_signals =  0;

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Call the application signal handling function, if there still is
           one.  */
        if (task -> tc_signal_handler)
        {
            /* Trace log */
            T_SIG_HANDLER_RUNNING((VOID*)task, (VOID*)task -> tc_signal_handler);

#if defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE)
            /* Determine if task is not part kernel process */
            if (PROC_Scheduled_CB -> kernel_mode == NU_FALSE)
            {
                /* Save task return address. */
                task_return_addr = task -> tc_return_addr;

                /* Run task in user mode. */
                PROC_AR_User_Mode();
            }
#endif

            /* Call signal handler.  (always in User mode) */
            (*(task -> tc_signal_handler))(signals);

#if defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE)
            /* Determine if task is not part kernel process */
            if (PROC_Scheduled_CB -> kernel_mode == NU_FALSE)
            {
                /* Switch to supervisor mode */
                PROC_SUPERVISOR_MODE();

                /* Restore task return address. */
                task -> tc_return_addr = task_return_addr;
            }
#endif

            /* Trace log */
            T_SIG_HANDLER_STOPPED((VOID*)task, (VOID*)task -> tc_signal_handler);
        }

        /* Protect against simultaneous access again.  */
        TCCT_Schedule_Lock();
    }

    /* At this point, signals have been exhausted and protection is in
       force.  */

    /* Clear the signal in process flag.  */
    task -> tc_signal_active =  NU_FALSE;

    /* Determine how the signal handler was called.  Either in a solicited or
       an unsolicited manner.  */
    if (task -> tc_saved_stack_ptr)
    {

        /* Determine if the saved status still indicates that the task should
           be suspended.  */
        if (task -> tc_saved_status != NU_READY)
        {

            /* Suspend the task.  */
            task -> tc_status =  task -> tc_saved_status;

            /* Remove the task from the ready list.  */

            /* Determine if the task is the only one on the list.  */
            if (task -> tc_ready_next == task)
            {

                /* Only task on the list.  Clear the task's pointers and
                   clear the entry in the priority table.  */
                task -> tc_ready_next =        NU_NULL;
                task -> tc_ready_previous =    NU_NULL;
                *(task -> tc_priority_head) =  NU_NULL;

                /* Clear the sub-priority group.  */
                *(task -> tc_sub_priority_ptr) &= (DATA_ELEMENT) (~(task -> tc_sub_priority)); 

                /* Determine if the main priority group needs to be cleared.
                   This is only true if there are no other bits set in this
                   sub-priority.  */
                if (*(task -> tc_sub_priority_ptr) == 0)
                {
                    /* Clear the main priority group bit.  */
                    TCD_Priority_Groups =
                        TCD_Priority_Groups & (DATA_ELEMENT)~(task -> tc_priority_group);
                }

                /* Determine if this priority group was the highest in the
                   system.  */
                if (task -> tc_priority == (DATA_ELEMENT) TCD_Highest_Priority)
                {

                    /* Determine the highest priority task in the system.  */
                    if (TCD_Priority_Groups == 0)
                    {

                        /* Re-initialize the highest priority variable and
                           clear the current task pointer.  */
                        TCD_Highest_Priority =  TC_PRIORITIES;
                    }
                    else
                    {

                        /* Find the next highest priority task.  */
                        if (TCD_Priority_Groups & TC_HIGHEST_MASK)
                        {
                            /* Base of sub-group is 0.  */
                            index =  0;
                        }
                        else if (TCD_Priority_Groups & TC_NEXT_HIGHEST_MASK)
                        {
                            /* Base of sub-group is 8.  */
                            index =  8;
                        }
                        else if (TCD_Priority_Groups & TC_NEXT_LOWEST_MASK)
                        {
                            /* Base of sub-group is 16.  */
                            index =  16;
                        }
                        else
                        {
                            /* Base of sub-group is 24.  */
                            index =  24;
                        }

                        /* Calculate the highest available priority.  */
                        index =  index + TCD_Lowest_Set_Bit[(INT)
                           ((TCD_Priority_Groups >> index) & TC_HIGHEST_MASK)];

                        /* Verify index is within bounds */
                        if (index >= TC_MAX_GROUPS)
                        {
                            index = TC_MAX_GROUPS - 1;
                        }

                        /* Get the mask of the priority within the group of
                           8 priorities.  */
                        temp =  TCD_Sub_Priority_Groups[index];

                        /* Calculate the actual priority.  */
                        TCD_Highest_Priority =
                            (index << 3) + TCD_Lowest_Set_Bit[temp];
                    }
                }
            }
            else
            {

                /* Not the only task ready at the same priority level.  */

                /* Remove from the linked-list.  */
                (task -> tc_ready_previous) -> tc_ready_next =
                                                 task -> tc_ready_next;
                (task -> tc_ready_next) -> tc_ready_previous =
                                                 task -> tc_ready_previous;

                /* See if the task being suspended is the current.  */
                if (*(task -> tc_priority_head) == task)
                {
                    /* Update the head of this priority list.  */
                    *(task -> tc_priority_head) =  task -> tc_ready_next;
                }

                /* Clear the task's pointers.  */
                task -> tc_ready_next =        NU_NULL;
                task -> tc_ready_previous =    NU_NULL;
            }

            /* Put the next task to execute in TCD_Execute_Task.  */
            TCCT_Set_Execute_Task(TCD_Priority_List[TCD_Highest_Priority]);

        }

        /* At this point, just exit back to the system.  Note that the
           signal exit routine clears the scheduling protection.  */
        TCCT_Signal_Exit();

        /* Code should never reach here.  This line ensures compiler doesn't try
           to optimize return from this function and cause a stack pointer
           problem. */
        ESAL_GE_STK_NO_RETURN();
    }

    /* A signal handler was called from the current task.  Nothing needs
       to be done except to release protection.  */
    TCCT_Schedule_Unlock();
}
