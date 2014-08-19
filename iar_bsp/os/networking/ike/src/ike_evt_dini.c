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
*       ike_evt_dini.c
*
* COMPONENT
*
*       IKE - Events
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE Events
*       de-initialization function.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Deinitialize_Events
*       IKE_Unset_All_Events
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_evt.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_evt.h"

/* External variables. */
extern IKE_EVENT_DB IKE_Event_List;
extern IKE_EVENT_DB IKE_Event_Freelist;

/* Local function prototypes. */
STATIC VOID IKE_Unset_All_Events(IKE_EVENT_DB *event_list);

/*************************************************************************
*
* FUNCTION
*
*       IKE_Deinitialize_Events
*
* DESCRIPTION
*
*       This function de-initializes the IKE Events component.
*       It deletes the IKE event handler task and then
*       un-registers the events from Nucleus NET EQ component.
*       It then de-allocates the event list memory. This
*       function MUST be called only when the IKE daemon is
*       in the IKE_DAEMON_STOPPING_MISC state.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*
*************************************************************************/
STATUS IKE_Deinitialize_Events(VOID)
{
    STATUS          status;

    IKE_DEBUG_LOG("De-initializing IKE events");

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Delete the terminated task. */
        status = NU_Delete_Task(&IKE_Data.ike_event_task_cb);

        if(status == NU_SUCCESS)
        {
            /* Deallocate task stack. */
            if(NU_Deallocate_Memory(IKE_Data.ike_event_task_stack) !=
               NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate IKE memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to delete IKE event handler task",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Un-register the Phase 2 timeout event. */
        if(EQ_Unregister_Event(IKE_Phase2_Timeout_Event) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to un-register IKE event from EQ",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Un-register the Phase 1 timeout event. */
        if(EQ_Unregister_Event(IKE_Phase1_Timeout_Event) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to un-register IKE event from EQ",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Un-register the IKE SA timeout event. */
        if(EQ_Unregister_Event(IKE_SA_Timeout_Event) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to un-register IKE event from EQ",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Un-register the IKE message reply timeout event. */
        if(EQ_Unregister_Event(IKE_Message_Reply_Event) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to un-register IKE event from EQ",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Deallocate all events from IKE event lists. Return
         * value of this function can be safely ignored.
         */
        IKE_Unset_All_Events(&IKE_Event_List);
        IKE_Unset_All_Events(&IKE_Event_Freelist);
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Deinitialize_Events */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Unset_All_Events
*
* DESCRIPTION
*
*       This function removes all events from the specified
*       event list. No protection for the event list is used
*       in this function because this function is only called
*       when IKE is shutting down.
*
* INPUTS
*
*       *event_list             Pointer to event list which
*                               is to be emptied.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_Unset_All_Events(IKE_EVENT_DB *event_list)
{
    IKE_EVENT       *event;
    IKE_EVENT       *next_event;

    /* Loop for all items in the event list. */
    for(event = event_list->ike_flink;
        event != NU_NULL;
        event = next_event)
    {
        /* Save pointer to the next event. */
        next_event = event->ike_flink;

        /* Deallocate event memory. */
        if(NU_Deallocate_Memory(event) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate IKE memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Reset event list pointers. */
    event_list->ike_flink = NU_NULL;
    event_list->ike_last  = NU_NULL;

} /* IKE_Unset_All_Events */
