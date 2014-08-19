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
*       tms_pause.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the supplemental pause routine for the timer
*       management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Pause_Timer                      Pause an application timer
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       timer.h                             Timer functions
*       thread_control.h                    Thread control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Pause_Timer
*
*   DESCRIPTION
*
*       This function pauses an application timer.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Get_Remaining_Time
*       TCCT_Schedule_Lock 
*       TMC_Stop_Timer
*       TCCT_Schedule_Unlock
*
*   INPUTS
*
*       timer_ptr                           Timer control block pointer
*
*   OUTPUTS
*
*       status
*           NU_INVALID_PAUSE_TIMER          Timer already paused
*           NU_INVALID_TIMER                Timer pointer not valid
*           NU_SUCCESS
*
***********************************************************************/
STATUS NU_Pause_Timer(NU_TIMER *timer)
{
    STATUS    status = NU_SUCCESS;
    UNSIGNED  remaining;
    TM_TCB   *actual_timer;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Save the remaining time, any validation
       issues with the timer control block will
       be checked in this function.  */
    NU_Get_Remaining_Time(timer, &remaining);

    /* Protect against simultaneous timer */
    TCCT_Schedule_Lock();

    /* Check if the specified timer pointer is valid and the timer
       has a valid ID. */
    if ((timer != NU_NULL) &&
        (timer -> tm_id == TM_TIMER_ID))
    {
        /* Check if the timer is not already paused and
           make sure it is an enabled timer. */
        if ((timer -> tm_enabled == NU_TRUE) &&
            (timer -> tm_paused_status == NU_FALSE))
        {
            /* Get a pointer to the actual timer control block from the application
               timer control block. */
            actual_timer = &(timer -> tm_actual_timer);

            /* Disable the timer */
            TMC_Stop_Timer(actual_timer);

            /* Mark the timer as disabled.  */
            timer -> tm_enabled = NU_FALSE;

            /* Indicate this timer is paused */
            timer -> tm_paused_status = NU_TRUE;

            /* Save the remaining time as the paused time */
            timer -> tm_paused_time = remaining;
        }
        else
        {
            /* Cannot pause a timer that has already been paused
               or a disabled timer */
            status = NU_INVALID_PAUSE_TIMER;
        }
    }
    else
    {
        status = NU_INVALID_TIMER;
    }

    /* Trace log */
    T_TIMER_PAUSE((VOID*)timer, status);

    /* Release protection.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode. */
    NU_USER_MODE();

    /* Return status to caller. */
    return (status);
}

