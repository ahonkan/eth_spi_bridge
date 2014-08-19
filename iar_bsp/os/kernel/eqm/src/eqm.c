/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
* FILE NAME
*
*       eqm.c
*
* COMPONENT
*
*       Nucleus Event Queue Manager
*
* DESCRIPTION
*
*       This file contains the routines for the Nucleus Event Queue
*       Manager component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       eqm_wait_event
*       eqm_search_buffer
*       eqm_get_event_handle
*       NU_EQM_Create
*       NU_EQM_Delete
*       NU_EQM_Post_Event
*       NU_EQM_Wait_Event
*       NU_EQM_Get_Event_Data
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*
*************************************************************************/

#define   EQM_SOURCE_FILE

/* Include the necessary files.   */
#include  "nucleus.h"
#include  "kernel/nu_kernel.h"
#include  "os/kernel/plus/core/inc/thread_control.h"

/* Local Functions declarations */

static  STATUS  eqm_wait_event(EQM_EVENT_QUEUE *event_queue_ptr,
                                EQM_EVENT_HANDLE from_index,
                                UINT32 requested_events,
                                UINT32 *recvd_event_type_ptr,
                                EQM_EVENT_ID *recvd_event_id_ptr,
                                EQM_EVENT_HANDLE *recvd_event_handle_ptr);

static  STATUS  eqm_search_buffer(EQM_EVENT_QUEUE *event_queue_ptr,
                                EQM_EVENT_HANDLE from_handle,
                                UINT32 requested_events,
                                UINT32 *recvd_event_type_ptr,
                                EQM_EVENT_ID *recvd_event_id_ptr,
                                EQM_EVENT_HANDLE *recvd_event_handle_ptr);

static  STATUS  eqm_get_event_handle(EQM_EVENT_QUEUE *event_queue_ptr,
                                EQM_EVENT_ID event_id,
                                EQM_EVENT_HANDLE end_index,
                                EQM_EVENT_HANDLE *event_handle);

