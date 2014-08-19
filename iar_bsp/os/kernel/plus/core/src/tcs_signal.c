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
*       tcs_signal.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the supplemental Signal routines for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Control_Signals                  Control signals
*       NU_Receive_Signals                  Receive signals
*       NU_Register_Signal_Handler          Register signal handler
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                           Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/* Define internal function calls.  */
VOID            TCC_Signal_Shell(VOID);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Control_Signals
*
*   DESCRIPTION
*
*       This function enables the specified signals and returns the
*       previous enable signal value back to the caller.  If a newly
*       enabled signal is present and a signal handler is registered,
*       signal handling is started.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCC_Signal_Shell                    Task signal execution
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect against other access
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       enable_signal_mask                  Enable signal mask
*
*   OUTPUTS
*
*       old_enable_mask
*
***********************************************************************/
UNSIGNED NU_Control_Signals(UNSIGNED enable_signal_mask)
{
    R1 TC_TCB       *task;                  /* Task pointer              */
    UNSIGNED        old_enable_mask = 1;    /* Old enable signal mask    */
    NU_SUPERV_USER_VARIABLES

    /* Pickup the task pointer.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Determine if the call is valid.  */
    NU_PARAM_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), old_enable_mask, 0);

    if (old_enable_mask != 0)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access.  */
        TCCT_Schedule_Lock();

        /* Pickup the old signal mask.  */
        old_enable_mask =  task -> tc_enabled_signals;

        /* Put the new mask in.  */
        task -> tc_enabled_signals =  enable_signal_mask;

        /* Trace log */
        T_SIG_CONTROL((VOID*)task, enable_signal_mask, old_enable_mask);

        /* Now, determine if the signal handler needs to be invoked.  */
        if ((enable_signal_mask & task -> tc_signals) &&
            (!task -> tc_signal_active) &&
            (task -> tc_signal_handler))
        {

            /* Signal processing is required.  */

            /* Indicate that signal processing is in progress.  */
            task -> tc_signal_active =  NU_TRUE;

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
            /* Release protection.  */
            TCCT_Schedule_Unlock();
        }

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SIG_CONTROL((VOID*)task, enable_signal_mask, old_enable_mask);
    }

    /* Return the old enable mask.  */
    return(old_enable_mask);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Receive_Signals
*
*   DESCRIPTION
*
*       This function returns the current signals back to the caller.
*       Note that the signals are cleared automatically.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect against other access
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       signals
*
***********************************************************************/
UNSIGNED NU_Receive_Signals(VOID)
{
    TC_TCB          *task;                  /* Task pointer              */
    UNSIGNED        signals = ~0;           /* Current signals           */
    NU_SUPERV_USER_VARIABLES

    /* Pickup the task pointer.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Determine if the call is valid.  */
    NU_PARAM_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), signals, 0);

    if (signals != 0)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access.  */
        TCCT_Schedule_Lock();

        /* Pickup the current events.  */
        signals =  task -> tc_signals;

        /* Clear the current signals.  */
        task -> tc_signals =  0;

        /* Trace log */
        T_SIG_RECV((VOID*)task, signals);

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SIG_RECV((VOID*)task, signals);
    }

    /* Return the signals to the caller.  */
    return(signals);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Register_Signal_Handler
*
*   DESCRIPTION
*
*       This function registers a signal handler for the calling task.
*       Note that if an enabled signal is present and this is the first
*       registered signal handler call, the signal is processed
*       immediately.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCC_Signal_Shell                    Signal execution shell
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect against other access
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       signal_handler                      Signal execution shell
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_TASK                     Not called from task thread
*       NU_INVALID_POINTER                  Signal handler pointer NULL
*
***********************************************************************/
STATUS NU_Register_Signal_Handler(VOID (*signal_handler)(UNSIGNED))
{
    R1 TC_TCB       *task;                  /* Task pointer              */
    STATUS           status = NU_SUCCESS;   /* Return status             */
    NU_SUPERV_USER_VARIABLES

    /* Pickup the task pointer.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Determine if the caller is a task.  */
    NU_ERROR_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    /* Determine if the signal handler is valid */
    NU_ERROR_CHECK((signal_handler == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access.  */
        TCCT_Schedule_Lock();

        /* Put the new signal handler in.  */
        task -> tc_signal_handler =  signal_handler;

        /* Trace log */
        T_SIG_REG_HANDLER((VOID*)task, (VOID*)signal_handler, status);

        /* Now, determine if the signal handler needs to be invoked.  */
        if ((task -> tc_enabled_signals & task -> tc_signals) &&
            (!task -> tc_signal_active) &&
            (task -> tc_signal_handler))
        {

            /* Signal processing is required.  */

            /* Indicate that signal processing is in progress.  */
            task -> tc_signal_active =  NU_TRUE;

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
            /* Release protection.  */
            TCCT_Schedule_Unlock();
        }

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SIG_REG_HANDLER((VOID*)task, (VOID*)signal_handler, status);
    }

    /* Return success.  */
    return(status);
}
