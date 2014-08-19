/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       net_ntfy.c
*
*   DESCRIPTION
*
*       This file contains those routines responsible for notifying the
*       application that a fatal error has occurred in the stack.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NET_Notify
*       NET_NTFY_HISR
*       NET_NTFY_Init
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

static NU_HISR              NET_NTFY_HISR_CB;

#if ( (NU_ENABLE_NOTIFICATION == NU_TRUE) || (NU_DEBUG_NET == NU_TRUE) )
static INT                  NET_NTFY_List_Index;
#endif

NET_NTFY_STRUCT             NET_NTFY_List[NET_NTFY_LIST_LENGTH];
NU_QUEUE                    NET_NTFY_Msg_Queue;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

CHAR    NET_NTFY_Hisr_Memory[NET_NTFY_HISR_SIZE];
CHAR    NET_NTFY_Queue_Memory[NET_NTFY_MAX_ENTRIES * NET_DBG_QUEUE_ELEMENT_SIZE];

#endif

VOID   NET_NTFY_HISR(VOID);

#if ( (NU_ENABLE_NOTIFICATION == NU_TRUE) || (NU_DEBUG_NET == NU_TRUE) )
/**************************************************************************
*
*   FUNCTION
*
*       NET_Notify
*
*   DESCRIPTION
*
*       This function is called when a lower layer needs to inform the
*       application layer of an event.  The routine saves off state
*       information in a global structure and activates the HISR to
*       notify the upper layer.
*
*   INPUTS
*
*       ntfy_event              The event to notify the upper layer about.
*       *file                   The file name in which the event occurred.
*       line                    The line number at which the event occurred.
*       *task_ptr               The task in which the event occurred or
*                               NULL if the error occurred within a LISR.
*       *debug_info             Additional debug information regarding
*                               the event encountered.
*
*   OUTPUTS
*
*       None
*
****************************************************************************/
VOID NET_Notify(INT32 ntfy_event, const CHAR *file, const INT line,
                NU_TASK *task_ptr, const NET_NTFY_Debug_Struct *debug_info)
{
    INT     previous_int_value;

    /* Lock out interrupts. */
    previous_int_value =
        NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If this entry is free, fill it in */
    if (NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_flags == 0)
    {
        NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_flags |=
            NET_NTFY_NOTIFY;

        NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_status =
                (STATUS)ntfy_event;

        strcpy(NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_file,
               file);

        NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_line =
            line;

        NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_task = task_ptr;

        if (debug_info)
        {
            NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_type =
                debug_info->net_ntfy_type;

            NU_BLOCK_COPY(&NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_info,
                          debug_info->net_ntfy_info,
                          (unsigned int)debug_info->net_ntfy_length);
        }

        else
            NET_NTFY_List[NET_NTFY_List_Index].net_ntfy_type = NET_NTFY_GENERAL;

        /* If the entry before this entry is already being processed by the
         * HISR, do not activate the HISR again.
         */
        if ( (((NET_NTFY_List_Index - 1) >= 0) &&
              (!(NET_NTFY_List[NET_NTFY_List_Index - 1].net_ntfy_flags &
                 NET_NTFY_NOTIFY))) ||
             (((NET_NTFY_List_Index - 1) < 0) &&
              (!(NET_NTFY_List[NET_NTFY_LIST_LENGTH - 1].net_ntfy_flags &
                 NET_NTFY_NOTIFY))) )
        {
            NU_Activate_HISR(&NET_NTFY_HISR_CB);
        }

        /* Increment the index */
        NET_NTFY_List_Index =
            (NET_NTFY_List_Index + 1) % NET_NTFY_LIST_LENGTH;
    }

    /* Re-enable interrupts. */
    NU_Local_Control_Interrupts(previous_int_value);

} /* NET_Notify */
#endif

