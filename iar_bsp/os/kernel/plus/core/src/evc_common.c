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
*       evc_common.c
*
*   COMPONENT
*
*       EV - Event Group Management
*
*   DESCRIPTION
*
*       This file contains the core common routines for the
*       Event Group Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Event_Group               Create an event group
*       NU_Set_Events                       Set events in a group
*       NU_Retrieve_Events                  Retrieve events from a group
*       EVC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       common_services.h                   Common service constants
*       thread_control.h                    Thread Control functions
*       event_group.h                       Event group functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/core/inc/event_group.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define external inner-component global data references.  */

extern CS_NODE         *EVD_Created_Event_Groups_List;
extern UNSIGNED         EVD_Total_Event_Groups;

/* Define internal component function prototypes.  */

VOID    EVC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Event_Group
*
*   DESCRIPTION
*
*       This function creates an event group and then places it on the
*       list of created event groups.
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
*
*   INPUTS
*
*       event_group_ptr                     Event Group control
*                                           block ptr
*       name                                Event Group name
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_GROUP                    Event group control block
*                                           pointer is NULL
*
***********************************************************************/
STATUS NU_Create_Event_Group(NU_EVENT_GROUP *event_group_ptr, CHAR *name)
{
    R1 EV_GCB       *event_group;           /* Event control block ptr   */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input event group pointer into internal pointer.  */
    event_group =  (EV_GCB *) event_group_ptr;

    /* Check for a NULL event group pointer or an already created event
       group.  */
    NU_ERROR_CHECK(((event_group == NU_NULL) || (event_group -> ev_id == EV_EVENT_ID)), status, NU_INVALID_GROUP);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(event_group, EV_GCB);

        /* Fill in the event group name. */
        strncpy(event_group -> ev_name, name, (NU_MAX_NAME - 1));

        /* Protect against access to the list of created event_groups.  */
        TCCT_Schedule_Lock();

        /* At this point the event_group is completely built.  The ID can now be
           set and it can be linked into the created event_group list.  */
        event_group -> ev_id =  EV_EVENT_ID;

        /* Link the event group into the list of created event groups and
           increment the total number of event groups in the system.  */
        NU_Place_On_List(&EVD_Created_Event_Groups_List,
                         &(event_group -> ev_created));
        EVD_Total_Event_Groups++;

        /* Trace log */
        T_EVT_GRP_CREATE((VOID*)event_group, name, OBJ_ACTION_SUCCESS);

        /* Release protection against access to the list of created event
           groups.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_EVT_GRP_CREATE((VOID*)event_group, name, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Set_Events
*
*   DESCRIPTION
*
*       This function sets event flags within the specified event flag
*       group.  Event flags may be ANDed or ORed against the current
*       events of the group.  Any task that is suspended on the group
*       that has its request satisfied is resumed.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove from suspend list
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect event group
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       event_group_ptr                     Event Group control
*                                           block ptr
*       events                              Event flag setting
*       operation                           Operation to perform on the
*                                           event flag group (AND/OR)
*
*   OUTPUTS
*
*       NU_SUCCESS                          If service is successful
*       NU_INVALID_GROUP                    Event group control block
*                                           pointer is invalid
*       NU_INVALID_OPERATION                Event operation is invalid
*
***********************************************************************/
STATUS NU_Set_Events(NU_EVENT_GROUP *event_group_ptr, UNSIGNED events,
                     OPTION operation)
{
    R1 EV_GCB       *event_group;           /* Event control block ptr   */
    R2 EV_SUSPEND   *suspend_ptr;           /* Pointer to suspension block */
    R3 EV_SUSPEND   *next_ptr;              /* Pointer to next suspend   */
    R4 EV_SUSPEND   *last_ptr;              /* Last suspension block ptr */
    UNSIGNED        consume;                /* Event flags to consume    */
    UNSIGNED        compare;                /* Event comparison variable */
    INT             preempt;                /* Preemption required flag  */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input event group pointer into internal pointer.  */
    event_group =  (EV_GCB *) event_group_ptr;

    /* Determine if event group pointer is valid. */
    NU_ERROR_CHECK((event_group == NU_NULL), status, NU_INVALID_GROUP);

    /* Determine if event group pointer is valid. */
    NU_ERROR_CHECK((event_group -> ev_id != EV_EVENT_ID), status, NU_INVALID_GROUP);

    /* Determine if valid operation on the event flag group. */
    NU_ERROR_CHECK(((operation != NU_AND) && (operation != NU_OR)), status, NU_INVALID_OPERATION);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the event group.  */
        TCCT_Schedule_Lock();

        /* Perform the specified operation on the current event flags in the
           group.  */
        if (operation & EV_AND)
        {
            /* AND the specified events with the current events.  */
            event_group -> ev_current_events =
                            event_group -> ev_current_events & events;
        }
        else
        {
            /* OR the specified events with the current events.  */
            event_group -> ev_current_events =
                            event_group -> ev_current_events | events;
        }

        /* Determine if there are any tasks suspended for events from this
           event flag group.  */
        if (event_group -> ev_suspension_list)
        {
            /* Initialize the consumption bits to 0.  */
            consume =  0;

            /* Now, walk the chain of tasks suspended on this event flag group to
               determine if any of their requests can be satisfied.  */
            suspend_ptr =  event_group -> ev_suspension_list;

            /* Setup a pointer to the last suspension block.  */
            last_ptr =  (EV_SUSPEND *) suspend_ptr -> ev_suspend_link.cs_previous;

            /* Clear the preempt flag.  */
            preempt =  0;
            do
            {
                /* Determine if this request has been satisfied.  */

                /* First, find the event flags in common.  */
                compare =  event_group -> ev_current_events &
                                            suspend_ptr -> ev_requested_events;

                /* Second, determine if all the event flags must match.  */
                if (suspend_ptr -> ev_operation & EV_AND)
                {
                    /* Yes, an AND condition is present.  All requested events
                       must be present.  */
                    compare =  (compare == suspend_ptr -> ev_requested_events);
                }

                /* Setup the next pointer.  Note that this must be done before
                   the suspended task is resumed, since its suspend block could
                   get corrupted.  */
                next_ptr =  (EV_SUSPEND *) suspend_ptr -> ev_suspend_link.cs_next;

                /* If compare is non-zero, the suspended task's event request is
                   satisfied.  */
                if (compare)
                {

                    /* Decrement the number of tasks waiting counter.  */
                    event_group -> ev_tasks_waiting--;

                    /* Determine if consumption is requested.  */
                    if (suspend_ptr -> ev_operation & EV_CONSUME)
                    {
                        /* Keep track of the event flags to consume.  */
                        consume =  consume | suspend_ptr -> ev_requested_events;
                    }

                    /* Remove the first suspended block from the list.  */
                    NU_Remove_From_List((CS_NODE **)
                                        &(event_group -> ev_suspension_list),
                                        &(suspend_ptr -> ev_suspend_link));

                    /* Setup the appropriate return value.  */
                    suspend_ptr -> ev_return_status =  NU_SUCCESS;
                    suspend_ptr -> ev_actual_events =
                                            event_group -> ev_current_events;

                    /* Trace log */
                    T_EVT_SET((VOID*)event_group, events, operation, OBJ_UNBLKD_CTXT);

                    /* Resume the suspended task.  */
                    preempt = preempt |
                     TCC_Resume_Task((NU_TASK *) suspend_ptr -> ev_suspended_task,
                                                           NU_EVENT_SUSPEND);

                }

                /* Determine if there is another suspension block to examine.  */
                if (suspend_ptr != last_ptr)
                {
                    /* More to examine in the suspension list.  Look at the
                       next suspend block.  */
                    suspend_ptr =  next_ptr;
                }
                else
                {
                    /* End of the list has been reached.  Set the suspend pointer
                       to NULL to end the search.  */
                    suspend_ptr =  NU_NULL;
                }

            } while (suspend_ptr);

            /* Apply all of the gathered consumption bits.  */
            event_group -> ev_current_events =
               event_group -> ev_current_events & ~consume;

            /* Trace log */
            T_EVT_SET((VOID*)event_group, events, operation, OBJ_ACTION_SUCCESS);

            /* Determine if a preempt condition is present.  */
            if (preempt)
            {
                /* Trace log */
                T_TASK_READY((VOID*)TCCT_Current_Thread());

                /* Transfer control to the system if the resumed task function
                   detects a preemption condition.  */
                TCCT_Control_To_System();
            }
        }
        else
        {
            /* Trace log */
            T_EVT_SET((VOID*)event_group, events, operation, OBJ_ACTION_SUCCESS);
        }

        /* Release protection of the event_group.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_EVT_SET((VOID*)event_group, events, operation, status);
    }

    /* Return a successful status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Retrieve_Events
*
*   DESCRIPTION
*
*       This function retrieves various combinations of event flags from
*       the specified event group.  If the group does not contain the
*       necessary flags, suspension of the calling task is possible.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       TCC_Suspend_Task                    Suspend calling task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Current_Thread                 Pickup current thread pointer
*       TCCT_Schedule_Lock                  Protect event group
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       event_group_ptr                     Event Group control
*                                           block ptr
*       requested_events                    Requested event flags
*       operation                           AND/OR selection of flags
*       retrieved_events                    Pointer to destination for
*                                           actual flags retrieved
*       suspend                             Suspension option
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If successful completion
*           NU_TIMEOUT                      If timeout on suspension
*           NU_NOT_PRESENT                  If event flags are not
*                                           present
*           NU_INVALID_GROUP                Event group control block
*                                           pointer is invalid
*           NU_INVALID_POINTER              Received event flag pointer
*                                           is NULL
*           NU_INVALID_OPERATION            Event operation is invalid
*           NU_INVALID_SUSPEND              Invalid suspension request
*
***********************************************************************/
STATUS NU_Retrieve_Events(NU_EVENT_GROUP *event_group_ptr,
                          UNSIGNED requested_events, OPTION operation,
                          UNSIGNED *retrieved_events, UNSIGNED suspend)
{
    R1 EV_GCB       *event_group;           /* Event control block ptr   */
    R2 EV_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    EV_SUSPEND      suspend_block;          /* Suspension block          */
    R3 UNSIGNED     compare;                /* Event comparison variable */
    TC_TCB          *task;                  /* Pointer to task           */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input event group pointer into internal pointer.  */
    event_group =  (EV_GCB *) event_group_ptr;

    /* Determine if event group pointer is valid. */
    NU_ERROR_CHECK((event_group == NU_NULL), status, NU_INVALID_GROUP);

    /* Determine if event group pointer is valid. */
    NU_ERROR_CHECK((event_group -> ev_id != EV_EVENT_ID), status, NU_INVALID_GROUP);

    /* Verify valid operation on the event flag group. */
    NU_ERROR_CHECK(((operation != NU_AND) && (operation != NU_AND_CONSUME) && (operation != NU_OR) && (operation != NU_OR_CONSUME)), status, NU_INVALID_OPERATION);

    /* Determine if suspension from an non-task thread. */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    /* Determine if retrieved events pointer is NULL. */
    NU_ERROR_CHECK((retrieved_events == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the event group.  */
        TCCT_Schedule_Lock();

        /* Determine if the events requested are present.  */

        /* Isolate common event flags.  */
        compare =  event_group -> ev_current_events & requested_events;

        /* Determine if all of the events must be present.  */
        if (operation & EV_AND)
        {
            /* Yes, all events must be present.  See if the compare value is
               the same as the requested value.  */
            compare =  (compare == requested_events);
        }

        /* Determine if the requested combination of event flags are present.  */
        if (compare)
        {
            /* Yes, necessary event flags are present.  */

            /* Copy the current event flags into the appropriate destination.  */
            *retrieved_events =  event_group -> ev_current_events;

            /* Determine if consumption is required.  If so, consume the event
               flags present in the group.  */
            if (operation & EV_CONSUME)
            {
                event_group -> ev_current_events =
                    event_group -> ev_current_events & ~requested_events;
            }
            /* Trace log */
            T_EVT_RETRIEVE((VOID*)event_group, requested_events,
                           *retrieved_events, suspend, operation, OBJ_ACTION_SUCCESS);
        }
        else
        {

            /* Determine if the task requested suspension.  */
            if (suspend)
            {
                /* Suspension is selected.  */

                /* Increment the number of tasks waiting.  */
                event_group -> ev_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
                suspend_ptr -> ev_event_group =              event_group;
                suspend_ptr -> ev_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> ev_suspend_link.cs_previous = NU_NULL;
                task =                            (TC_TCB *) TCCT_Current_Thread();
                suspend_ptr -> ev_suspended_task =           task;
                suspend_ptr -> ev_requested_events =         requested_events;
                suspend_ptr -> ev_operation =                operation;

                /* Link the suspend block into the list of suspended tasks on this
                   event group.  */
                NU_Place_On_List((CS_NODE **)
                                 &(event_group -> ev_suspension_list),
                                 &(suspend_ptr -> ev_suspend_link));

                /* Trace log */
                T_EVT_RETRIEVE((VOID*)event_group, requested_events,
                               suspend_ptr -> ev_actual_events, suspend, operation, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the event group.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_EVENT_SUSPEND,
                                            EVC_Cleanup, suspend_ptr, suspend);

                /* Pickup the return status and the actual retrieved events.  */
                status =             suspend_ptr -> ev_return_status;
                *retrieved_events =  suspend_ptr -> ev_actual_events;

            }
            else
            {
                /* No suspension requested.  Simply return an error status
                   and zero the retrieved events variable.  */
                status =             NU_NOT_PRESENT;
                *retrieved_events =  0;

                /* Trace log */
                 T_EVT_RETRIEVE((VOID*)event_group, requested_events,
                                *retrieved_events, suspend, operation, status);
            }

        }

        /* Release protection of the event_group.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_EVT_RETRIEVE((VOID*)event_group, requested_events,
                       *retrieved_events, suspend, operation, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       EVC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a event group.  It is not called unless a timeout or a task
*       terminate is in progress.  Note that protection is already in
*       effect - the same protection at suspension time.  This routine
*       must be called from Supervisor mode in Supervisor/User mode
*       switching kernels.
*
*   CALLED BY
*
*       TCC_Task_Timeout                    Task timeout
*       NU_Terminate_Task                   Task terminate
*
*   CALLS
*
*       NU_Remove_From_List                 Remove suspend block from
*                                           the suspension list
*
*   INPUTS
*
*       information                         Pointer to suspend block
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  EVC_Cleanup(VOID *information)
{
    EV_SUSPEND      *suspend_ptr;           /* Suspension block pointer  */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (EV_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> ev_return_status =  NU_TIMEOUT;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> ev_event_group) -> ev_tasks_waiting--;

    /* Unlink the suspend block from the suspension list.  */
    NU_Remove_From_List((CS_NODE **)
                        &((suspend_ptr -> ev_event_group) -> ev_suspension_list),
                        &(suspend_ptr -> ev_suspend_link));

    /* Return to user mode */
    NU_USER_MODE();
}

