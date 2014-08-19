/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_evt.c
*
* COMPONENT
*
*       IKE - Events
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE Events
*       which are based on the Nucleus NET timers.
*
* DATA STRUCTURES
*
*       IKE_Event_List              List of IKE events.
*       IKE_Event_Freelist          List of free event nodes.
*       IKE_Message_Reply_Event     Event ID of message reply event.
*       IKE_SA_Timeout_Event        Event ID of SA timeout event.
*       IKE_Phase1_Timeout_Event    Event ID of Phase 1 timeout event.
*       IKE_Phase2_Timeout_Event    Event ID of Phase 2 timeout event.
*
* FUNCTIONS
*
*       IKE_Initialize_Events
*       IKE_EQ_Handler
*       IKE_Event_Handler
*       IKE_IPsec_Request_Handler
*       IKE_Message_Reply_Handler
*       IKE_SA_Timeout_Handler
*       IKE_Phase1_Timeout_Handler
*       IKE_Phase2_Timeout_Handler
*       IKE_Unset_Single_Event
*       IKE_Unset_Matching_Events
*       IKE_Set_Timer
*       IKE_Unset_Timer
*       IKE_Unset_Matching_Timers
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_api.h
*       ike_pkt.h
*       ike_evt.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_pkt.h"
#include "networking/ike_evt.h"

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
#include "networking/ike2_evt.h"
#endif

/* LIST OF IKE EVENTS
 *
 * Listed below are the events recognized by IKE. The two
 * arbitrary TQ event arguments are also specified in this
 * table. Note that the event names in UPPER CASE are those
 * which are internal to IKE. They are not registered with
 * the NET TQ component.
 *
 * Event                       dat             ext_dat
 * -----------------------------------------------------------
 * IKE_Message_Reply_Event     SA pointer      Unused (0)
 * IKE_Message_Reply_Event     SA pointer      Phase 2 pointer
 * IKE_SA_Timeout_Event        SA pointer      SADB pointer
 * IKE_Phase1_Timeout_Event    SA pointer      SADB pointer
 * IKE_Phase2_Timeout_Event    SA pointer      Phase 2 pointer
 * IKE_PHASE1_ERROR_EVENT      SA pointer      SADB pointer
 * IKE_PHASE2_ERROR_EVENT      SA pointer      Phase 2 pointer
 * IKE_SA_REMOVE_EVENT         SA pointer      SADB pointer
 * IKE_INITIATE_REQ_EVENT      Unused (0)      Request Pointer
 * IKE_DELETE_NOTIFY_EVENT     Unused (0)      Request Pointer
 */

/* NOTES:
 *
 * This component has a complex synchronization mechanism,
 * needed because a single SA may have multiple timers running
 * on it and a timer event may delete an SA/Handle thus
 * requiring other pending events to be synchronized. At any
 * given time, a registered IKE event must be present in one of
 * two locations:
 *
 * 1. Nucleus NET timer event list.
 * 2. IKE event list (IKE_Event_List).
 *
 * Therefore, if an SA is being removed, both lists should be
 * updated to make sure that the SA will not be accessed by
 * any subsequent IKE event.
 *
 * The TQ_Timerunset function allows removing selected events
 * from the Nucleus NET timer event list. Similarly, the
 * IKE_Unset_Matching_Events and IKE_Unset_Single_Event
 * functions allow removing selected events from the IKE event
 * list.
 *
 * Protection for the IKE event list is provided by the NET
 * SLL component. If this list is directly accessed from
 * anywhere else, interrupts must be disabled, as is done in
 * the SLL component.
 */

/* Local function prototypes. */
STATIC VOID IKE_IPsec_Request_Handler(TQ_EVENT event,
                                      IKE_INITIATE_REQ *initiate_req);
STATIC VOID IKE_Message_Reply_Handler(IKE_SA *sa,
                                      IKE_PHASE2_HANDLE *phase2);
STATIC VOID IKE_SA_Timeout_Handler(TQ_EVENT event, IKE_SA *sa,
                                   IKE_SADB *sadb);
STATIC VOID IKE_Phase1_Timeout_Handler(TQ_EVENT event, IKE_SA *sa,
                                       IKE_SADB *sadb);
STATIC VOID IKE_Phase2_Timeout_Handler(TQ_EVENT event, IKE_SA *sa,
                                       IKE_PHASE2_HANDLE *phase2);

/* Lists of allocated and free IKE events. */
IKE_EVENT_DB IKE_Event_List;
IKE_EVENT_DB IKE_Event_Freelist;

/* Event ID of each type of event expected by IKE. */
TQ_EVENT IKE_Message_Reply_Event;
TQ_EVENT IKE_SA_Timeout_Event;
TQ_EVENT IKE_Phase1_Timeout_Event;
TQ_EVENT IKE_Phase2_Timeout_Event;