/*************************************************************************
* FUNCTION
*
*       NU_EQM_Create
*
* DESCRIPTION
*
*       This function creates a new instance of EQM and initializes the
*       data structures that control the operation of the EQM.
*
* CALLED BY
*
*       INC_Initialize                  System initialization
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Event queue pointer
*       queue_size                      Number of elements in the queue
*       max_event_data_size             Maximum size of event data
*       memory_pool_ptr                 Memory pool pointer
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_EQM_INVALID_INPUT            Invalid input
*
*************************************************************************/
STATUS  NU_EQM_Create(EQM_EVENT_QUEUE *event_queue_ptr,
                      UINT32 queue_size, UINT16 max_event_data_size,
                      NU_MEMORY_POOL *memory_pool_ptr)
{
    STATUS              status = NU_SUCCESS;
    int                 i;

    /* Check the input parameters. */
    if (event_queue_ptr == NU_NULL || memory_pool_ptr == NU_NULL ||
            queue_size == 0)
    {
        status = NU_EQM_INVALID_INPUT;
    }
    else
    {
        /* Do a single allocation for for eqm event nodes and the data buffers. */
        status = NU_Allocate_Memory(memory_pool_ptr,
                    (VOID**)&(event_queue_ptr->eqm_event_data_buffer),
                    ((sizeof(EQM_EVENT_NODE) + max_event_data_size) * queue_size),
                    NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {                        
            /* Initialize the event data buffer with '0'. */
            (VOID)memset(event_queue_ptr->eqm_event_data_buffer,
                         0, ((sizeof(EQM_EVENT_NODE) + max_event_data_size) * queue_size));

            /* Setup the pointer for eqm event data. */
            event_queue_ptr->eqm_event_data_buffer[0].eqm_event_data = 
                    (UINT8*)(((UINT32)event_queue_ptr->eqm_event_data_buffer) + (queue_size * sizeof(EQM_EVENT_NODE)));
                    

            /* Assign the memory allocated for event queue data
                buffer element to the respective elements. */
            for (i=1; i < queue_size; i++)
            {
                event_queue_ptr->eqm_event_data_buffer[i].eqm_event_data =
                event_queue_ptr->eqm_event_data_buffer[i -1].eqm_event_data + max_event_data_size;
            }
            
            /* Initialize the component event group. */
            (VOID)memset (&(event_queue_ptr->eqm_event_group), 0, sizeof(NU_EVENT_GROUP));

            /* Create component event group. */
            status = NU_Create_Event_Group(&(event_queue_ptr->eqm_event_group), "EQM_EVT");

            if (status != NU_SUCCESS)
            {
                /* Deallocate memory allocated to event queue data buffer. */
                (VOID)NU_Deallocate_Memory(event_queue_ptr->eqm_event_data_buffer);
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Initialize the Event ID that will be assigned to the first event */
            event_queue_ptr->eqm_current_event_id = 1;

            /* Initialize the handle pointing to the index where the received
               event will be placed */
            event_queue_ptr->eqm_buffer_index = 0;

            /* Save the queue size, that is total number of events in the
               queue. */ 
            event_queue_ptr->eqm_max_events = queue_size;

            /* Save the maximum data size that may be send with an event
               in the queue */  
            event_queue_ptr->eqm_max_event_data_size = max_event_data_size;
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_EQM_Delete
*
* DESCRIPTION
*
*       This function deletes the instance of the Event Queue Manager.
*
* CALLED BY
*
*       EQM_NMI_Init_Cleanup_Cb
*
* CALLS
*
*       NU_Delete_Event_Group
*       NU_Deallocate_Memory
*
* INPUTS
*
*       event_queue_ptr                 Event queue pointer
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_INVALID_POINTER              Invalid pointer
*       NU_EQM_INVALID_INPUT            Invalid event queue
*
*************************************************************************/
STATUS NU_EQM_Delete(EQM_EVENT_QUEUE *event_queue_ptr)
{
    STATUS status = NU_SUCCESS;
    STATUS alloc_status;

    /* Check the event queue pointer first. */
    if (event_queue_ptr == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }
    else
    {
        /* Delete the component events group. */
        status = NU_Delete_Event_Group(&(event_queue_ptr->eqm_event_group));

        /* Deallocate memory allocated to event queue data buffer. */
        alloc_status = NU_Deallocate_Memory(event_queue_ptr->eqm_event_data_buffer);
        
        /* Check for allocation status */
        if (alloc_status != NU_SUCCESS)
        {
            /* If both function calls report failure 
               this is not a valid event queue */
            if (status != NU_SUCCESS)
            {
                status = NU_EQM_INVALID_INPUT;
            }
            else
            {
                /* Return the error from allocation */
                status = alloc_status;
            }
        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_EQM_Post_Event
*
* DESCRIPTION
*
*       This function posts the event in the data buffer and inform the
*       waiting components about the arrival of the event.
*
* CALLED BY
*
*       Components that want to post the event
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Pointer to the event queue where
*                                       event is to be posted
*       event_ptr                       Pointer to the event containing
*                                       the event type and associated data
*       event_data_size                 Size of associated event data in
*                                       bytes 
*       posted_event_id_ptr             Pointer to hold the posted event
*                                       id. NU_NULL is passed if it is 
*                                       not required.
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_INVALID_POINTER              Invalid pointer
*       NU_EQM_INVALID_EVENT_SIZE       Invalid event data size. Size is
*                                       greater than the limit specified
*                                       for the queue.
*
*************************************************************************/
STATUS  NU_EQM_Post_Event(EQM_EVENT_QUEUE *event_queue_ptr,
                          EQM_EVENT *event_ptr, UINT16 event_data_size,
                          EQM_EVENT_ID *posted_event_id_ptr)
{
    STATUS status = NU_SUCCESS;

    /* Check the input parameters. */
    if (event_queue_ptr == NU_NULL || event_ptr == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }
    /* Check whether the Event data size is in valid range. */
    else if (event_data_size < sizeof(UINT32) ||
             event_data_size > event_queue_ptr->eqm_max_event_data_size)
    {
        status = NU_EQM_INVALID_EVENT_SIZE;
    }
    else
    {
        /* Lock out the scheduler during the critcal section. */
        TCCT_Schedule_Lock();

        /* Assign a unique Id to the posted event in the event queue. */
        event_queue_ptr->eqm_event_data_buffer[event_queue_ptr->eqm_buffer_index].eqm_event_id =
                                        event_queue_ptr->eqm_current_event_id;

        /* Assign the unique Id to the parameter so that the caller may access it. */
        if (posted_event_id_ptr != NU_NULL)
        {
            *posted_event_id_ptr = event_queue_ptr->eqm_current_event_id;
        }

        /* Store the event data size in the event queue as well. */
        event_queue_ptr->eqm_event_data_buffer[event_queue_ptr->eqm_buffer_index].eqm_event_data_size =
                                        event_data_size;

        /* Copy the posted event data in the event queue. */
        memcpy(
            event_queue_ptr->eqm_event_data_buffer[event_queue_ptr->eqm_buffer_index].eqm_event_data,
            event_ptr, (size_t)event_data_size);

        /* Increment the event queue buffer index. */
        event_queue_ptr->eqm_buffer_index++;
        /* Check for array wraparound */
        if (event_queue_ptr->eqm_buffer_index ==
            event_queue_ptr->eqm_max_events)
        {
            event_queue_ptr->eqm_buffer_index = 0;
        }

        /* Increment the event queue current event ID. This will be 
           assigned to the next event. */
        event_queue_ptr->eqm_current_event_id++;
        /* Check for reserve value '0'.
           It cann't be assigned to any event */
        if (event_queue_ptr->eqm_current_event_id == 0)
        {
            event_queue_ptr->eqm_current_event_id++;
        }

        /* Report about the received event to all the waiting components. */
        (VOID)NU_Set_Events(&(event_queue_ptr->eqm_event_group),
                            event_ptr->eqm_event_type, NU_OR);

        /* Release the lock. */
        TCCT_Schedule_Unlock();
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_EQM_Get_Event_Data
*
* DESCRIPTION
*
*       The components that are interested in reading data associated with
*       the event call this function.
*
* CALLED BY
*
*       Components that want to read the event data
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Pointer to the event queue from
*                                       where event data to be extracted
*       event_id                        ID of the event whose data to be
*                                       read
*       event_handle                    Handle of the event whose data
*                                       to be read. Handle is used to
*                                       avoid searching overhead.
*       event_ptr                       Points to the structure where
*                                       event's data will be copied
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_EQM_EVENT_WITHOUT_DATA       The Event type doesn't have data
*       NU_INVALID_POINTER              Invalid pointer
*       NU_EQM_INVALID_HANDLE           Invalid handle
*       NU_EQM_EVENT_EXPIRED            Event expired. Doesn't exist in
*                                       the buffer anymore
*
*************************************************************************/
STATUS  NU_EQM_Get_Event_Data(EQM_EVENT_QUEUE *event_queue_ptr,
							  EQM_EVENT_ID event_id,
							  EQM_EVENT_HANDLE event_handle,
							  EQM_EVENT *event_ptr)
{
    STATUS  status = NU_SUCCESS;

    /* Check the input parameters. */
    if (event_ptr == NU_NULL || event_queue_ptr == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }
    /* Validate the handle. */
    else if (event_handle >= event_queue_ptr->eqm_max_events)
    {
        status = NU_EQM_INVALID_HANDLE;
    }
    /* Check if the event type has any associated data or not. */
    else if (event_queue_ptr->eqm_event_data_buffer[event_handle].eqm_event_id == event_id &&
             event_queue_ptr->eqm_event_data_buffer[event_handle].eqm_event_data_size == sizeof(UINT32))
    {
        status = NU_EQM_EVENT_WITHOUT_DATA;
    }

    if (status == NU_SUCCESS)
    {
        /* Lock out the scheduler during the critcal section. */
        TCCT_Schedule_Lock();
        
        /* Validate that the event is not overridden in the mean time. */
        if (event_queue_ptr->eqm_event_data_buffer[event_handle].eqm_event_id !=
            event_id)
        {
            status = NU_EQM_EVENT_EXPIRED;
        }
        else
        {
            /* Copy the associated in the caller event structure. */
            memcpy(event_ptr,
                event_queue_ptr->eqm_event_data_buffer[event_handle].eqm_event_data,
                (size_t)event_queue_ptr->eqm_event_data_buffer[event_handle].eqm_event_data_size);
                
        }

        /* Release the lock. */
        TCCT_Schedule_Unlock();
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_EQM_Wait_Event
*
* DESCRIPTION
*
*       The components that are interested in a particular event reading
*       data associated with the event call this function.
*
* CALLED BY
*
*       Components that want to wait for some events.
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Pointer to the event queue
*       requested_events_mask           Mask that specifies the events to
*                                       wait for.
*       recvd_event_type_ptr            Points to the location where type
*                                       of the received event will be
*                                       placed.
*       recvd_event_id_ptr              Points to the location which has
*                                       event ID of last processed event.
*                                       The new event of relevant type
*                                       will be searched from there
*                                       onwards. For the first call the
*                                       event_id must be '0'. A following
*                                       '0' is used to indicate skipping
*                                       search and wait for next received
*                                       event of relevant type. When the
*                                       call returns it contains the ID of
*                                       the received event.
*       recvd_event_handle_ptr          Points to the location where a
*                                       handle to the received event will
*                                       be placed. This handle is required
*                                       if data associated with the event
*                                       is to be read.
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_INVALID_POINTER              Invalid pointer
*       NU_EQM_INVALID_INPUT            Invalid input. Mask is 0
*
*************************************************************************/
STATUS  NU_EQM_Wait_Event(EQM_EVENT_QUEUE *event_queue_ptr,
                          UINT32 requested_events_mask,
						  UINT32 *recvd_event_type_ptr,
						  EQM_EVENT_ID *recvd_event_id_ptr,
						  EQM_EVENT_HANDLE *recvd_event_handle_ptr)
{
    STATUS             status = NU_SUCCESS;
    UINT32             recieved_event_type = 0;
    EQM_EVENT_HANDLE   last_processed_event_handle = 0;
    EQM_EVENT_HANDLE   last_recieved_event_handle = 0;
    EQM_EVENT_HANDLE   event_handle = 0;

    /* Check the input parameters. */
    if (event_queue_ptr == NU_NULL ||
        recvd_event_type_ptr == NU_NULL ||
        recvd_event_id_ptr == NU_NULL ||
        recvd_event_handle_ptr == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }
    else if (requested_events_mask == 0)
    {
        status = NU_EQM_INVALID_INPUT;
    }
    else
    {
        /* Lock out the scheduler during the critical section. */
        TCCT_Schedule_Lock();
                    
        /* Identify the last received event handle. */
        if (event_queue_ptr->eqm_buffer_index == 0)
        {
            last_recieved_event_handle = event_queue_ptr->eqm_max_events - 1;
        }
        else
        {
            last_recieved_event_handle = event_queue_ptr->eqm_buffer_index - 1;
        }

        /* Interested in events of relevant type that occur after this call */
        if (*recvd_event_id_ptr == 0)
        {
            /* Release the lock. */
            TCCT_Schedule_Unlock();

            /* Wait for the next occurrence of the events of requested type. */
            status = eqm_wait_event(event_queue_ptr,
                            last_recieved_event_handle,
                            requested_events_mask,
                            recvd_event_type_ptr,
                            recvd_event_id_ptr,
                            recvd_event_handle_ptr);
        }
        /* Interested in first relevant event that occurred after the specified
            last processed event */
        else
        {
            /* Get the handle of the last received event */
            if (eqm_get_event_handle(event_queue_ptr,
                        *recvd_event_id_ptr,
                        last_recieved_event_handle,
                        &last_processed_event_handle) == NU_SUCCESS)
            {
                /* Search the buffer for event of relevant type after
                the last processed event */
                event_handle = last_processed_event_handle;
            }
            else
            {
                /* Search the complete buffer for event of relevant type */
                event_handle = event_queue_ptr->eqm_max_events;
            }

            /* Search the buffer for event of relevant type */
            status = eqm_search_buffer(event_queue_ptr,
                                    event_handle,
                                    requested_events_mask,
                                    recvd_event_type_ptr,
                                    recvd_event_id_ptr,
                                    recvd_event_handle_ptr);

            /* Event of relevant type found in the buffer */
            if (status == NU_SUCCESS)
            {
                /* Consume the event flag. */ 
                (VOID)NU_Retrieve_Events(&(event_queue_ptr->eqm_event_group),
                            *recvd_event_type_ptr, NU_OR_CONSUME,
                            &recieved_event_type, NU_NO_SUSPEND);
                
                /* Release the lock. */
                TCCT_Schedule_Unlock();

            }
                
            /* Event of relevant type not found in the buffer */
            else
            {
                /* Release the lock. */
                TCCT_Schedule_Unlock();

                /* Start waiting for event of requested type */
                status = eqm_wait_event(event_queue_ptr,
                                last_recieved_event_handle,
                                requested_events_mask,
                                recvd_event_type_ptr,
                                recvd_event_id_ptr,
                                recvd_event_handle_ptr);
            }
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       eqm_wait_event
*
* DESCRIPTION
*
*       This function waits for the occurrence of event of relevant type.
*       It returns the event type, ID and handle of the first relevant
*       event after the specified index.
*
* CALLED BY
*
*       EQM_Wait_Event
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Pointer to the event queue
*       from_handle                     Search the events of relevant type
*                                       in the buffer, after this index.
*       requested_events                Mask that specifies the events to
*                                       wait for.
*       recvd_event_type_ptr            Points to the location where type
*                                       of the received event will be
*                                       placed.
*       recvd_event_id_ptr              Points to the location which has
*                                       event ID of last processed event.
*                                       The new event of relevant type
*                                       will be searched from there
*                                       onwards. For the first call the
*                                       event_id must be '0'. A following
*                                       '0' is used to indicate skipping
*                                       search and wait for next received
*                                       event of relevant type. When the
*                                       call returns it contains the ID of
*                                       the received event.
*       recvd_event_handle_ptr          Points to the location where a
*                                       handle to the received event will
*                                       be placed. This handle is required
*                                       if data associated with the event
*                                       is to be read.
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*
*************************************************************************/
static STATUS   eqm_wait_event(EQM_EVENT_QUEUE *event_queue_ptr,
                            EQM_EVENT_HANDLE from_handle,
                            UINT32 requested_events,
                            UINT32 *recvd_event_type_ptr,
                            EQM_EVENT_ID *recvd_event_id_ptr,
                            EQM_EVENT_HANDLE *recvd_event_handle_ptr)
{
    UINT32  recieved_event_type = 0;
    STATUS  status = NU_SUCCESS;
    UINT32  event_type;

    /* Compare the received event type with the requested events mask. */
    while ((requested_events & recieved_event_type) == 0)
    {
        /* Waiting for the next occurrence of the relevant event */
        (VOID)NU_Retrieve_Events(&(event_queue_ptr->eqm_event_group),
                       requested_events, NU_OR_CONSUME,
                       &event_type, NU_SUSPEND);

        /* Lock out the scheduler during the critcal section. */
        TCCT_Schedule_Lock();
                
        /* Search the event queue to determine the
            received event type. */
        status = eqm_search_buffer(event_queue_ptr,
                        from_handle,
                        requested_events,
                        recvd_event_type_ptr,
                        recvd_event_id_ptr,
                        recvd_event_handle_ptr);

        /* Release the lock. */
        TCCT_Schedule_Unlock();

        if (status == NU_SUCCESS)
        {
            /* Assign the received event type to local variable. */
            recieved_event_type = *recvd_event_type_ptr;
        }

    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       eqm_search_buffer
*
* DESCRIPTION
*
*       This function searches the buffer for event of relevant type.It
*       returns the event type, ID and handle of the first relevant event
*       after the specified index.
*
* CALLED BY
*
*       EQM_Wait_Event
*       eqm_wait_event
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Pointer to the event queue
*       from_handle                     Search the events of relevant type
*                                       in the buffer, after this index.
*                                       EQM_MAX_EVENTS is reserve value
*                                       used to search all the buffer.
*       requested_events                Mask that specifies the events to
*                                       wait for.
*       recvd_event_type_ptr            Points to the location where type
*                                       of the received event will be
*                                       placed.
*       recvd_event_id_ptr              Points to the location which has
*                                       event ID of last processed event.
*                                       The new event of relevant type
*                                       will be searched from there
*                                       onwards. For the first call the
*                                       event_id must be '0'. A following
*                                       '0' is used to indicate skipping
*                                       search and wait for next received
*                                       event of relevant type. When the
*                                       call returns it contains the ID of
*                                       the received event.
*       recvd_event_handle_ptr          Points to the location where a
*                                       handle to the received event will
*                                       be placed. This handle is required
*                                       if data associated with the event
*                                       is to be read.
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_NOT_PRESENT                  The event type of requested mask
*                                       is not present in the buffer after
*                                       the index.
*************************************************************************/
static STATUS   eqm_search_buffer(EQM_EVENT_QUEUE *event_queue_ptr,
                                EQM_EVENT_HANDLE from_handle,
                                UINT32 requested_events,
                                UINT32 *recvd_event_type_ptr,
                                EQM_EVENT_ID *recvd_event_id_ptr,
                                EQM_EVENT_HANDLE *recvd_event_handle_ptr)
{
    STATUS  status = NU_NOT_PRESENT;
    UINT32  iterator = 0;
    UINT8   search_all_buffer_flag = 0;

    /* If the start index for the searching is a reserved value of
       max events we will perform the complete search in the queue. */
    if (from_handle == event_queue_ptr->eqm_max_events)
    {
        /* Set the iterator to the buffer index. */
        iterator = event_queue_ptr->eqm_buffer_index;
        /* Set the flag to search the oldest element in this case. */
        search_all_buffer_flag = 1;
    }

    else
    {
        /* Set the iterator to the next index from the specified index. */
        iterator = from_handle + 1;
        /* Check for iterator wraparound. */
        if (iterator == event_queue_ptr->eqm_max_events)
        {
            iterator = 0;
        }
    }

    /* Search while match is not found. */
    while (iterator != event_queue_ptr->eqm_buffer_index || search_all_buffer_flag == 1)
    {
        /* Check if the event at the index matches the requested event mask. */
        if ((requested_events &
        (*((EQM_EVENT*)(event_queue_ptr->eqm_event_data_buffer[iterator].eqm_event_data))).eqm_event_type) != 0)
        {
            /* Copy the event type to the parameter. */
            *recvd_event_type_ptr = (*((EQM_EVENT*)
            (event_queue_ptr->eqm_event_data_buffer[iterator].eqm_event_data))).eqm_event_type;
            /* Copy the event id to the parameter. */
            *recvd_event_id_ptr = event_queue_ptr->eqm_event_data_buffer[iterator].eqm_event_id;
            /* Copy the event handle to the parameter. */
            *recvd_event_handle_ptr = iterator;
            /* Set the search status to success. */
            status = NU_SUCCESS;
            /* Break the search loop. */
            break;
        }

        else
        {
            /* Clear the search flag. */
            search_all_buffer_flag = 0;
            /* Increment the iterator to the next index in event queue. */
            iterator++;
            /* Check for iterator wraparound. */
            if (iterator == event_queue_ptr->eqm_max_events)
            {
                iterator = 0;
            }
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       eqm_get_event_handle
*
* DESCRIPTION
*
*       This function returns the index of the buffer for the event having
*       the specified identifier.
*
* CALLED BY
*
*       EQM_Wait_Event
*
* CALLS
*
*       None
*
* INPUTS
*
*       event_queue_ptr                 Pointer to the event queue
*       event_id                        Identifier of the event whose
*                                       index is required.
*       last_recvd_handle               Index of the last received event.
*       event_handle                    Points to the location where the
*                                       index of the specified event will
*                                       be placed.
*
* OUTPUTS
*
*       NU_SUCCESS                      Success
*       NU_EQM_INVALID_INPUT            The event ID passed as argument
*                                       doesn't exist in the buffer
*
*************************************************************************/
static STATUS   eqm_get_event_handle(EQM_EVENT_QUEUE *event_queue_ptr,
                                    EQM_EVENT_ID event_id,
                                    EQM_EVENT_HANDLE last_recvd_handle,
                                    EQM_EVENT_HANDLE *event_handle)
{
    STATUS  status = NU_SUCCESS;
    UINT32  offset = 0;

    /* Checking for event ID validity */
    /* If ID wraparound hasn't occurred */
    if (event_queue_ptr->eqm_event_data_buffer[event_queue_ptr->eqm_buffer_index].eqm_event_id <
        event_queue_ptr->eqm_event_data_buffer[last_recvd_handle].eqm_event_id)
    {
        /* if event_id exists in the range */
        if (event_id >= event_queue_ptr->eqm_event_data_buffer[event_queue_ptr->eqm_buffer_index].eqm_event_id
            && event_id <=
                event_queue_ptr->eqm_event_data_buffer[last_recvd_handle].eqm_event_id)
        {
            /* Determine the offset from the last received event. */
            offset = event_queue_ptr->eqm_event_data_buffer[last_recvd_handle].eqm_event_id -
                        event_id;
        }
        else
        {
            status = NU_EQM_INVALID_INPUT;
        }
    }
    /* ID wraparound has occurred */
    else
    {
        if (event_id >= event_queue_ptr->eqm_event_data_buffer[event_queue_ptr->eqm_buffer_index].eqm_event_id)
        {
            /* Determine the offset from the last received event. */
            offset = (0xFFFFFFFF - event_id) +
                    event_queue_ptr->eqm_event_data_buffer[last_recvd_handle].eqm_event_id;
        }
        else if (event_id <=
                    event_queue_ptr->eqm_event_data_buffer[last_recvd_handle].eqm_event_id)
        {
            /* Determine the offset from the last received event. */
            offset = event_queue_ptr->eqm_event_data_buffer[last_recvd_handle].eqm_event_id -
                        event_id;
        }
        else
        {
            status = NU_EQM_INVALID_INPUT;
        }
    }

    /* If event ID exists in the buffer */
    if (status == NU_SUCCESS)
    {
        /* Determine event handle from event ID. */
        if (offset <= last_recvd_handle)
        {
            *event_handle = last_recvd_handle - offset;
        }
        else
        {
            *event_handle = event_queue_ptr->eqm_max_events -
                            (offset - last_recvd_handle);
        }
    }
    
    /* Return the completion status of the service. */
    return (status);
}
