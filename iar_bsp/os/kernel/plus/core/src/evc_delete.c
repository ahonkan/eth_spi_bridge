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
*       evc_delete.c
*
*   COMPONENT
*
*       EV - Event Group Management
*
*   DESCRIPTION
*
*       This file contains the core Delete routine for the Event Group
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Event_Group               Delete an event group
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
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *EVD_Created_Event_Groups_List;
extern UNSIGNED         EVD_Total_Event_Groups;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Event_Group
*
*   DESCRIPTION
*
*       This function deletes an event group and removes it from the
*       list of created event groups.  All tasks suspended on the
*       event group are resumed.  Note that this function does not
*       free the memory associated with the event group control block.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove node from list
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect created list
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       event_group_ptr                     Event Group control
*                                           block ptr
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_GROUP                    Event group control block
*                                           pointer is invalid
*
***********************************************************************/
STATUS NU_Delete_Event_Group(NU_EVENT_GROUP *event_group_ptr)
{
    R1 EV_GCB       *event_group;           /* Event control block ptr   */
    EV_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    EV_SUSPEND      *next_ptr;              /* Next suspend block        */
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input event group pointer into internal pointer.  */
    event_group =  (EV_GCB *) event_group_ptr;

    /* Determine if the event group pointer is valid. */
    NU_ERROR_CHECK(((event_group == NU_NULL) || (event_group -> ev_id != EV_EVENT_ID)), status, NU_INVALID_GROUP);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the event group.  */
        TCCT_Schedule_Lock();

        /* Clear the event group ID.  */
        event_group -> ev_id =  0;

        /* Remove the event_group from the list of created event groups.  */
        NU_Remove_From_List(&EVD_Created_Event_Groups_List,
                            &(event_group -> ev_created));

        /* Decrement the total number of created event groups.  */
        EVD_Total_Event_Groups--;

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  event_group -> ev_suspension_list;

        /* Walk the chain task(s) currently suspended on the event_group.  */
        preempt =  0;
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_GROUP_DELETED.  */
            suspend_ptr -> ev_return_status =  NU_GROUP_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (EV_SUSPEND *) (suspend_ptr -> ev_suspend_link.cs_next);

            /* Trace log */
            T_EVT_GRP_DELETE((VOID*)event_group, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> ev_suspended_task,
                                                    NU_EVENT_SUSPEND);

            /* Determine if the next is the same as the current pointer.  */
            if (next_ptr == event_group -> ev_suspension_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Move the next pointer into the suspend block pointer.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Trace log */
        T_EVT_GRP_DELETE((VOID*)event_group, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the list of created
           event groups. */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_EVT_GRP_DELETE((VOID*)event_group, status);
    }

    /* Return a successful completion.  */
    return(status);
}

