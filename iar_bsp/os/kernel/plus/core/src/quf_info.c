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
*       quf_info.c
*
*   COMPONENT
*
*       QU - Queue Management
*
*   DESCRIPTION
*
*       This file contains the Information routine to obtain facts about
*       the Queue management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Queue_Information                Retrieve queue information
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       queue.h                             Queue functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/queue.h"
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Queue_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified queue.
*       However, if the supplied queue pointer is invalid, the
*       function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect queue
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       queue_ptr                           Pointer to the queue
*       name                                Destination for the name
*       start_address                       Destination for the start
*                                           address of the queue
*       queue_size                          Destination for queue size
*       available                           Destination for available
*                                           room in queue
*       messages                            Destination for number of
*                                           messages queued
*       message_type                        Destination for message type
*       message_size                        Destination for message size
*       suspend_type                        Destination for suspension
*                                           type
*       tasks_waiting                       Destination for the tasks
*                                           waiting count
*       first_task                          Destination for the pointer
*                                           to the first task waiting
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid queue pointer
*                                           is supplied
*           NU_INVALID_QUEUE                If queue pointer invalid
*
***********************************************************************/
STATUS NU_Queue_Information(NU_QUEUE *queue_ptr, CHAR *name,
                            VOID **start_address, UNSIGNED *queue_size,
                            UNSIGNED *available, UNSIGNED *messages,
                            OPTION *message_type, UNSIGNED *message_size,
                            OPTION *suspend_type, UNSIGNED *tasks_waiting,
                            NU_TASK **first_task)
{
    QU_QCB          *queue;                 /* Queue control block ptr   */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input queue pointer into internal pointer.  */
    queue =  (QU_QCB *) queue_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this queue id is valid.  */
    if ((queue != NU_NULL) && (queue -> qu_id == QU_QUEUE_ID))
    {
        /* Setup protection of the queue.  */
        TCCT_Schedule_Lock();

        /* The queue pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the queue's name.  */
        strncpy(name, queue -> qu_name, NU_MAX_NAME);

        /* Determine the suspension type.  */
        if (queue -> qu_fifo_suspend)
        {
            *suspend_type =  NU_FIFO;
        }
        else
        {
            *suspend_type =  NU_PRIORITY;
        }

        /* Determine the message type.  */
        if (queue -> qu_fixed_size)
        {
            *message_type =  NU_FIXED_SIZE;
        }
        else
        {
            *message_type =  NU_VARIABLE_SIZE;
        }

        /* Get various information about the queue.  */
        *start_address =  (UNSIGNED *) queue -> qu_start;
        *queue_size =     queue -> qu_queue_size;
        *available =      queue -> qu_available;
        *messages =       queue -> qu_messages;
        *message_size =   queue -> qu_message_size;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  queue -> qu_tasks_waiting;
        if (queue -> qu_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task =  (NU_TASK *)
                (queue -> qu_suspension_list) -> qu_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection of the queue.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the queue pointer is invalid.   */
        completion =  NU_INVALID_QUEUE;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
