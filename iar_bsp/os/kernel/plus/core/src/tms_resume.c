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
*       tms_resume.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the supplemental resume routine for the timer
*       management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Resume_Timer                     Resume an application timer
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
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "services/nu_trace_os_mark.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Resume_Timer
*
*   DESCRIPTION
*
*       This function resumes a paused application timer.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Reset_Timer
*
*   INPUTS
*
*       timer_ptr                           Timer control block pointer
*
*   OUTPUTS
*
*       status
*           NU_INVALID_RESUME_TIMER         Timer not previously paused
*           NU_INVALID_TIMER                Timer pointer not valid
*           NU_SUCCESS
*
***********************************************************************/
STATUS NU_Resume_Timer(NU_TIMER *timer)
{
    STATUS status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if the specified timer pointer is valid, the timer
       has a valid ID and it is a paused timer. */
    if ((timer != NU_NULL) &&
        (timer -> tm_id == TM_TIMER_ID))
    {
        /* Verify the timer is already paused */
        if (timer -> tm_paused_status == NU_TRUE)
        {
            /* Trace log */
            T_TIMER_RESUME((VOID*)timer, status);

            /* Re-enable the timer with the remaining time and
               original reschedule time.  Reset timer clears the
               pause state. */
            status = NU_Reset_Timer(timer, timer -> tm_expiration_routine,
                                    timer -> tm_paused_time, timer -> tm_reschedule_time,
                                    NU_ENABLE_TIMER);
        }
        else
        {
            /* Cannot resume a timer unless it has been paused */
            status = NU_INVALID_RESUME_TIMER;

            /* Trace log */
            T_TIMER_RESUME((VOID*)timer, status);
        }
    }
    else
    {
        status = NU_INVALID_TIMER;

        /* Trace log */
        T_TIMER_RESUME((VOID*)timer, status);
    }

    /* Return to user mode. */
    NU_USER_MODE();

    /* Return status to caller. */
    return (status);
}

