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
*       tms_reset.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the supplemental Reset routine for the timer
*       management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Reset_Timer                      Reset application timer
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
*       NU_Reset_Timer
*
*   DESCRIPTION
*
*       This function resets the specified application timer.  Note that
*       the timer must be in a disabled state prior to this call.  The
*       timer is activated after it is reset if the enable parameter
*       specifies automatic activation.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect active list
*       TCCT_Schedule_Unlock                Release protection
*       NU_Control_Timer                    Enable/disable timer
*
*   INPUTS
*
*       timer_ptr                           Timer control block pointer
*       expiration_routine                  Timer expiration routine
*       initial_time                        Initial expiration time
*       reschedule_time                     Reschedule expiration time
*       enable                              Automatic enable option
*
*   OUTPUTS
*
*       status
*           NU_NOT_DISABLED                 Timer not disabled first
*           NU_SUCCESS                      Successful completion
*           NU_INVALID_TIMER                Indicates timer pointer is
*                                           invalid
*           NU_INVALID_FUNCTION             Indicates that expiration
*                                           function pointer is NULL
*           NU_INVALID_ENABLE               Indicates enable parameter
*                                           is invalid
*
***********************************************************************/
STATUS NU_Reset_Timer(NU_TIMER *timer_ptr,
                      VOID (*expiration_routine)(UNSIGNED),
                      UNSIGNED initial_time, UNSIGNED reschedule_time,
                      OPTION enable)
{
    R1 TM_APP_TCB   *timer;                 /* Timer control block ptr  */
    STATUS          status = NU_SUCCESS;    /* Completion status        */
    NU_SUPERV_USER_VARIABLES

    /* Move input timer pointer into internal pointer */
    timer =  (TM_APP_TCB *) timer_ptr;

    /* Check the parameters to the reset timer function */
    NU_ERROR_CHECK((timer == NU_NULL), status, NU_INVALID_TIMER);

    /* Check for an invalid timer pointer */
    NU_ERROR_CHECK((timer -> tm_id != TM_TIMER_ID), status, NU_INVALID_TIMER);

    /* Check for an invalid time value */
    NU_ERROR_CHECK((initial_time == 0), status, NU_INVALID_OPERATION);

    /* Check for an invalid expiration function pointer */
    NU_ERROR_CHECK((expiration_routine == NU_NULL), status, NU_INVALID_FUNCTION);

    /* Check for an invalid enable parameter */
    NU_ERROR_CHECK(((enable != NU_ENABLE_TIMER) && (enable != NU_DISABLE_TIMER)), status, NU_INVALID_ENABLE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against access to the active timer list.  */
        TCCT_Schedule_Lock();

        /* Determine if this timer is active.  An active timer cannot be
           reset.  */
        if (timer -> tm_enabled)
        {
            /* Indicate that the timer is active by returning the proper status. */
            status =  NU_NOT_DISABLED;
        }
        else
        {

            /* Load the timer with the appropriate values.  */
            timer -> tm_expiration_routine =    expiration_routine;
            timer -> tm_expirations =           0;
            timer -> tm_initial_time =          initial_time;
            timer -> tm_reschedule_time =       reschedule_time;

            /* Clear any paused state */
            timer -> tm_paused_status =         NU_FALSE;
        }

        /* Trace log */
        T_TIMER_RESET((VOID*)timer, (VOID*)expiration_routine, initial_time,
        reschedule_time, enable, status);

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Determine if the timer needs to be enabled.  */
        if ((status == NU_SUCCESS) && (enable == NU_ENABLE_TIMER))
        {
            /* Activate the timer.  */
            NU_Control_Timer(timer_ptr, NU_ENABLE_TIMER);
        }

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TIMER_RESET((VOID*)timer, (VOID*)expiration_routine, initial_time,
        reschedule_time, enable, status);
    }

    /* Return completion status.  */
    return(status);
}