/*************************************************************************
*
* FUNCTION
*
*       IKE_Initialize_Events
*
* DESCRIPTION
*
*       This function initializes the IKE Events component.
*       It registers IKE events with the Nucleus NET EQ
*       component and creates the event handler task.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_INVALID_SIZE         Task stack not large enough.
*       NU_INVALID_PREEMPT      Task preempt parameter is invalid.
*       NU_INVALID_START        Task auto-start parameter is invalid.
*       NU_NO_MEMORY            Not enough memory available.
*
*************************************************************************/
STATUS IKE_Initialize_Events(VOID)
{
    STATUS          status;
    UINT8           *memory;

    IKE_DEBUG_LOG("Initializing IKE events");

    /* Initialize both event lists. */
    IKE_Event_List.ike_flink     = NU_NULL;
    IKE_Event_List.ike_last      = NU_NULL;
    IKE_Event_Freelist.ike_flink = NU_NULL;
    IKE_Event_Freelist.ike_last  = NU_NULL;

    /* Register the IKE message reply timeout event. */
    status = EQ_Register_Event(IKE_EQ_Handler, &IKE_Message_Reply_Event);

    if(status == NU_SUCCESS)
    {
        /* Register the IKE SA timeout event. */
        status = EQ_Register_Event(IKE_EQ_Handler, &IKE_SA_Timeout_Event);

        if(status == NU_SUCCESS)
        {
            /* Register the Phase 1 timeout event. */
            status = EQ_Register_Event(IKE_EQ_Handler,
                                       &IKE_Phase1_Timeout_Event);

            if(status == NU_SUCCESS)
            {
                /* Register the Phase 2 timeout event. */
                status = EQ_Register_Event(IKE_EQ_Handler,
                                           &IKE_Phase2_Timeout_Event);

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
                if(status == NU_SUCCESS)
                {
                    /* Register the IKEv2 cookie secret timeout event. */
                    status = EQ_Register_Event(IKE_EQ_Handler,
                                &IKE2_Cookie_Secret_Timeout_Event);
                    if(status == NU_SUCCESS)
                    {
                        /* Register the IKEv2 message reply timeout
                         * event.
                         */
                        status = EQ_Register_Event(IKE_EQ_Handler,
                                    &IKE2_Message_Reply_Event);
                        if(status == NU_SUCCESS)
                        {
                            /* Register the IKEv2 SA timeout event. */
                            status = EQ_Register_Event(IKE_EQ_Handler,
                                        &IKE2_SA_Timeout_Event);
                            if(status == NU_SUCCESS)
                            {
                                /* Register the IKEv2 AUTH exchange
                                 * timeout event.
                                 */
                                status = EQ_Register_Event(IKE_EQ_Handler,
                                            &IKE2_INIT_AUTH_Timeout);
                            }

                        }

                    }

                }

#endif

                if(status == NU_SUCCESS)
                {
                    /* Allocate memory for event task stack. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                                (VOID**)&memory,
                                                IKE_EVENT_TASK_STACK_SIZE,
                                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Normalize the pointer. */
                        memory = TLS_Normalize_Ptr(memory);

                        /* Create the IKE event handler task. */
                        status = NU_Create_Task(
                                     &IKE_Data.ike_event_task_cb,
                                     "NU_IKE", IKE_Event_Handler, 0,
                                     NU_NULL, memory,
                                     IKE_EVENT_TASK_STACK_SIZE,
                                     IKE_EVENT_TASK_PRIORITY,
                                     IKE_EVENT_TASK_TIME_SLICE,
                                     IKE_EVENT_TASK_PREEMPT, NU_NO_START);

                        if(status == NU_SUCCESS)
                        {
                            /* Store memory pointer in IKE global data. */
                            IKE_Data.ike_event_task_stack = memory;

                            /* Start the IKE task. */
                            status = NU_Resume_Task(
                                         &IKE_Data.ike_event_task_cb);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to start IKE event task",
                                    NERR_SEVERE, __FILE__, __LINE__);
                            }

                            /* Log debug message. */
                            IKE_DEBUG_LOG("Created IKE event task");
                        }

                        else
                        {
                            /* Deallocate the memory allocated above. */
                            if(NU_Deallocate_Memory(memory) != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to deallocate IKE memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                            }
                        }
                    }

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
                    /* If any of the above functions failed. */
                    if(status != NU_SUCCESS)
                    {
                        /* Un-register the IKEv2 cookie secret timeout event. */
                        status = EQ_Register_Event(IKE_EQ_Handler,
                            &IKE2_Cookie_Secret_Timeout_Event);
                    }
#endif

                    /* If any of the above functions failed. */
                    if(status != NU_SUCCESS)
                    {
                        /* Un-register the Phase 2 timeout event. */
                        if(EQ_Unregister_Event(IKE_Phase2_Timeout_Event)
                           != NU_SUCCESS)
                        {
                            NLOG_Error_Log(
                                "Failed to un-register IKE event from EQ",
                                NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }
                }

                /* If any of the above functions failed. */
                if(status != NU_SUCCESS)
                {
                    /* Un-register the Phase 1 timeout event. */
                    if(EQ_Unregister_Event(IKE_Phase1_Timeout_Event)
                       != NU_SUCCESS)
                    {
                        NLOG_Error_Log(
                            "Failed to un-register IKE event from EQ",
                            NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }

            /* If any of the above functions failed. */
            if(status != NU_SUCCESS)
            {
                /* Un-register the IKE SA timeout event. */
                if(EQ_Unregister_Event(IKE_SA_Timeout_Event) != NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Failed to un-register IKE event from EQ",
                        NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        /* If any of the above functions failed. */
        if(status != NU_SUCCESS)
        {
            /* Un-register the IKE message reply timeout event. */
            if(EQ_Unregister_Event(IKE_Message_Reply_Event) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to un-register IKE event from EQ",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Initialize_Events */

/*************************************************************************
*
* FUNCTION
*
*       IKE_EQ_Handler
*
* DESCRIPTION
*
*       This function is the EQ event handler. It is a callback
*       called by Nucleus NET when an event is triggered. The
*       TCP_Resource semaphore is obtained when this is called
*       so it must perform simple tasks only.
*
* INPUTS
*
*       event                   Event which has triggered.
*       dat                     Optional data.
*       ext_dat                 Extra optional data.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_EQ_Handler(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat)
{
    STATUS          status = NU_SUCCESS;
    IKE_EVENT       *new_event;

    /* Get a free event node from the event free list. */
    new_event = SLL_Dequeue(&IKE_Event_Freelist);

    /* If no free node was found. */
    if(new_event == NU_NULL)
    {
        /* Allocate memory for an event. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&new_event,
                                    sizeof(IKE_EVENT), NU_NO_SUSPEND);
    }

    if(status == NU_SUCCESS)
    {
        /* Fill in the new event. */
        new_event->ike_id      = event;
        new_event->ike_dat     = dat;
        new_event->ike_ext_dat = ext_dat;

        /* Add event to the event list. */
        SLL_Enqueue(&IKE_Event_List, new_event);

        /* Indicate to the IKE event handler that a
         * new event is ready for processing.
         */
        if(NU_Set_Events(&IKE_Data.ike_event_group,
                         IKE_NEW_EVENT_FLAG, NU_OR) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to set event in IKE event group",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate event item",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

} /* IKE_EQ_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Event_Handler
*
* DESCRIPTION
*
*       This function is the entry point of the IKE event handler
*       task. This task is responsible for processing the events
*       posted to the IKE event list.
*
* INPUTS
*
*       argc                    Argument count.
*       *argv                   Argument vector containing pointers
*                               to command line arguments.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Event_Handler(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    UNSIGNED            ret_events;
    IKE_EVENT           *event;
    IKE_SA              *sa;
    VOID                *ptr;
    NU_SUPERV_USER_VARIABLES

    /* To avoid compiler warnings. */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Loop for as long as stopping state is not reached. */
    while(IKE_Daemon_State != IKE_DAEMON_STOPPING_EVENTS)
    {
        /* Wait for an event. */
        status = NU_Retrieve_Events(&IKE_Data.ike_event_group,
                                    IKE_NEW_EVENT_FLAG, NU_OR_CONSUME,
                                    &ret_events, IKE_DAEMON_POLL_INTERVAL);

        if(status != NU_SUCCESS)
        {
            /* If this is only a timeout, start over again. */
            if(status == NU_TIMEOUT)
            {
                continue;
            }

            else
            {
                /* Log failure of event retrieval. */
                NLOG_Error_Log("Unable to retrieve IKE event",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Critical error so abort this task. */
                break;
            }
        }

        /* Get first event from event list. */
        event = (IKE_EVENT*)SLL_Dequeue(&IKE_Event_List);

        /* Loop until all pending events are processed. */
        while(event != NU_NULL)
        {
            /* If this is a request from IPsec. */
            if((event->ike_id == IKE_INITIATE_REQ_EVENT)
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
               || (event->ike_id == IKE_DELETE_NOTIFY_EVENT)
#endif
               )
            {
                /* Call the event handler. */
                IKE_IPsec_Request_Handler(event->ike_id,
                    (IKE_INITIATE_REQ*)event->ike_ext_dat);
            }

            /* Otherwise grab the IKE semaphore. */
            else if(NU_Obtain_Semaphore(&IKE_Data.ike_semaphore,
                                        IKE_TIMEOUT) != NU_SUCCESS)
            {
                /* Failed to obtain semaphore. */
                NLOG_Error_Log("Failed to obtain IKE semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Get the SA pointer from the event parameters. */
                sa = (IKE_SA*)event->ike_dat;

                /* Get arbitrary pointer from the event parameters. */
                ptr = (VOID*)event->ike_ext_dat;

                /* If a message reply interval has timed-out. */
                if(event->ike_id == IKE_Message_Reply_Event)
                {
                    /* Call the event handler. */
                    IKE_Message_Reply_Handler(sa, (IKE_PHASE2_HANDLE*)ptr);
                }

                /* If an IKE SA has timed-out. */
                else if((event->ike_id == IKE_SA_Timeout_Event) ||
                        (event->ike_id == IKE_SA_REMOVE_EVENT))
                {
                    /* Call the event handler. */
                    IKE_SA_Timeout_Handler(event->ike_id, sa,
                                           (IKE_SADB*)ptr);
                }

                /* If a Phase 1 exchange has timed-out. */
                else if((event->ike_id == IKE_Phase1_Timeout_Event) ||
                        (event->ike_id == IKE_PHASE1_ERROR_EVENT))
                {
                    /* Call the event handler. */
                    IKE_Phase1_Timeout_Handler(event->ike_id, sa,
                                               (IKE_SADB*)ptr);
                }

                /* If a Phase 2 exchange has timed-out. */
                else if((event->ike_id == IKE_Phase2_Timeout_Event) ||
                        (event->ike_id == IKE_PHASE2_ERROR_EVENT))
                {
                    /* Call the event handler. */
                    IKE_Phase2_Timeout_Handler(event->ike_id, sa,
                                               (IKE_PHASE2_HANDLE*)ptr);
                }

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                /* Following are IKEv2 related events. */

                /* Response to IKEv2 message timed out. */
                else if(event->ike_id == IKE2_Message_Reply_Event)
                {
                    /* Call the event handler. */
                    IKE2_Message_Reply_Handler((IKE2_SA *)sa,
                        (IKE2_EXCHANGE_HANDLE *)ptr);
                }

                /* IKEv2 SA timed out. */
                else if(event->ike_id == IKE2_SA_Timeout_Event)
                {
                    /* Call the event handler. */
                    IKE2_SA_Timeout_Handler(event->ike_id,
                                            sa, (IKE2_SADB *)ptr);
                }

                /* If an IKEv2 Cookie secret has timed out. */
                else if(event->ike_id == IKE2_Cookie_Secret_Timeout_Event)
                {
                    /* Call the event handler. */
                    IKE2_Cookie_Secret_Timeout_Handler(event->ike_id);
                }

                /* Exchange timed out or an error occurred. */
                else if((event->ike_id == IKE2_INIT_AUTH_Timeout) ||
                        (event->ike_id == IKE2_SA_ERROR_EVENT))
                {
                    /* Call the event handler. */
                    IKE2_Exchange_Timeout_Event(event->ike_id,
                        (IKE2_SA *)sa, (IKE2_EXCHANGE_HANDLE *)ptr);
                }

                /* SA needs to be removed. */
                else if(event->ike_id == IKE2_SA_REMOVE_EVENT)
                {
                    /* Call the event handler. */
                    IKE2_SA_Remove_Event(event->ike_id,
                        (IKE2_EXCHANGE_HANDLE *)sa, (IKE2_SADB *)ptr);
                }

#endif

                /* Release the IKE semaphore. */
                if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) !=
                    NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release the semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Return current event to the free list. */
            SLL_Enqueue(&IKE_Event_Freelist, event);

            /* Get next event from event list. */
            event = (IKE_EVENT*)SLL_Dequeue(&IKE_Event_List);
        }
    }

    /* If shutdown of this task was requested. */
    if(IKE_Daemon_State == IKE_DAEMON_STOPPING_EVENTS)
    {
        /* Request completed. Now move to next stop state. */
        IKE_Daemon_State = IKE_DAEMON_STOPPING_MISC;
    }

    /* Log debug message. */
    IKE_DEBUG_LOG("IKE events task terminated");

    /* Switch back to user mode. */
    NU_USER_MODE();

} /* IKE_Event_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPsec_Request_Handler
*
* DESCRIPTION
*
*       This function handles all requests from IPsec.
*       A request could be for initiating a new exchange
*       or for sending a delete notification for an IPsec
*       SA. All requests must be non-blocking. This function
*       de-allocates the request structure after use because
*       it is dynamically allocated by IPsec.
*
* INPUTS
*
*       event                   Event identifier.
*       *initiate_req           Pointer to a dynamically allocated
*                               initiate request structure.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_IPsec_Request_Handler(TQ_EVENT event,
                                      IKE_INITIATE_REQ *initiate_req)
{
#if (IKE_INCLUDE_INFO_MODE == NU_FALSE)
    UNUSED_PARAMETER(event);
#endif

    /* Make sure the pointer is valid. */
    if(initiate_req != NU_NULL)
    {
        /* Make sure this is a non-blocking request. */
        if(initiate_req->ike_suspend == NU_NO_SUSPEND)
        {
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
            /* If this is an Initiate request. */
            if(event == IKE_INITIATE_REQ_EVENT)
#endif
            {
                /* Log the event. */
                IKE_DEBUG_LOG("Initiating exchange request by IPsec");

                /* Initiate the IKE exchange. */
                if(IKE_Initiate(initiate_req) != NU_SUCCESS)
                {
                    NLOG_Error_Log("IKE initiate request by IPsec failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
            /* Otherwise it must be a delete notification request. */
            else
            {
                IKE_DEBUG_LOG("Sending IPsec SA deletion notification");

                /* Try to send the delete notification. */
                if(IKE_Send_Delete_Notification(initiate_req) ==
                   NU_SUCCESS)
                {
                    IKE_DEBUG_LOG("Sent error notification");
                }
            }
#endif

            /* The request structure is dynamically allocated by
             * by IPsec and IKE is responsible for deallocating it.
             */
            if(NU_Deallocate_Memory(initiate_req) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to free IKE request structure",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

} /* IKE_IPsec_Request_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Message_Reply_Handler
*
* DESCRIPTION
*
*       This function handles the IKE_Message_Reply_Event event.
*       It re-sends the Phase 1 or Phase 2 message which was last
*       transmitted.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       *phase2                 Pointer to the Phase 2 Handle. If
*                               this parameter is NULL, then a
*                               Phase 1 event is assumed.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_Message_Reply_Handler(IKE_SA *sa,
                                      IKE_PHASE2_HANDLE *phase2)
{
    UINT8               resend_count;

    /* Log the event. */
    IKE_DEBUG_LOG("Exchange message reply timed-out");

    /* Initialize re-send count to zero. */
    resend_count = 0;

    /* If the phase 2 handle pointer is valid. */
    if(phase2 != NU_NULL)
    {
        /* Re-send the phase 2 message. */
        if(IKE_Resend_Packet((IKE_PHASE1_HANDLE*)phase2) != NU_SUCCESS)
        {
            /* Failed to re-send message. */
            NLOG_Error_Log("Failed to re-send IKE message on timeout",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Get re-send count from phase 2 handle. */
        resend_count = phase2->ike_resend_count;
    }

    /* If a Phase 1 Handle is present and the SA
     * has not yet been established.
     */
    else if(sa->ike_state != IKE_SA_ESTABLISHED)
    {
        /* Re-send the phase 1 message. */
        if(IKE_Resend_Packet(sa->ike_phase1) != NU_SUCCESS)
        {
            /* Failed to re-send message. */
            NLOG_Error_Log("Failed to re-send IKE message on timeout",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Get re-send count from SA. */
        resend_count = sa->ike_phase1->ike_resend_count;
    }

    /* If re-send count has not expired. */
    if(resend_count != 0)
    {
        /* Set the message reply timer again with
         * exponential back-off.
         */
        if(IKE_Set_Timer(IKE_Message_Reply_Event,
                        (UNSIGNED)sa, (UNSIGNED)phase2,
                        (UNSIGNED)(IKE_RESEND_INTERVAL *
                                  (IKE_RESEND_COUNT - resend_count + 1)))
                                  != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to set message resend timer",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

} /* IKE_Message_Reply_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_SA_Timeout_Handler
*
* DESCRIPTION
*
*       This function handles the IKE_SA_Timeout_Event and
*       IKE_SA_REMOVE_EVENT events. The difference between
*       the two is that the delete SA notification is only
*       sent for the first event.
*
* INPUTS
*
*       event                   Event identifier.
*       *sa                     Pointer to the IKE SA.
*       *sadb                   Pointer to the SADB.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_SA_Timeout_Handler(TQ_EVENT event, IKE_SA *sa,
                                   IKE_SADB *sadb)
{
#if (IKE_INCLUDE_INFO_MODE == NU_FALSE)
    UNUSED_PARAMETER(event);
#endif

    /* Log the event. */
    IKE_DEBUG_LOG("IKE SA timed-out");

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
    /* Resume waiting processes with an error status. */
    if(IKE_Resume_Waiting_Processes(sa, IKE_SA_TIMED_OUT) == NU_SUCCESS)
    {
        IKE_DEBUG_LOG("Waiting processes resumed");
    }
#endif

#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
    if((event         == IKE_SA_Timeout_Event) &&
       (sa->ike_state == IKE_SA_ESTABLISHED))
    {
        /* Try to send delete notification to remote node. */
        if(IKE_Delete_Notification(sa, IKE_PROTO_ISAKMP, 0) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to send delete notification for IKE SA",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }
#endif

    /* Remove the IKE SA. */
    if(IKE_Remove_SA(sadb, sa->ike_cookies, IKE_MATCH_SA_PARTIAL_COOKIE)
       != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to remove SA from SADB",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* Grab the NET semaphore. */
        if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain the semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Remove conflicting events containing the deleted SA
             * from the NET TQ timer list.
             */
            TQ_Timerunset(IKE_Message_Reply_Event,
                          TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);
            TQ_Timerunset(IKE_SA_Timeout_Event,
                          TQ_CLEAR_EXACT, (UNSIGNED)sa, (UNSIGNED)sadb);
            TQ_Timerunset(IKE_Phase1_Timeout_Event,
                          TQ_CLEAR_EXACT, (UNSIGNED)sa, (UNSIGNED)sadb);
            TQ_Timerunset(IKE_Phase2_Timeout_Event,
                          TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);

            /* Also remove conflicting events from the IKE event list. */
            IKE_Unset_Matching_Events((UNSIGNED)sa, 0, TQ_CLEAR_ALL_EXTRA);

            /* Release the NET semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

} /* IKE_SA_Timeout_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Phase1_Timeout_Handler
*
* DESCRIPTION
*
*       This function handles the IKE_Phase1_Timeout_Event and
*       IKE_PHASE1_ERROR_EVENT events. It cleans up all IKE
*       Exchange related data after an exchange error or timeout.
*
* INPUTS
*
*       event                   Event identifier.
*       *sa                     Pointer to an IKE SA.
*       *sadb                   Pointer to the SADB.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_Phase1_Timeout_Handler(TQ_EVENT event, IKE_SA *sa,
                                       IKE_SADB *sadb)
{
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_FALSE)
    UNUSED_PARAMETER(event);
#endif

    /* Make sure the Phase 1 Handle is present. */
    if(sa->ike_phase1 != NU_NULL)
    {
        /* Make sure SA has not been established. */
        if(sa->ike_state != IKE_SA_ESTABLISHED)
        {
            /* Log the event. */
            IKE_DEBUG_LOG("Phase 1 Exchange timeout/error.");

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
            /* If this is a phase 1 timeout. */
            if(event == IKE_Phase1_Timeout_Event)
            {
                /* Resume waiting processes with an error status. */
                if(IKE_Resume_Waiting_Processes(sa, IKE_PHASE1_TIMED_OUT)
                   == NU_SUCCESS)
                {
                    IKE_DEBUG_LOG("Waiting processes resumed");
                }
            }

            /* Otherwise, an error occurred during the exchange
             * and exchange deletion has been requested.
             */
            else
            {
                /* Resume waiting processes. The error status must
                 * be updated by the task which posted this event.
                 */
                if(IKE_Resume_Waiting_Processes(sa, IKE_NO_UPDATE)
                   == NU_SUCCESS)
                {
                    IKE_DEBUG_LOG("Waiting processes resumed");
                }
            }
#endif

            /* Remove the IKE SA. */
            if(IKE_Remove_SA(sadb, sa->ike_cookies,
                             IKE_MATCH_SA_PARTIAL_COOKIE) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to remove SA from SADB",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Grab the NET semaphore. */
                if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) !=
                   NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to obtain the semaphore",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                else
                {
                    /* Remove conflicting events containing the
                     * deleted SA.
                     */
                    TQ_Timerunset(IKE_Message_Reply_Event,
                                  TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);
                    TQ_Timerunset(IKE_SA_Timeout_Event, TQ_CLEAR_EXACT,
                                  (UNSIGNED)sa, (UNSIGNED)sadb);
                    TQ_Timerunset(IKE_Phase1_Timeout_Event, TQ_CLEAR_EXACT,
                                  (UNSIGNED)sa, (UNSIGNED)sadb);
                    TQ_Timerunset(IKE_Phase2_Timeout_Event,
                                  TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);

                    /* Also remove conflicting events from the
                     * IKE event list.
                     */
                    IKE_Unset_Matching_Events((UNSIGNED)sa, 0,
                                              TQ_CLEAR_ALL_EXTRA);

                    /* Release the NET semaphore. */
                    if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        /* Failed to release semaphore. */
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }
        }

#if (IKE_RETAIN_LAST_MESSAGE == NU_TRUE)
        /* If retaining last message of an exchange, then the
         * Phase 1 timeout event would be responsible for
         * deallocating the Phase 1 handle.
         */
        else
        {
            /* Log the event. */
            IKE_DEBUG_LOG("Deleting Phase 1 Handle.");

            /* IKE SA has been established and the exchange has
             * timed-out. In this case the Phase 1 Handle would
             * still be valid because it is maintained for
             * sometime after SA establishment to allow message
             * re-sends in case the last exchange message is lost.
             */
            if(IKE_Remove_Phase1(sa) != NU_SUCCESS)
            {
                /* Unable to remove Handle. */
                NLOG_Error_Log("Failed to remove Phase 1 Handle",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Grab the NET semaphore. */
            if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) !=
               NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain the semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Remove conflicting events containing the
                 * deleted Handle.
                 */
                TQ_Timerunset(IKE_Message_Reply_Event,
                              TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);
                TQ_Timerunset(IKE_Phase1_Timeout_Event, TQ_CLEAR_EXACT,
                              (UNSIGNED)sa, (UNSIGNED)sadb);

                /* Also remove conflicting events from the
                 * IKE event list.
                 */
                IKE_Unset_Single_Event(IKE_Message_Reply_Event,
                                       (UNSIGNED)sa, 0,
                                       TQ_CLEAR_ALL_EXTRA);
                IKE_Unset_Single_Event(IKE_Phase1_Timeout_Event,
                                       (UNSIGNED)sa, (UNSIGNED)sadb,
                                       TQ_CLEAR_EXACT);

                /* Release the NET semaphore. */
                if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    /* Failed to release semaphore. */
                    NLOG_Error_Log("Failed to release semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
#endif /* (IKE_RETAIN_LAST_MESSAGE == NU_TRUE) */
    }

} /* IKE_Phase1_Timeout_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Phase2_Timeout_Handler
*
* DESCRIPTION
*
*       This function handles the IKE_Phase2_Timeout_Event and
*       IKE_PHASE2_ERROR_EVENT events. It cleans up all IKE
*       Exchange related data after an exchange error or timeout.
*
* INPUTS
*
*       event                   Event identifier.
*       *sa                     Pointer to an IKE SA.
*       *phase2                 Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_Phase2_Timeout_Handler(TQ_EVENT event, IKE_SA *sa,
                                       IKE_PHASE2_HANDLE *phase2)
{
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_FALSE)
    UNUSED_PARAMETER(event);
#endif

    /* Log the event. */
    IKE_DEBUG_LOG("Phase 2 exchange being removed.");

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
    /* If this is a phase 2 timeout. */
    if(event == IKE_Phase2_Timeout_Event)
    {
        /* Make sure exchange has not completed. */
        if(phase2->ike_xchg_state != IKE_COMPLETE_STATE)
        {
            /* Resume waiting process. */
            if(IKE_Resume_Waiting_Process(phase2, IKE_PHASE2_TIMED_OUT)
               == NU_SUCCESS)
            {
                IKE_DEBUG_LOG("Waiting process resumed");
            }
        }
    }

    /* Otherwise, an error occurred during the exchange
     * and exchange deletion has been requested.
     */
    else
    {
        /* Resume waiting process. The error status must be
         * updated by the task which posted this event.
         */
        if(IKE_Resume_Waiting_Process(phase2, IKE_NO_UPDATE) == NU_SUCCESS)
        {
            IKE_DEBUG_LOG("Waiting process resumed");
        }
    }
#endif

    /* Remove phase 2 handle from the database. */
    if(IKE_Remove_Phase2(&sa->ike_phase2_db, phase2->ike_msg_id) !=
       NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to remove phase 2 handle from SA",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* Grab the NET semaphore. */
        if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain the semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Remove conflicting events containing the
             * deleted SA.
             */
            TQ_Timerunset(IKE_Message_Reply_Event, TQ_CLEAR_EXACT,
                          (UNSIGNED)sa, (UNSIGNED)phase2);
            TQ_Timerunset(IKE_Phase2_Timeout_Event, TQ_CLEAR_EXACT,
                          (UNSIGNED)sa, (UNSIGNED)phase2);

            /* Also remove conflicting events from the
             * IKE event list.
             */
            IKE_Unset_Matching_Events((UNSIGNED)sa, (UNSIGNED)phase2,
                                      TQ_CLEAR_EXACT);

            /* Release the NET semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

} /* IKE_Phase2_Timeout_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Unset_Single_Event
*
* DESCRIPTION
*
*       This function removes instances of the specified
*       IKE event which match the search criteria. These
*       events are removed from the IKE event list.
*
*       The caller is responsible for obtaining the IKE
*       and TCP semaphores before calling this function.
*
* INPUTS
*
*       event_id                Event identifier.
*       dat                     Arbitrary event parameter.
*       ext_dat                 Arbitrary event parameter.
*       match_type              The matching type argument as
*                               expected by the TQ_Timerunset
*                               function.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Unset_Single_Event(TQ_EVENT event_id, UNSIGNED dat,
                            UNSIGNED ext_dat, INT16 match_type)
{
    IKE_EVENT       *event;
    IKE_EVENT       *next_event;
    UINT8           match;

    /* Loop for all items in the event list. */
    for(event = IKE_Event_List.ike_flink;
        event != NU_NULL;
        event = next_event)
    {
        /* Save pointer to the next event. */
        next_event = event->ike_flink;

        /* Initialize match flag to false. */
        match = NU_FALSE;

        /* Determine the matching criteria. */
        switch(match_type)
        {
        case TQ_CLEAR_ALL:
            /* All events must match. Both data members disregarded. */
            if(event->ike_id == event_id)
            {
                match = NU_TRUE;
            }
            break;

        case TQ_CLEAR_ALL_EXTRA:
            /* Data must match. */
            if((event->ike_id == event_id) && (event->ike_dat == dat))
            {
                match = NU_TRUE;
            }
            break;

        case TQ_CLEAR_EXACT:
            /* Data and extra data must match. */
            if((event->ike_id == event_id) && (event->ike_dat == dat) &&
               (event->ike_ext_dat == ext_dat))
            {
                match = NU_TRUE;
            }
            break;
        }

        /* If a match is found. */
        if(match == NU_TRUE)
        {
            /* Remove event from the event list. */
            SLL_Remove(&IKE_Event_List, event);

            /* Return event to the free list. */
            SLL_Enqueue(&IKE_Event_Freelist, event);
        }
    }

} /* IKE_Unset_Single_Event */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Unset_Matching_Events
*
* DESCRIPTION
*
*       This function removes all events which match the
*       criteria specified by the caller, from the IKE
*       event list.
*
*       The caller is responsible for obtaining the IKE
*       and TCP semaphores before calling this function.
*
* INPUTS
*
*       dat                     Arbitrary event parameter.
*       ext_dat                 Arbitrary event parameter.
*       match_type              The matching type argument as
*                               expected by the TQ_Timerunset
*                               function.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Unset_Matching_Events(UNSIGNED dat, UNSIGNED ext_dat,
                               INT16 match_type)
{
    IKE_EVENT       *event;
    IKE_EVENT       *next_event;
    UINT8           match;

    /* Loop for all items in the event list. */
    for(event = IKE_Event_List.ike_flink;
        event != NU_NULL;
        event = next_event)
    {
        /* Save pointer to the next event. */
        next_event = event->ike_flink;

        /* Initialize match flag to false. */
        match = NU_FALSE;

        /* Determine the matching criteria. */
        switch(match_type)
        {
        case TQ_CLEAR_ALL:
            /* All events must match. Both data members disregarded. */
            match = NU_TRUE;
            break;

        case TQ_CLEAR_ALL_EXTRA:
            /* Data must match. */
            if(event->ike_dat == dat)
            {
                match = NU_TRUE;
            }
            break;

        case TQ_CLEAR_EXACT:
            /* Data and extra data must match. */
            if((event->ike_dat == dat) && (event->ike_ext_dat == ext_dat))
            {
                match = NU_TRUE;
            }
            break;
        }

        /* If a match is found. */
        if(match == NU_TRUE)
        {
            /* Remove event from the event list. */
            SLL_Remove(&IKE_Event_List, event);

            /* Return event to the free list. */
            SLL_Enqueue(&IKE_Event_Freelist, event);
        }
    }

} /* IKE_Unset_Matching_Events */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Set_Timer
*
* DESCRIPTION
*
*       This function registers a timer event with the
*       Nucleus NET TQ component.
*
* INPUTS
*
*       event                   Event identifier.
*       dat                     Arbitrary event parameter.
*       ext_dat                 Arbitrary event parameter.
*       how_long                Number of seconds after which
*                               the event must be triggered.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       -1                      If the Nucleus NET TQ call failed.
*
*************************************************************************/
STATUS IKE_Set_Timer(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat,
                     UNSIGNED how_long)
{
    STATUS          status;

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Set the timer. */
        status = TQ_Timerset(event, dat,
                             (UNSIGNED)(how_long * SCK_Ticks_Per_Second),
                             ext_dat);

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Set_Timer */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Unset_Timer
*
* DESCRIPTION
*
*       This function removes a registered timer event from
*       the Nucleus NET TQ component.
*
* INPUTS
*
*       event                   Event identifier.
*       dat                     Arbitrary event parameter.
*       ext_dat                 Arbitrary event parameter.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*
*************************************************************************/
STATUS IKE_Unset_Timer(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat)
{
    STATUS          status;

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Remove timer from TQ timer list. */
        status = TQ_Timerunset(event, TQ_CLEAR_EXACT, dat, ext_dat);

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Unset_Timer */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Unset_Matching_Timers
*
* DESCRIPTION
*
*       This function removes all registered IKE timer
*       events which match the specified criteria, from
*       the Nucleus NET TQ component.
*
*       The caller is responsible for obtaining the IKE
*       and TCP semaphores before calling this function.
*
* INPUTS
*
*       dat                     Arbitrary event parameter.
*       ext_dat                 Arbitrary event parameter.
*       match_type              The matching type argument
*                               expected by the TQ_Timerunset
*                               function.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE_Unset_Matching_Timers(UNSIGNED dat, UNSIGNED ext_dat,
                                 INT16 match_type)
{
    STATUS          status;

    /* Remove matching message reply events. */
    status = TQ_Timerunset(IKE_Message_Reply_Event,
                           match_type, dat, ext_dat);

    if(status == NU_SUCCESS)
    {
        /* Remove matching SA timeout events. */
        status = TQ_Timerunset(IKE_SA_Timeout_Event,
                               match_type, dat, ext_dat);

        if(status == NU_SUCCESS)
        {
            /* Remove matching phase 1 timeout events. */
            status = TQ_Timerunset(IKE_Phase1_Timeout_Event,
                                   match_type, dat, ext_dat);

            if(status == NU_SUCCESS)
            {
                /* Remove matching phase 2 timeout events. */
                status = TQ_Timerunset(IKE_Phase2_Timeout_Event,
                                       match_type, dat, ext_dat);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Failed to unset phase 2 timeout events",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to unset phase 1 timeout events",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to unset SA timeout events",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to unset message reply events",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Unset_Matching_Timers */
