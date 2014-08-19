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
*       tmf_info.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains information (fact) routines for the Timer
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Timer_Information                Return information about the
*                                           application timer
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
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Timer_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified timer.
*       However, if the supplied timer pointer is invalid, the
*       function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect active timer
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       timer_ptr                           Pointer to the timer
*       name                                Destination for the name
*       enable                              Destination for the enable
*                                           posture
*       expirations                         Destination for the total
*                                           number of expirations
*       id                                  Destination for the timer id
*       initial_time                        Destination for the initial
*                                           time
*       reschedule_time                     Destination for the
*                                           reschedule time
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid timer pointer
*                                           is supplied
*           NU_INVALID_TIMER                If timer pointer invalid
*
***********************************************************************/
STATUS NU_Timer_Information(NU_TIMER *timer_ptr, CHAR *name,
                            OPTION *enable, UNSIGNED *expirations,
                            UNSIGNED *id, UNSIGNED *initial_time,
                            UNSIGNED *reschedule_time)
{
    TM_APP_TCB      *timer;                 /* Timer control block ptr   */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input timer pointer into internal pointer.  */
    timer =  (TM_APP_TCB *) timer_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Protect the active list.  */
    TCCT_Schedule_Lock();

    /* Determine if this timer ID is valid.  */
    if ((timer != NU_NULL) && (timer -> tm_id == TM_TIMER_ID))
    {

        /* The timer pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the timer's name.  */
        strncpy(name, timer -> tm_name, NU_MAX_NAME);

        /* Determine if the timer is enabled or disabled.  */
        if (timer -> tm_enabled)
        {
            *enable =  NU_ENABLE_TIMER;
        }
        else
        {
            *enable =  NU_DISABLE_TIMER;
        }

        /* Fill in the remaining information.  */
        *expirations =          timer -> tm_expirations;
        *id =                   timer -> tm_expiration_id;
        *initial_time =         timer -> tm_initial_time;
        *reschedule_time =      timer -> tm_reschedule_time;
    }
    else
    {
        /* Indicate that the timer pointer is invalid.   */
        completion =  NU_INVALID_TIMER;
    }

    /* Release protection.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
