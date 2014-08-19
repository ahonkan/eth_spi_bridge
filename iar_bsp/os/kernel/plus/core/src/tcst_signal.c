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
*       tcst_signal.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains supplemental Signal routine for thread
*       control with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Send_Signals                     Send signals to a task
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/* Define external function references */
extern VOID                 TCC_Signal_Shell(VOID);

#if (NU_POSIX_INCLUDED == NU_TRUE)

extern VOID                 POSIX_Signal_Handle(VOID);

#endif  /* NU_POSIX_INCLUDED == NU_TRUE */

/***********************************************************************
*
*   FUNCTION
*
*       NU_Send_Signals
*
*   DESCRIPTION
*
*       This function sends the specified task the specified signals.
*       If enabled, the specified task is setup in order to process the
*       signals.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       ESAL_GE_STK_Solicited_Set           Sets initial state of a
*                                           solicited stack frame
*       TCC_Resume_Task                     Resume task that is
*                                           suspended
*       TCC_Signal_Shell                    Signal execution shell
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Control to system
*       TCCT_Schedule_Lock                  Protect against other access
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       task_ptr                            Task pointer
*       signal_mask                         Signals to send to the task
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_TASK                     Task pointer is invalid
*
***********************************************************************/
STATUS NU_Send_Signals(NU_TASK *task_ptr, UNSIGNED signal_mask)
{
    R1 TC_TCB      *task;                   /* Task control block ptr    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input task pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Determine if the task pointer is valid.  */
    NU_ERROR_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access.  */
        TCCT_Schedule_Lock();

        /* Or the new signals into the current signals.  */
        task -> tc_signals =  task -> tc_signals | signal_mask;

        /* Trace log */
        T_SIG_SEND((VOID*)task, signal_mask, status);

        /* Now, determine if the signal handler needs to be invoked.  */
        if ((task -> tc_signals & task -> tc_enabled_signals) &&
            (!task -> tc_signal_active) &&
            (task -> tc_status != NU_TERMINATED) &&
            (task -> tc_status != NU_FINISHED) &&
            (task -> tc_signal_handler))
        {

            /* Indicate that signal processing is in progress.  */
            task -> tc_signal_active =  NU_TRUE;

            /* Signal processing is required.  Determine if the task is sending
               signals to itself or if the calling thread is not the current
               task.  */
            if (task == (TC_TCB *) TCD_Current_Thread)
            {

                /* Task sending signals to itself.  */

                /* Clear the saved stack pointer to indicate that this is an
                   in line signal handler call.  */
                task -> tc_saved_stack_ptr =  NU_NULL;

                /* Release protection from multiple access.  */
                TCCT_Schedule_Unlock();

                /* Call the signal handling shell. */
                TCC_Signal_Shell();
            }
            else
            {
                /* Copy the current status, stack pointer, and return address to the signal save
                   areas.  */
                task -> tc_saved_status =           task -> tc_status;
                task -> tc_saved_stack_ptr =        task -> tc_stack_pointer;
                task -> tc_saved_return_addr =      task -> tc_return_addr;

                /* Align the stack pointer address as required */
                task -> tc_stack_pointer = ESAL_GE_STK_ALIGN(task -> tc_stack_pointer);

#if (NU_POSIX_INCLUDED == NU_FALSE)

                /* Set solicited stack frame */
                task -> tc_stack_pointer = ESAL_GE_STK_Solicited_Set(task -> tc_stack_start,
                                                                     task -> tc_stack_pointer,
                                                                     TCC_Signal_Shell);
#else

                /* Set solicited stack frame */
                task -> tc_stack_pointer = ESAL_GE_STK_Solicited_Set(task -> tc_stack_start,
                                                                     task -> tc_stack_pointer,
                                                                     POSIX_Signal_Handle);

#endif  /* NU_POSIX_INCLUDED == NU_FALSE */

                /* Determine if the target task is currently suspended.  If it is
                   suspended for any other reason than a pure suspend, resume
                   it.  */
                if ((task -> tc_status != NU_READY) &&
                    (task -> tc_status != NU_PURE_SUSPEND))
                {
                    /* Resume the target task and check for preemption.  */
                    if (TCC_Resume_Task(task_ptr, task -> tc_status))
                    {
                    	/* Trace log */
                    	T_TASK_SUSPEND((VOID*)TCCT_Current_Thread(), task -> tc_status);

                        /* Preemption needs to take place. */
                        TCCT_Control_To_System();
                    }
                }
            }
        }

        /* Release protection, no signals are currently enabled.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
    	/* Trace log */
    	T_SIG_SEND((VOID*)task, signal_mask, status);
    }

    /* Return status.  */
    return(status);
}
