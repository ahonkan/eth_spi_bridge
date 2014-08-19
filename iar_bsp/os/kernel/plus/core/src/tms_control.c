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
*       tms_control.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the supplemental Control routine for the
*       timer management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Control_Timer                    Enable/Disable application
*                                           timer
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

/* Define internal function prototypes.  */

VOID            TMC_Start_Timer(TM_TCB *timer, UNSIGNED time);
VOID            TMC_Stop_Timer(TM_TCB *timer);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Control_Timer
*
*   DESCRIPTION
*
*       This function either enables or disables the specified timer.
*       If the timer is already in the desired state, simply leave it
*       alone.
*
*   CALLED BY
*
*       Application
*       NU_Create_Timer                     Creates an application timer
*       NU_Reset_Timer                      Resets a application timer
*
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect the active list
*       TCCT_Schedule_Unlock                Release protection
*       TMC_Start_Timer                     Start a timer
*       TMC_Stop_Timer                      Stop a timer
*
*   INPUTS
*
*       app_timer                           Timer control block pointer
*       enable                              Disable/enable timer option
*
*   OUTPUTS
*
*       NU_SUCCESS                          If service is successful
*       NU_INVALID_TIMER                    Indicates the timer pointer
*                                           is invalid
*       NU_INVALID_ENABLE                   Indicates enable parameter
*                                           is invalid
*       NU_TIMER_PAUSED                     Indicates the timer is
*                                           paused
*
***********************************************************************/
STATUS NU_Control_Timer(NU_TIMER *app_timer, OPTION enable)
{
    R1 TM_APP_TCB   *timer;                 /* Timer control block ptr   */
    TM_TCB          *timer_ptr;             /* Actual timer pointer      */
    UNSIGNED        time;                   /* Variable to hold request  */
    STATUS          status = NU_SUCCESS;    /* Completion status        */
    NU_SUPERV_USER_VARIABLES

    /* Move input timer pointer to internal pointer.  */
    timer =  (TM_APP_TCB *) app_timer;

    /* Verify timer pointer */
    NU_ERROR_CHECK((timer == NU_NULL), status, NU_INVALID_TIMER);

    /* Verify timer pointer */
    NU_ERROR_CHECK((timer -> tm_id != TM_TIMER_ID), status, NU_INVALID_TIMER);

    /* Verify enable parameter */
    NU_ERROR_CHECK(((enable != NU_ENABLE_TIMER) && (enable != NU_DISABLE_TIMER)), status, NU_INVALID_ENABLE);

    /* Verify enable parameter */
    NU_ERROR_CHECK((timer -> tm_paused_status == NU_TRUE), status, NU_TIMER_PAUSED);

#if (CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0)

    /* Verify timer pointer to prevent application controlling / disabling LV timer */
    NU_ERROR_CHECK((timer->tm_expiration_id == ((UNSIGNED)timer | 0x1)), status, NU_INVALID_TIMER);

#endif  /* CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0 */

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the active timer list.  */
        TCCT_Schedule_Lock();

        /* Setup pointer to actual timer part of the control block.  */
        timer_ptr =  &(timer -> tm_actual_timer);

        /* Determine what type of request is present.  */
        if ((enable == NU_ENABLE_TIMER) && (!timer -> tm_enabled))
        {

            /* Enable timer request is present and timer is currently disabled.  */

            /* Determine how to setup the remaining field in the actual timer. */
            if (timer -> tm_expirations)
            {
                /* Use reschedule time since this timer has expired previously. */
                time =  timer -> tm_reschedule_time;
            }
            else
            {
                /* Use initial time since this timer has never expired.  */
                time =  timer -> tm_initial_time;
            }

            /* Mark the application timer as enabled.  */
            timer -> tm_enabled =  NU_TRUE;

            /* Call the start timer routine to actually start the timer.  */
            TMC_Start_Timer(&(timer -> tm_actual_timer), time);
        }
        else if ((enable == NU_DISABLE_TIMER) && (timer -> tm_enabled))
        {

            /* Disable timer request is present and timer is currently enabled.  */
            TMC_Stop_Timer(timer_ptr);

            /* Mark the timer as disabled.  */
            timer -> tm_enabled =  NU_FALSE;
        }

        /* Trace log */
        T_TIMER_CONTROL((VOID*)timer, enable, status);

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TIMER_CONTROL((VOID*)timer, enable, status);
    }

    /* Return the completion status.  */
    return(status);
}
