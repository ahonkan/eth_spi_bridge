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
*       tms_create.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the supplemental Create routine for the timer
*       management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Timer                     Create an application timer
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       timer.h                             Timer functions
*       thread_control.h                    Thread control functions
*       common_services.h                   Common services
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define external inner-component global data references.  */

extern CS_NODE        *TMD_Created_Timers_List;
extern UNSIGNED        TMD_Total_Timers;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Timer
*
*   DESCRIPTION
*
*       This function creates an application timer and places it on the
*       list of created timers.  The timer is activated if designated by
*       the enable parameter.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Add node to linked-list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Data structure protect
*       TCCT_Schedule_Unlock                Un-protect data structure
*       NU_Control_Timer                    Enable the new timer
*
*   INPUTS
*
*       timer_ptr                           Timer control block pointer
*       name                                Timer name
*       expiration_routine                  Timer expiration routine
*       id                                  Timer expiration ID
*       initial_time                        Initial expiration time
*       reschedule_time                     Reschedule expiration time
*       enable                              Automatic enable option
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_TIMER                    Indicates timer pointer is
*                                           NULL
*       NU_INVALID_FUNCTION                 Indicates timer expiration
*                                           function pointer is NULL
*       NU_INVALID_ENABLE                   Indicates enable parameter
*                                           is invalid
*
***********************************************************************/
STATUS NU_Create_Timer(NU_TIMER *timer_ptr, CHAR *name,
                       VOID (*expiration_routine)(UNSIGNED), UNSIGNED id,
                       UNSIGNED initial_time, UNSIGNED reschedule_time,
                       OPTION enable)
{
    R1 TM_APP_TCB   *timer;                 /* Timer control block ptr   */
    STATUS          status = NU_SUCCESS;    /* Completion status        */
    NU_SUPERV_USER_VARIABLES

    /* Move input timer pointer into internal pointer */
    timer =  (TM_APP_TCB *) timer_ptr;

    /* Check the parameters to the create timer function */
    NU_ERROR_CHECK(((timer == NU_NULL) || (timer -> tm_id == TM_TIMER_ID)), status, NU_INVALID_TIMER);

    /* Check for an invalid expiration function pointer */
    NU_ERROR_CHECK((expiration_routine == NU_NULL), status, NU_INVALID_FUNCTION);

    /* Check for an invalid time value */
    NU_ERROR_CHECK((initial_time == 0), status, NU_INVALID_OPERATION);

    /* Checkf for an invalid enable parameter */
    NU_ERROR_CHECK(((enable != NU_ENABLE_TIMER) && (enable != NU_DISABLE_TIMER)), status, NU_INVALID_ENABLE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(timer, TM_APP_TCB);

        /* Fill in the timer name. */
        strncpy(timer -> tm_name, name, (NU_MAX_NAME - 1));

        /* Load the timer with the appropriate values.  */
        timer -> tm_expiration_routine =            expiration_routine;
        timer -> tm_expiration_id =                 id;
        timer -> tm_initial_time =                  initial_time;
        timer -> tm_reschedule_time =               reschedule_time;
        timer -> tm_actual_timer.tm_timer_type =    TM_APPL_TIMER;
        timer -> tm_actual_timer.tm_information =   (VOID *) timer;

        /* Protect against access to the list of created timers.  */
        TCCT_Schedule_Lock();

        /* At this point the timer is completely built.  The ID can now be
           set and it can be linked into the created timer list.  */
        timer -> tm_id =                     TM_TIMER_ID;

        /* Link the timer into the list of created timers and increment the
           total number of timers in the system.  */
        NU_Place_On_List(&TMD_Created_Timers_List, &(timer -> tm_created));
        TMD_Total_Timers++;

        /* Trace log */
        T_TIMER_CREATE((VOID*)timer, (VOID*)expiration_routine, name, id, initial_time,                         \
        reschedule_time, enable, status);

        /* Release protection against access to the list of created timers.  */
        TCCT_Schedule_Unlock();

        /* Determine if the timer should be enabled.  */
        if (enable == NU_ENABLE_TIMER)
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
        T_TIMER_CREATE((VOID*)timer, (VOID*)expiration_routine, name, id, initial_time,                         \
        reschedule_time, enable, status);
    }

    /* Return successful completion.  */
    return(status);
}
