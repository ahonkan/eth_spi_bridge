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
*       qus_broadcast.c
*
*   COMPONENT
*
*       QU - Queue Management
*
*   DESCRIPTION
*
*       This file contains the supplemental Broadcast routine for the
*       Queue Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Broadcast_To_Queue               Broadcast a message to queue
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       common_services.h                   Common Service Constants
*       thread_control.h                    Thread Control functions
*       queue.h                             Queue functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/core/inc/queue.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define internal component function prototypes.  */

VOID    QUC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Broadcast_To_Queue
*
*   DESCRIPTION
*
*       This function sends a message to all tasks waiting for a message
*       from the specified queue.  If there are no tasks waiting for a
*       message the service performs like a standard send request.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       NU_Priority_Place_On_List           Place on priority list
*       NU_Remove_From_List                 Remove from suspend list
*       TCC_Resume_Task                     Resume a suspended task
*       TCC_Suspend_Task                    Suspend calling task
*       TCC_Task_Priority                   Pickup task's priority
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Current_Thread                 Pickup current thread
*                                           pointer
*       TCCT_Schedule_Lock                  Protect queue
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       queue_ptr                           Queue control block pointer
*       message                             Pointer to message to send
*       size                                Size of message to send
*       suspend                             Suspension option if full
*
*   OUTPUTS
*
*       NU_SUCCESS                          If service is successful
*       NU_QUEUE_FULL                       If queue is currently full
*       NU_TIMEOUT                          If timeout on service
*                                           expires
*       NU_QUEUE_DELETED                    If queue was deleted during
*                                           suspension
*       NU_QUEUE_RESET                      If queue was reset during
*                                           suspension
*       NU_INVALID_QUEUE                    Invalid queue pointer
*       NU_INVALID_POINTER                  Invalid message pointer
*       NU_INVALID_SIZE                     Invalid message size
*       NU_INVALID_SUSPEND                  Invalid suspend request
*
***********************************************************************/
STATUS NU_Broadcast_To_Queue(NU_QUEUE *queue_ptr, VOID *message,
                             UNSIGNED size, UNSIGNED suspend)
{
    R1 QU_QCB       *queue;                 /* Queue control block ptr   */
    QU_SUSPEND      suspend_block;          /* Allocate suspension block */
    QU_SUSPEND      *suspend_ptr;           /* Pointer to suspend block  */
    R3 UNSIGNED_PTR source;                 /* Pointer to source         */
    R4 UNSIGNED_PTR destination;            /* Pointer to destination    */
    UNSIGNED        copy_size;              /* Partial copy size         */
    R2 INT          i;                      /* Working counter           */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          preempt;                /* Preempt flag              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input queue pointer into internal pointer.  */
    queue =  (QU_QCB *) queue_ptr;

    /* Determine if there is an error with the queue pointer */
    NU_ERROR_CHECK((queue == NU_NULL), status, NU_INVALID_QUEUE);

    /* Indicate that the queue pointer is invalid. */
    NU_ERROR_CHECK((queue -> qu_id != QU_QUEUE_ID), status, NU_INVALID_QUEUE);

    /* Indicate that the pointer to the message is invalid.  */
    NU_ERROR_CHECK((message == NU_NULL), status, NU_INVALID_POINTER);

    /* Indicate that the message size is invalid.   */
    NU_ERROR_CHECK((size == 0), status, NU_INVALID_SIZE);

    /* Indicate that the message size is invalid.  */
    NU_ERROR_CHECK(((queue -> qu_fixed_size) && (size != queue -> qu_message_size)), status, NU_INVALID_SIZE);

    /* Indicate that the message size is invalid.  */
    NU_ERROR_CHECK(((!queue -> qu_fixed_size) && (size > queue -> qu_message_size)), status, NU_INVALID_SIZE);

    /* Indicate that the suspension is only allowed from a task thread. */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the queue.  */
        TCCT_Schedule_Lock();

        /* Determine if an extra word of overhead needs to be added to the
           calculation.  */
        if (queue -> qu_fixed_size)
        {
            /* No overhead.  */
            i =  0;
        }
        else
        {
            /* Variable messages have one additional word of overhead.  */
            i =  1;

            /* Make special check to see if a suspension needs to be
               forced for a variable length message.  */
            if ((queue -> qu_suspension_list) && (queue -> qu_messages))
            {
                /* Pickup task control block pointer.  */
                task =  (TC_TCB *) TCCT_Current_Thread();

                /* Now we know that there are other task(s) are suspended trying
                   to send a variable length message.  Determine whether or not
                   a suspension should be forced.  */
                if ((queue -> qu_fifo_suspend) ||
                    (suspend == NU_NO_SUSPEND) ||
                    ((queue -> qu_suspension_list) -> qu_suspend_link.cs_priority <=
                                                        TCC_Task_Priority(task)))
                {
                    /* Bump the computed size to avoid placing the new variable
                       length message ahead of the suspended tasks.  */
                    i =  (INT) queue -> qu_available;
                }
            }
        }

        /* Determine if there is enough room in the queue for the message.  */
        if (queue -> qu_available < (size + i))
        {
            /* Queue does not have room for the message.  Determine if
               suspension is required.  */
            if (suspend)
            {
                /* Suspension is requested.   */

                /* Increment the number of tasks waiting.  */
                queue -> qu_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr = &suspend_block;
                suspend_ptr -> qu_queue =                    queue;
                suspend_ptr -> qu_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> qu_suspend_link.cs_previous = NU_NULL;
                suspend_ptr -> qu_message_area =           (UNSIGNED_PTR) message;
                suspend_ptr -> qu_message_size =             size;
                task =                            (TC_TCB *) TCCT_Current_Thread();
                suspend_ptr -> qu_suspended_task =           task;

                /* Determine if priority or FIFO suspension is associated with the
                   queue.  */
                if (queue -> qu_fifo_suspend)
                {
                    /* FIFO suspension is required.  Link the suspend block into
                       the list of suspended tasks on this queue.  */
                    NU_Place_On_List((CS_NODE **) &(queue -> qu_suspension_list),
                                     &(suspend_ptr -> qu_suspend_link));
                }
                else
                {
                    /* Get the priority of the current thread so the suspend block
                       can be placed in the appropriate place.  */
                    suspend_ptr -> qu_suspend_link.cs_priority =
                                                          TCC_Task_Priority(task);

                    NU_Priority_Place_On_List((CS_NODE **)
                                              &(queue -> qu_suspension_list),
                                              &(suspend_ptr -> qu_suspend_link));
                }

                /* Trace log */
                T_Q_BCAST((VOID*)queue, (VOID*)message, size, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the queue.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_QUEUE_SUSPEND,
                                            QUC_Cleanup, suspend_ptr, suspend);

                /* Pickup the return status.  */
                status =  suspend_ptr -> qu_return_status;
            }
            else
            {
                /* Return a status of NU_QUEUE_FULL because there is no
                   room in the queue for the message.  */
                status =  NU_QUEUE_FULL;

                /* Trace log */
                T_Q_BCAST((VOID*)queue, (VOID*)message, size, suspend, status);

            }
        }
        else
        {

            /* Determine if a task is waiting on an empty queue.  */
            if ((queue -> qu_suspension_list) && (queue -> qu_messages == 0))
            {
                /* Yes, one or more tasks are waiting for a message from this
                   queue.  */
                preempt =  0;
                do
                {
                    /* Decrement the number of tasks waiting on queue.  */
                    queue -> qu_tasks_waiting--;

                    /* Remove the first suspended block from the list.  */
                    suspend_ptr =  queue -> qu_suspension_list;
                    NU_Remove_From_List((CS_NODE **)
                                        &(queue -> qu_suspension_list),
                                        &(suspend_ptr -> qu_suspend_link));

                    /* Initialize the return status.  */
                    suspend_ptr -> qu_return_status =  NU_SUCCESS;

                    /* Copy the message.  */
                    memcpy(suspend_ptr -> qu_message_area, (UNSIGNED_PTR) message, QU_COPY_SIZE(size));

                    /* Return the size of the message copied.  */
                    suspend_ptr -> qu_actual_size =  size;

                    /* Trace log */
                    T_Q_BCAST((VOID*)queue, (VOID*)message, size, suspend, OBJ_UNBLKD_CTXT);

                    /* Wakeup the waiting task and check for preemption.  */
                    preempt =  preempt |
                     TCC_Resume_Task((NU_TASK *) suspend_ptr -> qu_suspended_task,
                                                             NU_QUEUE_SUSPEND);

                    /* Move the suspend pointer to the next node, which is now
                       at the head of the list.  */
                    suspend_ptr =  queue -> qu_suspension_list;
                } while (suspend_ptr);

                /* Trace log */
                T_Q_BCAST((VOID*)queue, (VOID*)message, size, suspend, OBJ_ACTION_SUCCESS);

                /* Determine if preemption needs to take place. */
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
                /* There is enough room in the queue and no task is waiting.  */

                /* Setup the source pointer.  */
                source =       (UNSIGNED_PTR) message;
                destination =  queue -> qu_write;

                /* Process according to the type of message supported.  */
                if (queue -> qu_fixed_size)
                {
                    /* Fixed-size messages are supported by this queue.  */
                    memcpy(destination, source, QU_COPY_SIZE(size));
                    destination += size;
                }
                else
                {
                    /* Variable-size messages are supported.  Processing must
                       check for queue wrap-around conditions.  */

                    /* Place message size in first location.  */
                    *(destination++) =  size;

                    /* Check for a wrap-around condition on the queue.  */
                    if (destination >= queue -> qu_end)
                    {
                        /* Wrap the write pointer back to the top of the queue
                           area.  */
                        destination =  queue -> qu_start;
                    }

                    /* Decrement the number of words remaining by 1 for this
                       extra word of overhead.  */
                    queue -> qu_available--;

                    /* Calculate the number of words remaining from the write
                       pointer to the bottom of the queue.  */
                    copy_size =  queue -> qu_end - destination;

                    /* Determine if the message needs to be wrapped around the
                       edge of the queue area.  */
                    if (copy_size >= size)
                    {
                        /* Copy the whole message at once.  */
                        memcpy(destination, source, QU_COPY_SIZE(size));
                        destination += size;
                    }
                    else
                    {
                        /* Copy the first half of the message.  */
                        memcpy(destination, source, QU_COPY_SIZE(copy_size));
                        source += copy_size;

                        /* Copy the second half of the message.  */
                        destination =  queue -> qu_start;
                        memcpy(destination, source, QU_COPY_SIZE(size - copy_size));
                        destination += (size - copy_size);
                    }
                }

                /* Check again for wrap-around condition on the write pointer. */
                if (destination >= queue -> qu_end)
                {
                    /* Move the write pointer to the top of the queue area.  */
                    queue -> qu_write =  queue -> qu_start;
                }
                else
                {
                    /* Simply copy the last position of the destination pointer
                       into the write pointer.  */
                    queue -> qu_write =  destination;
                }

                /* Decrement the number of available words.  */
                queue -> qu_available =  queue -> qu_available - size;

                /* Increment the number of messages in the queue.  */
                queue -> qu_messages++;

                /* Trace log */
                T_Q_BCAST((VOID*)queue, (VOID*)message, size, suspend, OBJ_ACTION_SUCCESS);
            }
        }

        /* Release protection against access to the queue.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_Q_BCAST((VOID*)queue, (VOID*)message, size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}
