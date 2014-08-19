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
*       tmf_remaining.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the information (fact) Remaining Time routine
*       for the Timer Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Get_Remaining_Time               Return remaining timer until
*                                           a timer expires
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

/* Define external inner-component global data references.  */

extern TM_TCB         *TMD_Active_Timers_List;
extern UNSIGNED        TMD_Timer_Start;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Get_Remaining_Time
*
*   DESCRIPTION
*
*       This function returns the remaining time before expiration for
*       the specified timer.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCCT_Schedule_Lock                  Protect active timer
*       TCCT_Schedule_Unlock                Release protection
*       TMCT_Read_Timer                     Returns the current
*                                           count-down timer
*
*   INPUTS
*
*       timer_ptr                           Pointer to the timer
*       remaining_time                      time until timer expiration
*
*   OUTPUTS
*
*       status
*           NU_INVALID_TIMER                If timer pointer invalid
*
***********************************************************************/
STATUS NU_Get_Remaining_Time(NU_TIMER *timer_ptr, UNSIGNED *remaining_time)
{
    R1 TM_TCB       *list_ptr;
    TM_TCB          *actual_timer;
    STATUS          status;
    TM_APP_TCB      *timer;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Move input timer pointer to internal pointer.  */
    timer =  (TM_APP_TCB *) timer_ptr;

    /* Protect against simultaneous access to the active timers list and
       changing of the specified timer. */
    TCCT_Schedule_Lock();

#if (CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0)

    /* Check if the LV timer */
    if (timer->tm_expiration_id == (UNSIGNED)timer)
    {
        /* Set the timer pointer to NULL */
        timer = NU_NULL;
    }

#endif  /* CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0 */

    /* Check if the specified timer pointer is valid and the timer
       has a valid ID. */
    if ((timer != NU_NULL) &&
        (timer -> tm_id == TM_TIMER_ID))
    {
        /* The specified timer is a valid timer.  Reflect this in the
           completion status. */
        status = NU_SUCCESS;

        /* Check if the specified timer is enabled */
        if (timer -> tm_enabled == NU_TRUE)
        {
            /* Get pointer to active timers list. */
            list_ptr = TMD_Active_Timers_List;

            /* Check if active timer list is empty. */
            if (list_ptr == NU_NULL)
            {
                /* No entries in list - return remaining time of 0. */
                *remaining_time = 0;
            }
            else
            {
                /* Get time remaining until next timer expiration. */
                *remaining_time = TMCT_Read_Timer();

                /* Check if the first timer's remaining time is greater than
                   TMD_Timer_Start - this can only occur when a timer at
                   the head of the list is disabled and the list has
                   not been updated. */
                if (list_ptr -> tm_remaining_time > TMD_Timer_Start)
                {
                    /* Adjust the remaining time based on the difference between
                       TMD_Timer_Start and the first timer's remaining time. */
                    *remaining_time += (list_ptr -> tm_remaining_time - TMD_Timer_Start);
                }

                /* Get a pointer to the actual timer control block from the application
                   timer control block. */
                actual_timer = &(timer -> tm_actual_timer);

                /* Check if the specified timer is not the first timer on the active
                   timer list. */
                if (list_ptr != actual_timer)
                {
                    /* Loop through all other timers in the active timer list. */
                    do
                    {
                        /* Move to next timer in the list. */
                        list_ptr = list_ptr -> tm_next_timer;

                        /* Check if the list pointer is pointing to the
                           head of the list. */
                        if (list_ptr == TMD_Active_Timers_List)
                        {
                            /* Set time remaining to 0 since entire
                               list has been searched without finding
                               the specified timer. */
                            *remaining_time = 0;

                            /* Break out of the while loop. */
                            break;
                        }
                        else
                        {
                            /* Update remaining time with this timer's
                               remaining time. */
                            *remaining_time += list_ptr -> tm_remaining_time;
                        }

                    /* Keep looping until specified timer is found. */
                    } while (list_ptr != actual_timer);
                }
            }
        }
        /* Timer is not enabled, check if the timer is paused */
        else if (timer->tm_paused_status == NU_TRUE)
        {
            /* The timer has previously been paused read the
               time that was saved when it was paused */
            *remaining_time = timer -> tm_paused_time;
        }
        else
        {
            /* Conflicting status values exist. Report
               this as an invalid timer. */
            status =  NU_INVALID_TIMER;
        }
    }
    else
    {
        /* The specified timer is not valid.  Indicate this in
           the completion status. */
        status =  NU_INVALID_TIMER;
    }

    /* Remove protection of timer list and specified timer. */
    TCCT_Schedule_Unlock();

    /* Return to user mode. */
    NU_USER_MODE();

    /* Return status to caller. */
    return (status);
}

