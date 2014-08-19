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
*       quc_common.c
*
*   COMPONENT
*
*       QU - Queue Management
*
*   DESCRIPTION
*
*       This file contains the core common routines for the
*       Queue management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Queue                     Create a message queue
*       NU_Send_To_Queue                    Send message to a queue
*       NU_Receive_From_Queue               Receive a message from queue
*       QUC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       common_services.h                   Common service constants
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

/* Define external inner-component global data references.  */

extern CS_NODE         *QUD_Created_Queues_List;
extern UNSIGNED         QUD_Total_Queues;

/* Define internal component function prototypes.  */

VOID    QUC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Queue
*
*   DESCRIPTION
*
*       This function creates a queue and then places it on the list
*       of created queues.
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
*       TCCT_Schedule_Lock                  Protect created list
*       TCCT_Schedule_Unlock                Un-protect data structure
*
*   INPUTS
*
*       queue_ptr                           Queue control block pointer
*       name                                Queue name
*       start_address                       Starting address of actual
*                                           queue area
*       queue_size                          Total size of queue
*       message_type                        Type of message supported by
*                                           the queue (fixed/variable)
*       message_size                        Size of message.  Variable
*                                           message-length queues, this
*                                           represents the maximum size
*       suspend_type                        Suspension type
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_QUEUE                    Invalid queue pointer
*       NU_INVALID_MEMORY                   Invalid queue starting addr
*       NU_INVALID_SIZE                     Invalid queue size and/or
*                                           size of message
*       NU_INVALID_MESSAGE                  Invalid message type
*       NU_INVALID_SUSPEND                  Invalid suspend type
*       NU_NOT_ALIGNED                      Start address is not aligned
*
***********************************************************************/
STATUS NU_Create_Queue(NU_QUEUE *queue_ptr, CHAR *name,
                       VOID *start_address, UNSIGNED queue_size,
                       OPTION message_type, UNSIGNED message_size,
                       OPTION suspend_type)
{
    R1 QU_QCB       *queue;                 /* Queue control block ptr   */
    STATUS          status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Move input queue pointer into internal pointer. */
    queue =  (QU_QCB *) queue_ptr;

    /* Determine if there is an error with the queue pointer. */
    NU_ERROR_CHECK(((queue == NU_NULL) || (queue -> qu_id == QU_QUEUE_ID)), status, NU_INVALID_QUEUE);

    /* Determine if the starting address of the queue is valid. */
    NU_ERROR_CHECK((start_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Verify the start address is aligned */
    NU_ERROR_CHECK((ESAL_GE_MEM_ALIGNED_CHECK(start_address, sizeof(UNSIGNED)) == NU_FALSE), status, NU_NOT_ALIGNED);

    /* Verify that both of the size parameters are valid.
       NOTE: Variable-size queues require an additional word of overhead. */
    NU_ERROR_CHECK(((queue_size == 0) || (message_size == 0) || ((message_size + ((message_type == NU_VARIABLE_SIZE) ? 1 : 0)) > queue_size)), status, NU_INVALID_SIZE);

    /* Determinen if the message type is valid. */
    NU_ERROR_CHECK(((message_type != NU_FIXED_SIZE) && (message_type != NU_VARIABLE_SIZE)), status, NU_INVALID_MESSAGE);

    /* Determine if the suspend type is valid. */
    NU_ERROR_CHECK(((suspend_type != NU_FIFO) && (suspend_type != NU_PRIORITY)), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(queue, QU_QCB);

        /* Fill in the queue name. */
        strncpy(queue -> qu_name, name, (NU_MAX_NAME - 1));

        /* Setup the queue suspension type.  */
        if (suspend_type == NU_FIFO)
        {
            /* FIFO suspension is selected, setup the flag accordingly.  */
            queue -> qu_fifo_suspend =  NU_TRUE;
        }

        /* Setup the queue message type.  */
        if (message_type == NU_FIXED_SIZE)
        {
            /* Fixed-size messages are required.  */
            queue -> qu_fixed_size =  NU_TRUE;
        }

        /* Setup the message size.  */
        queue -> qu_message_size =  message_size;

        /* Setup the actual queue parameters.  */
        queue -> qu_queue_size =    queue_size;

        /* If the queue supports fixed-size messages, make sure that the queue
           size is an even multiple of the message size.  */
        if (queue -> qu_fixed_size)
        {
            /* Adjust the area of the queue being used.  */
            queue_size =  (queue_size / message_size) * message_size;
        }

        queue -> qu_available =     queue_size;
        queue -> qu_start =         (UNSIGNED *) start_address;
        queue -> qu_end =           queue -> qu_start + queue_size;
        queue -> qu_read =          (UNSIGNED *) start_address;
        queue -> qu_write =         (UNSIGNED *) start_address;

        /* Protect against access to the list of created queues.  */
        TCCT_Schedule_Lock();

        /* At this point the queue is completely built.  The ID can now be
           set and it can be linked into the created queue list.  */
        queue -> qu_id =                     QU_QUEUE_ID;

        /* Link the queue into the list of created queues and increment the
           total number of queues in the system.  */
        NU_Place_On_List(&QUD_Created_Queues_List, &(queue -> qu_created));
        QUD_Total_Queues++;

        /* Trace log */
        T_Q_CREATE((VOID*)queue, start_address, name, queue_size, message_size,
        message_type, suspend_type, OBJ_ACTION_SUCCESS);

        /* Release protection against access to the list of created queues.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        T_Q_CREATE((VOID*)queue, start_address, name, queue_size, message_size,
        message_type, suspend_type, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Send_To_Queue
*
*   DESCRIPTION
*
*       This function sends a message to the specified queue.  The
*       message length is determined by the caller.  If there are one
*       or more tasks suspended on the queue for a message, the message
*       is copied into the message area of the first waiting task.  If
*       the task's request is satisfied, it is resumed.  Otherwise, if
*       the queue cannot hold the message, suspension of the calling
*       task is an option of the caller.
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
*       status
*           NU_SUCCESS                      If service is successful
*           NU_QUEUE_FULL                   If queue is currently full
*           NU_TIMEOUT                      If timeout on service
*                                           expires
*           NU_QUEUE_DELETED                If queue was deleted during
*                                           suspension
*           NU_QUEUE_RESET                  If queue was reset during
*                                           suspension
*           NU_INVALID_QUEUE                Invalid queue pointer
*           NU_INVALID_POINTER              Invalid message pointer
*           NU_INVALID_SIZE                 Invalid message size
*           NU_INVALID_SUSPEND              Invalid suspend request
*
***********************************************************************/
STATUS NU_Send_To_Queue(NU_QUEUE *queue_ptr, VOID *message, UNSIGNED size,
                        UNSIGNED suspend)
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

    /* Move input queue pointer into internal pointer. */
    queue =  (QU_QCB *) queue_ptr;

    /* Determine if there is an error with the queue pointer. */
    NU_ERROR_CHECK((queue == NU_NULL), status, NU_INVALID_QUEUE);

    /* Determine if the queue pointer is valid. */
    NU_ERROR_CHECK((queue -> qu_id != QU_QUEUE_ID), status, NU_INVALID_QUEUE);

    /* Determine if the pointer to the message is valid. */
    NU_ERROR_CHECK((message == NU_NULL), status, NU_INVALID_POINTER);

    /* Determine if the message size is valid, non-zero. */
    NU_ERROR_CHECK((size == 0), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid, matches fixed size. */
    NU_ERROR_CHECK(((queue -> qu_fixed_size) && (size != queue -> qu_message_size)), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid, less than max variable size. */
    NU_ERROR_CHECK(((!queue -> qu_fixed_size) && (size > queue -> qu_message_size)), status, NU_INVALID_SIZE);

    /* Verify that suspension is only allowed. */
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

        /* Determine if there is enough room in the queue for the message.  The
           extra logic is to prevent a variable-length message */
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
                suspend_ptr =  &suspend_block;
                suspend_ptr -> qu_queue =                    queue;
                suspend_ptr -> qu_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> qu_suspend_link.cs_previous = NU_NULL;
                suspend_ptr -> qu_message_area =             (UNSIGNED_PTR) message;
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
                T_Q_SEND((VOID*)queue, message, size, suspend, OBJ_BLKD_CTXT);

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
                T_Q_SEND((VOID*)queue, message, size, suspend, status);
            }
        }
        else
        {

            /* Determine if a task is waiting on an empty queue.  */
            if ((queue -> qu_suspension_list) && (queue -> qu_messages == 0))
            {

                /* Task is waiting on an empty queue for a message.  */

                /* Decrement the number of tasks waiting on queue.  */
                queue -> qu_tasks_waiting--;

                /* Remove the first suspended block from the list.  */
                suspend_ptr =  queue -> qu_suspension_list;
                NU_Remove_From_List((CS_NODE **) &(queue -> qu_suspension_list),
                                    &(suspend_ptr -> qu_suspend_link));

                /* Initialize the return status.  */
                suspend_ptr -> qu_return_status =  NU_SUCCESS;

                /* Copy the message.  */
                memcpy(suspend_ptr -> qu_message_area, message, QU_COPY_SIZE(size));

                /* Return the size of the message copied.  */
                suspend_ptr -> qu_actual_size =  size;

                /* Trace log */
                T_Q_SEND((VOID*)queue, message, size, suspend, OBJ_UNBLKD_CTXT);

                /* Wakeup the waiting task and check for preemption.  */
                preempt =
                    TCC_Resume_Task((NU_TASK *) suspend_ptr -> qu_suspended_task,
                                                         NU_QUEUE_SUSPEND);

                /* Trace log */
                T_Q_SEND((VOID*)queue, message, size, suspend, OBJ_ACTION_SUCCESS);

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
                T_Q_SEND((VOID*)queue, message, size, suspend, OBJ_ACTION_SUCCESS);
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
        T_Q_SEND((VOID*)queue, message, size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Receive_From_Queue
*
*   DESCRIPTION
*
*       This function receives a message from the specified queue.  The
*       size of the message is specified by the caller.  If there is a
*       message currently in the queue, the message is removed from the
*       queue and placed in the caller's area.  Suspension is possible
*       if the request cannot be satisfied.
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
*       size                                Size of the message
*       actual_size                         Size of message received
*       suspend                             Suspension option if empty
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_QUEUE_EMPTY                  If queue is currently empty
*           NU_TIMEOUT                      If timeout on service
*                                           expires
*           NU_QUEUE_DELETED                If queue was deleted during
*                                           suspension
*           NU_QUEUE_RESET                  If queue was reset during
*                                           suspension
*           NU_INVALID_QUEUE                Invalid queue pointer
*           NU_INVALID_POINTER              Invalid message pointer
*           NU_INVALID_SIZE                 Invalid message size
*           NU_INVALID_SUSPEND              Invalid suspend request
*
***********************************************************************/
STATUS NU_Receive_From_Queue(NU_QUEUE *queue_ptr, VOID *message,
                             UNSIGNED size, UNSIGNED *actual_size,
                             UNSIGNED suspend)
{
    R1 QU_QCB       *queue;                 /* Queue control block ptr   */
    QU_SUSPEND      suspend_block;          /* Allocate suspension block */
    QU_SUSPEND      *suspend_ptr;           /* Pointer to suspend block  */
    R3 UNSIGNED_PTR source;                 /* Pointer to source         */
    R4 UNSIGNED_PTR destination;            /* Pointer to destination    */
    TC_TCB          *task;                  /* Task pointer              */
    UNSIGNED        copy_size;              /* Number of words to copy   */
    R2 INT          i;                      /* Working counter           */
    STATUS          preempt;                /* Preemption flag           */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input queue pointer into internal pointer.  */
    queue =  (QU_QCB *) queue_ptr;

    /* Determine if there is an error with the queue pointer.  */
    NU_ERROR_CHECK((queue == NU_NULL), status, NU_INVALID_QUEUE);

    /* Determine if the queue pointer is valid. */
    NU_ERROR_CHECK((queue -> qu_id != QU_QUEUE_ID), status, NU_INVALID_QUEUE);

    /* Determine if the pointer to the message is valid. */
    NU_ERROR_CHECK((message == NU_NULL), status, NU_INVALID_POINTER);

    /* Determine if the message size is valid, non-zero. */
    NU_ERROR_CHECK((size == 0), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid, matches fixed size. */
    NU_ERROR_CHECK(((queue -> qu_fixed_size) && (size != queue -> qu_message_size)), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid, less than max variable size. */
    NU_ERROR_CHECK(((!queue -> qu_fixed_size) && (size > queue -> qu_message_size)), status, NU_INVALID_SIZE);

    /* Verify that suspension is only allowed. */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Initialize the status as successful.  */
        status =  NU_SUCCESS;

        /* Protect against simultaneous access to the queue.  */
        TCCT_Schedule_Lock();

        /* Determine if an urgent message request is currently suspended.  */
        if (queue -> qu_urgent_list)
        {

            /* If so, copy the message from the suspended request block and
               resume the associated task.  */

            /* Decrement the number of tasks waiting on queue.  */
            queue -> qu_tasks_waiting--;

            /* Remove the first suspended block from the list.  */
            suspend_ptr =  queue -> qu_urgent_list;
            NU_Remove_From_List((CS_NODE **) &(queue -> qu_urgent_list),
                                &(suspend_ptr -> qu_suspend_link));

            /* Initialize the return status.  */
            suspend_ptr -> qu_return_status =  NU_SUCCESS;

            /* Copy the message.  */
            memcpy(message, suspend_ptr -> qu_message_area, QU_COPY_SIZE(suspend_ptr -> qu_message_size));

            /* Return the size of the message copied.  */
            *actual_size =  suspend_ptr -> qu_message_size;

            /* Trace log */
            T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_UNBLKD_CTXT);

            /* Wakeup the waiting task and check for preemption.  */
            preempt =
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> qu_suspended_task,
                                                         NU_QUEUE_SUSPEND);

            /* Trace log */
            T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_ACTION_SUCCESS);

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

        /* Determine if there are messages in the queue.  */
        else if (queue -> qu_messages)
        {

            /* Copy message from queue into the caller's area.  */

            /* Setup the source and destination pointers.  */
            source =       queue -> qu_read;
            destination =  (UNSIGNED_PTR) message;

            /* Process according to the type of message supported by the queue. */
            if (queue -> qu_fixed_size)
            {

                /* Queue supports fixed-size messages.  */

                /* Copy the message from the queue area into the destination.  */
                memcpy(destination, source, QU_COPY_SIZE(size));
                source += size;
            }
            else
            {

                /* Queue supports variable-size messages.  */

                /* Variable length message size is actually in the queue area. */
                size =  *(source++);

                /* Check for a wrap-around condition on the queue.  */
                if (source >= queue -> qu_end)
                {
                    /* Wrap the read pointer back to the top of the queue
                       area.  */
                    source =  queue -> qu_start;
                }

                /* Increment the number of available words in the queue.  */
                queue -> qu_available++;

                /* Calculate the number of words remaining from the read pointer
                   to the bottom of the queue.  */
                copy_size =  queue -> qu_end - source;

                /* Determine if the message needs to be wrapped around the
                   edge of the queue area.  */
                if (copy_size >= size)
                {

                    /* Copy the whole message at once.  */
                    memcpy(destination, source, QU_COPY_SIZE(size));
                    source += size;
                }
                else
                {

                    /* Copy the first half of the message.  */
                    memcpy(destination, source, QU_COPY_SIZE(copy_size));
                    destination += copy_size;

                    /* Copy the second half of the message.  */
                    source =  queue -> qu_start;
                    memcpy(destination, source, QU_COPY_SIZE(size - copy_size));
                    source += (size - copy_size);
                }
            }

            /* Check again for wrap-around condition on the read pointer. */
            if (source >= queue -> qu_end)
            {
                /* Move the read pointer to the top of the queue area.  */
                queue -> qu_read =  queue -> qu_start;
            }
            else
            {
                /* Move the read pointer to where the copy left off.  */
                queue -> qu_read =  source;
            }

            /* Increment the number of available words.  */
            queue -> qu_available =  queue -> qu_available + size;

            /* Decrement the number of messages in the queue.  */
            queue -> qu_messages--;

            /* Return the number of words received.  */
            *actual_size =  size;

            /* Determine if any tasks suspended on a full queue can be woken
               up.  */
            if (queue -> qu_suspension_list)
            {

                /* Overhead of each queue message.  */
                if (!queue -> qu_fixed_size)
                {
                    i =  1;
                }
                else
                {
                    i =  0;
                }

                /* Pickup the suspension list and examine suspension blocks
                   to see if the message could now fit in the queue.  */
                suspend_ptr =  queue -> qu_suspension_list;
                preempt =      NU_FALSE;
                while ((suspend_ptr) &&
                  ((suspend_ptr -> qu_message_size + i) <= queue -> qu_available))
                {

                    /* Place the suspended task's message into the queue.  */

                    /* Setup the source and destination pointers.  */
                    source =        suspend_ptr -> qu_message_area;
                    destination =   queue -> qu_write;
                    size =          suspend_ptr -> qu_message_size;

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
                            memcpy(destination, queue -> qu_start, QU_COPY_SIZE(size - copy_size));
                            destination += (size - copy_size);
                        }
                    }

                    /* Check again for wrap-around condition on the write
                       pointer. */
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

                    /* Decrement the number of tasks waiting counter.  */
                    queue -> qu_tasks_waiting--;

                    /* Remove the first suspended block from the list.  */
                    NU_Remove_From_List((CS_NODE **)
                                        &(queue -> qu_suspension_list),
                                        &(suspend_ptr -> qu_suspend_link));

                    /* Return a successful status.  */
                    suspend_ptr -> qu_return_status =  NU_SUCCESS;

                    /* Trace log */
                    T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_UNBLKD_CTXT);

                    /* Resume the suspended task.  */
                    preempt =  preempt |
                      TCC_Resume_Task((NU_TASK *) suspend_ptr -> qu_suspended_task,
                                                           NU_QUEUE_SUSPEND);

                    /* Setup suspend pointer to the head of the list.  */
                    suspend_ptr =  queue -> qu_suspension_list;

                    /* Overhead of each queue message.  */
                    if (!queue -> qu_fixed_size)
                    {
                        i =  1;
                    }
                    else
                    {
                        i =  0;
                    }

                }

                /* Trace log */
                T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_ACTION_SUCCESS);

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
                T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_ACTION_SUCCESS);
            }
        }
        else
        {

            /* Queue is empty.  Determine if the task wants to suspend.  */
            if (suspend)
            {

                /* Increment the number of tasks waiting on the queue counter. */
                queue -> qu_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
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
                T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the queue.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_QUEUE_SUSPEND,
                                            QUC_Cleanup, suspend_ptr, suspend);

                /* Pickup the status of the request.  */
                status =  suspend_ptr -> qu_return_status;
                *actual_size =  suspend_ptr -> qu_actual_size;
            }
            else
            {

                /* Return a status of NU_QUEUE_EMPTY because there are no
                   messages in the queue.  */
                status =  NU_QUEUE_EMPTY;

                /* Trace log */
                T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, status)
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
        T_Q_RECV((VOID*)queue, (VOID*)message, size, (UINT32)*actual_size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       QUC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a queue.  It is not called unless a timeout or a task
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
VOID  QUC_Cleanup(VOID *information)
{
    QU_SUSPEND      *suspend_ptr;           /* Suspension block pointer  */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (QU_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> qu_return_status =  NU_TIMEOUT;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> qu_queue) -> qu_tasks_waiting--;

    /* Unlink the suspend block from the suspension list.  */
    if ((suspend_ptr -> qu_queue) -> qu_urgent_list)
    {
       /* Unlink the suspend block from the suspension list.  */
       NU_Remove_From_List((CS_NODE **)
                           &((suspend_ptr -> qu_queue) -> qu_urgent_list),
                           &(suspend_ptr -> qu_suspend_link));
    }
    else
    {
       /* Unlink the suspend block from the suspension list.  */
       NU_Remove_From_List((CS_NODE **)
                           &((suspend_ptr -> qu_queue) -> qu_suspension_list),
                           &(suspend_ptr -> qu_suspend_link));
    }

    /* Return to user mode */
    NU_USER_MODE();
}
