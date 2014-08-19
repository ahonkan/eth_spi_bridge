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
*       evf_info.c
*
*   COMPONENT
*
*       EV - Event Group Management
*
*   DESCRIPTION
*
*       This file contains routines to obtain Information about
*       the Event Group Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Event_Group_Information          Retrieve event group info
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       event_group.h                       Event group functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/event_group.h"
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Event_Group_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified event
*       group. However, if the supplied event group pointer is invalid,
*       the function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect event group
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       event_group_ptr                     Pointer to the event group
*       name                                Destination for the name
*       event_flags                         Pointer to a variable to
*                                           hold the current event flags
*       tasks_waiting                       Destination for the tasks
*                                           waiting count
*       first_task                          Destination for the pointer
*                                           to the first task waiting
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid event group
*                                           pointer is supplied
*           NU_INVALID_GROUP                If event group pointer is
*                                           not valid
*
***********************************************************************/
STATUS NU_Event_Group_Information(NU_EVENT_GROUP *event_group_ptr, CHAR *name,
                                  UNSIGNED *event_flags, UNSIGNED *tasks_waiting,
                                  NU_TASK **first_task)
{
    EV_GCB          *event_group;           /* Event control block ptr   */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input event group pointer into internal pointer.  */
    event_group =  (EV_GCB *) event_group_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this event_group id is valid.  */
    if ((event_group != NU_NULL) && (event_group -> ev_id == EV_EVENT_ID))
    {
        /* Setup protection of the event_group.  */
        TCCT_Schedule_Lock();

        /* The event_group pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the event_group's name.  */
        strncpy(name, event_group -> ev_name, NU_MAX_NAME);

        /* Return the current event flags.  */
        *event_flags =  event_group -> ev_current_events;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  event_group -> ev_tasks_waiting;
        if (event_group -> ev_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task =  (NU_TASK *)
                (event_group -> ev_suspension_list) -> ev_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection of the event group.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the event group pointer is invalid.   */
        completion =  NU_INVALID_GROUP;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}