/**************************************************************************
*
*   FUNCTION
*
*       NET_NTFY_HISR
*
*   DESCRIPTION
*
*       This HISR is invoked when a fatal error occurs.  The routine
*       notifies the upper layer of the error.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
****************************************************************************/
VOID NET_NTFY_HISR(VOID)
{
    INT                 i;
    NET_NTFY_STRUCT     *ntfy_struct;

    for (i = 0; i < NET_NTFY_LIST_LENGTH; i++)
    {
        /* If the application needs to be notified of this one */
        if (NET_NTFY_List[i].net_ntfy_flags & NET_NTFY_NOTIFY)
        {
            ntfy_struct = &NET_NTFY_List[i];

            switch(ntfy_struct->net_ntfy_type)
            {

#if (NU_DEBUG_NET == NU_TRUE)

                case NET_NTFY_GENERAL:

                    /* Determine where the buffers are in the system, and
                     * fill in the debug structure accordingly.
                     */
                    NET_DBG_Count_Buffers(&ntfy_struct->net_ntfy_info.net_ntfy_buffer_info);

                    break;
#endif

                case NET_NTFY_ETHERNET:
                default:
                    break;
            }

            /* Put this debug structure on the queue */
            NU_Send_To_Queue(&NET_NTFY_Msg_Queue, (VOID*)ntfy_struct,
                             NET_DBG_QUEUE_ELEMENT_SIZE, NU_NO_SUSPEND);

            /* Clear the notify flag */
            ntfy_struct->net_ntfy_flags &= ~NET_NTFY_NOTIFY;
        }
    }

} /* NET_NTFY_HISR */

/**************************************************************************
*
*   FUNCTION
*
*       NET_NTFY_Init
*
*   DESCRIPTION
*
*       This function initializes the notification module.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Operating system specific error indicating that memory cannot
*       be allocated, a queue cannot be created or a HISR cannot be
*       created.
*
****************************************************************************/
STATUS NET_NTFY_Init(VOID)
{
    STATUS  status;
    VOID    *ntfy_queue_memory;
    VOID    *ntfy_hisr_memory;

    UTL_Zero(NET_NTFY_List, sizeof(NET_NTFY_List));

#if ( (NU_ENABLE_NOTIFICATION == NU_TRUE) || (NU_DEBUG_NET == NU_TRUE) )
    /* Initialize the index. */
    NET_NTFY_List_Index = 0;
#endif

    /* Create a logging message queue.  */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Allocate_Memory(MEM_Cached, &ntfy_queue_memory,
                                (NET_NTFY_MAX_ENTRIES * NET_DBG_QUEUE_ELEMENT_SIZE *
                                sizeof(UNSIGNED)), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
        return (status);

#else

    /* Assign memory to the logging message queue */
    pointer = (VOID*)NET_NTFY_Queue_Memory;

#endif

    status = NU_Create_Queue(&NET_NTFY_Msg_Queue, "Debug_Queue", ntfy_queue_memory,
                             (NET_NTFY_MAX_ENTRIES * NET_DBG_QUEUE_ELEMENT_SIZE),
                             NU_FIXED_SIZE, NET_DBG_QUEUE_ELEMENT_SIZE, NU_FIFO);

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the Interrupt-Safe Abort HISR. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ntfy_hisr_memory,
                                    NLOG_IS_LOGGING_HISR_SIZE, NU_NO_SUSPEND);
        /* Did we get the memory? */
        if (status == NU_SUCCESS)
#else
        /* Assign memory to the HISR */
        pointer = NET_NTFY_Hisr_Memory;
#endif

        {
            /* Create the HISR. */
            status = NU_Create_HISR(&NET_NTFY_HISR_CB, "NET_NTFY",
                                    NET_NTFY_HISR, 2, ntfy_hisr_memory, NET_NTFY_HISR_SIZE);

            if (status != NU_SUCCESS)
            {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                /* Deallocate memory reserved for HISR. */
                NU_Deallocate_Memory(ntfy_hisr_memory);
#endif
            }
        }

        if (status != NU_SUCCESS)
        {
            /* Delete notify queue. */
            NU_Delete_Queue(&NET_NTFY_Msg_Queue);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Deallocate memory reserved for queue. */
            NU_Deallocate_Memory(ntfy_queue_memory);
#endif
        }
    }
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    else
    {
        /* Deallocate memory reserved for Queue. */
        NU_Deallocate_Memory(ntfy_queue_memory);
    }
#endif

    return (status);

} /* NET_NTFY_Init */
