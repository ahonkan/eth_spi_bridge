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
*       tms_delete.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the supplemental Delete routine for the timer
*       management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Timer                     Delete an application timer
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

/* Define external inner-component global data references.  */

extern CS_NODE        *TMD_Created_Timers_List;
extern UNSIGNED        TMD_Total_Timers;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Timer
*
*   DESCRIPTION
*
*       This function deletes an application timer and removes it from
*       the list of created timers.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove node from list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created list
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       timer_ptr                           Timer control block pointer
*
*   OUTPUTS
*
*       status
*           NU_NOT_DISABLED                 Timer not disabled first
*           NU_SUCCESS
*           NU_INVALID_TIMER                Indicates the timer pointer
*                                           is NULL or not a timer
*
***********************************************************************/
STATUS NU_Delete_Timer(NU_TIMER *timer_ptr)
{
    TM_APP_TCB      *timer;                 /* Timer control block ptr  */
    STATUS          status = NU_SUCCESS;    /* Completion status        */
    NU_SUPERV_USER_VARIABLES

    /* Move input timer pointer into internal pointer.  */
    timer =  (TM_APP_TCB *) timer_ptr;

    /* Check the parameters to the delete timer function */
    NU_ERROR_CHECK((timer == NU_NULL), status, NU_INVALID_TIMER);

    /* Checkf for an invalid timer pointer */
    NU_ERROR_CHECK((timer -> tm_id != TM_TIMER_ID), status, NU_INVALID_TIMER);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Use system protect to protect the active timer list temporarily.  */
        TCCT_Schedule_Lock();

        /* Determine if the timer is currently disabled. */
        if (timer -> tm_enabled)
        {
            /* Error, indicate to the caller that the timer is currently active. */
            status =  NU_NOT_DISABLED;
        }
        else
        {
            /* Clear the timer ID.  */
            timer -> tm_id =  0;
        }

        /* Determine if an error was detected.  */
        if (status == NU_SUCCESS)
        {
            /* Remove the timer from the list of created timers.  */
            NU_Remove_From_List(&TMD_Created_Timers_List, &(timer -> tm_created));

            /* Decrement the total number of created timers.  */
            TMD_Total_Timers--;
        }

        /* Trace log */
        T_TIMER_DELETE((VOID*)timer, status);

        /* Release protection against access to the list of created timers.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TIMER_DELETE((VOID*)timer, status);
    }

    /* Return completion status.  */
    return(status);
}
