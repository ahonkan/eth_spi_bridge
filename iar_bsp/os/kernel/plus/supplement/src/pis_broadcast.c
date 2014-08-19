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
*       pis_broadcast.c
*
*   COMPONENT
*
*       PI - Pipe Management
*
*   DESCRIPTION
*
*       This file contains the Broadcast supplemental routines for the
*       pipe management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Broadcast_To_Pipe                Broadcast a message to pipe
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       pipe.h                              Pipe functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/supplement/inc/pipe.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define internal component function prototypes.  */

VOID    PIC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Broadcast_To_Pipe
*
*   DESCRIPTION
*
*       This function sends a message to all tasks waiting for a message
*       from the specified pipe.  If there are no tasks waiting for a
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
*       TCCT_Schedule_Lock                  Protect pipe
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pipe_ptr                            Pipe control block pointer
*       message                             Pointer to message to send
*       size                                Size of message to send
*       suspend                             Suspension option if full
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_PIPE_FULL                    If pipe is currently full
*           NU_TIMEOUT                      If timeout on service
*                                           expires
*           NU_PIPE_DELETED                 If pipe was deleted during
*                                           suspension
*           NU_PIPE_RESET                   If pipe was reset during
*                                           suspension
*           NU_INVALID_PIPE                 Invalid pipe pointer
*           NU_INVALID_POINTER              Invalid message pointer
*           NU_INVALID_SIZE                 Invalid message size
*           NU_INVALID_SUSPEND              Invalid suspend request
*
***********************************************************************/
STATUS NU_Broadcast_To_Pipe(NU_PIPE *pipe_ptr, VOID *message,
                            UNSIGNED size, UNSIGNED suspend)
{
    R1 PI_PCB       *pipe;                  /* Pipe control block ptr    */
    PI_SUSPEND      suspend_block;          /* Allocate suspension block */
    PI_SUSPEND      *suspend_ptr;           /* Pointer to suspend block  */
    R2 BYTE_PTR     source;                 /* Pointer to source         */
    R3 BYTE_PTR     destination;            /* Pointer to destination    */
    UNSIGNED        copy_size;              /* Partial copy size         */
    UNSIGNED        pad =  0;               /* Number of pad bytes       */
    R4 INT          i;                      /* Working counter           */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          preempt;                /* Preempt flag              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pipe pointer into internal pointer */
    pipe =  (PI_PCB *) pipe_ptr;

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK((pipe == NU_NULL), status, NU_INVALID_PIPE);

    /* Determine if the pipe pointer is valid */
    NU_ERROR_CHECK((pipe -> pi_id != PI_PIPE_ID), status, NU_INVALID_PIPE);

    /* Determine if the pointer to the message is valid */
    NU_ERROR_CHECK((message == NU_NULL), status, NU_INVALID_POINTER);

    /* Determine if the message size is valid */
    NU_ERROR_CHECK((size == 0), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid */
    NU_ERROR_CHECK(((pipe -> pi_fixed_size) && (size != pipe -> pi_message_size)), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid */
    NU_ERROR_CHECK(((!pipe -> pi_fixed_size) && (size > pipe -> pi_message_size)), status, NU_INVALID_SIZE);

    /* Verify suspension.  Only valid from a non-task thread.  */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the pipe.  */
        TCCT_Schedule_Lock();

        /* Determine if an extra word of overhead needs to be added to the
           calculation.  */
        if (pipe -> pi_fixed_size)
        {
            /* No overhead.  */
            i =  0;
        }
        else
        {
            /* Variable messages have one additional word of overhead.  */
            i =  sizeof(UNSIGNED);

            /* Calculate the number of pad bytes necessary to keep the pipe
               write pointer on an UNSIGNED data element alignment.  */
            pad =  (((size + sizeof(UNSIGNED) - 1)/sizeof(UNSIGNED)) *
                    sizeof(UNSIGNED)) - size;

            /* Insure that padding is included in the overhead.  */
            i =  i + ((INT) pad);

            /* Make special check to see if a suspension needs to be
               forced for a variable length message.  */
            if ((pipe -> pi_suspension_list) && (pipe -> pi_messages))
            {
                /* Pickup task control block pointer.  */
                task =  (TC_TCB *) TCCT_Current_Thread();

                /* Now we know that there are other task(s) are suspended trying
                   to send a variable length message.  Determine whether or not
                   a suspension should be forced.  */
                if ((pipe -> pi_fifo_suspend) ||
                    (suspend == NU_NO_SUSPEND) ||
                    ((pipe -> pi_suspension_list) -> pi_suspend_link.cs_priority <=
                                                        TCC_Task_Priority(task)))
                {
                    /* Bump the computed size to avoid placing the new variable
                       length message ahead of the suspended tasks.  */
                    i =  (INT) pipe -> pi_available;
                }
            }
        }

        /* Determine if there is enough room in the pipe for the message.  */
        if (pipe -> pi_available < (size + i))
        {
            /* pipe does not have room for the message.  Determine if
               suspension is required.  */
            if (suspend)
            {
                /* Suspension is requested.   */

                /* Increment the number of tasks waiting.  */
                pipe -> pi_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
                suspend_ptr -> pi_pipe =                     pipe;
                suspend_ptr -> pi_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> pi_suspend_link.cs_previous = NU_NULL;
                suspend_ptr -> pi_message_area =             (BYTE_PTR) message;
                suspend_ptr -> pi_message_size =             size;
                task =                            (TC_TCB *) TCCT_Current_Thread();
                suspend_ptr -> pi_suspended_task =           task;

                /* Determine if priority or FIFO suspension is associated with the
                   pipe.  */
                if (pipe -> pi_fifo_suspend)
                {
                    /* FIFO suspension is required.  Link the suspend block into
                       the list of suspended tasks on this pipe.  */
                    NU_Place_On_List((CS_NODE **) &(pipe -> pi_suspension_list),
                                     &(suspend_ptr -> pi_suspend_link));
                }
                else
                {
                    /* Get the priority of the current thread so the suspend block
                       can be placed in the appropriate place.  */
                    suspend_ptr -> pi_suspend_link.cs_priority =
                                                        TCC_Task_Priority(task);

                    NU_Priority_Place_On_List((CS_NODE **)
                                              &(pipe -> pi_suspension_list),
                                              &(suspend_ptr -> pi_suspend_link));
                }

                /* Trace log */
                T_PIPE_BCAST((VOID*)pipe, (VOID*)message, size, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the pipe.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_PIPE_SUSPEND,
                                            PIC_Cleanup, suspend_ptr, suspend);

                /* Pickup the return status.  */
                status =  suspend_ptr -> pi_return_status;
            }
            else
            {
                /* Return a status of NU_PIPE_FULL because there is no
                   room in the pipe for the message.  */
                status =  NU_PIPE_FULL;

                /* Trace log */
                T_PIPE_BCAST((VOID*)pipe, (VOID*)message, size, suspend, status);
            }
        }
        else
        {
            /* Determine if a task is waiting on an empty pipe.  */
            if ((pipe -> pi_suspension_list) && (pipe -> pi_messages == 0))
            {
                /* Yes, one or more tasks are waiting for a message from this
                   pipe.  */
                preempt =  0;
                do
                {
                    /* Decrement the number of tasks waiting on pipe.  */
                    pipe -> pi_tasks_waiting--;

                    /* Remove the first suspended block from the list.  */
                    suspend_ptr =  pipe -> pi_suspension_list;
                    NU_Remove_From_List((CS_NODE **)
                                        &(pipe -> pi_suspension_list),
                                        &(suspend_ptr -> pi_suspend_link));

                    /* Initialize the return status.  */
                    suspend_ptr -> pi_return_status =  NU_SUCCESS;

                    /* Copy the message.  */
                    memcpy(suspend_ptr -> pi_message_area, (BYTE_PTR) message, size);

                    /* Return the size of the message copied.  */
                    suspend_ptr -> pi_actual_size =  size;

                    /* Trace log */
                    T_PIPE_BCAST((VOID*)pipe, (VOID*)message, size, suspend, OBJ_UNBLKD_CTXT);

                    /* Wakeup the waiting task and check for preemption.  */
                    preempt =  preempt |
                     TCC_Resume_Task((NU_TASK *) suspend_ptr -> pi_suspended_task,
                                                             NU_PIPE_SUSPEND);

                    /* Move the suspend pointer to the next node, which is now
                       at the head of the list.  */
                    suspend_ptr =  pipe -> pi_suspension_list;
                } while (suspend_ptr);

                /* Trace log */
                T_PIPE_BCAST((VOID*)pipe, (VOID*)message, size, suspend, OBJ_ACTION_SUCCESS);

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
                /* There is enough room in the pipe and no task is waiting.  */

                /* Setup the source pointer.  */
                source =       (BYTE_PTR) message;
                destination =  pipe -> pi_write;

                /* Process according to the type of message supported.  */
                if (pipe -> pi_fixed_size)
                {
                    /* Fixed-size messages are supported by this pipe.  */
                    memcpy(destination, source, size);
                    destination += size;
                }
                else
                {
                    /* Variable-size messages are supported.  Processing must
                       check for pipe wrap-around conditions.  */

                    /* Place message size in first location.  */
                    *((UNSIGNED *) destination) =  size;
                    destination =  destination + sizeof(UNSIGNED);

                    /* Check for a wrap-around condition on the pipe.  */
                    if (destination >= pipe -> pi_end)
                    {
                        /* Wrap the write pointer back to the top of the pipe
                           area.  */
                        destination =  pipe -> pi_start;
                    }

                    /* Decrement the number of bytes remaining for this
                       extra word of overhead.  */
                    pipe -> pi_available =  pipe -> pi_available -
                                                            sizeof(UNSIGNED);

                    /* Calculate the number of bytes remaining from the write
                       pointer to the bottom of the pipe.  */
                    copy_size =  pipe -> pi_end - destination;

                    /* Determine if the message needs to be wrapped around the
                       edge of the pipe area.  */
                    if (copy_size >= size)
                    {
                        /* Copy the whole message at once.  */
                        memcpy(destination, source, size);
                        destination += size;
                    }
                    else
                    {
                        /* Copy the first half of the message.  */
                        memcpy(destination, source, copy_size);
                        source += copy_size;

                        /* Copy the second half of the message.  */
                        destination =  pipe -> pi_start;
                        memcpy(destination, source, (size - copy_size));
                        destination += (size - copy_size);
                    }
                }

                /* Check again for wrap-around condition on the write pointer. */
                if (destination >= pipe -> pi_end)
                {
                    /* Move the write pointer to the top of the pipe area.  */
                    destination =  pipe -> pi_start;
                }

                /* Determine if the pipe supports variable-length messages.  If
                   so, pad bytes are needed to keep UNSIGNED alignment.  */
                if (pad)
                {

                    /* Variable-size message.  Add pad bytes to the write
                       pointer.  */

                    /* Calculate the number of bytes remaining from the write
                       pointer to the bottom of the pipe.  */
                    copy_size =  pipe -> pi_end - destination;

                    /* If there is not enough room at the bottom of the pipe, the
                       pad bytes must be wrapped around to the top.  */
                    if (copy_size <= pad)
                    {
                        /* Move write pointer to the top of the pipe and make the
                           necessary adjustment.  */
                        destination =  pipe -> pi_start + (pad - copy_size);
                    }
                    else
                    {
                        /* There is enough room in the pipe to simply add the
                           the pad bytes to the write pointer.  */
                        destination =  destination + pad;
                    }

                    /* Decrement the number of available bytes.  */
                    pipe -> pi_available =  pipe -> pi_available - pad;
                }

                /* Update the actual write pointer.  */
                pipe -> pi_write =  destination;

                /* Decrement the number of available bytes.  */
                pipe -> pi_available =  pipe -> pi_available - size;

                /* Increment the number of messages in the pipe.  */
                pipe -> pi_messages++;

                /* Trace log */
                T_PIPE_BCAST((VOID*)pipe, (VOID*)message, size, suspend, OBJ_ACTION_SUCCESS);
            }
        }

        /* Release protection against access to the pipe.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_PIPE_BCAST((VOID*)pipe, (VOID*)message, size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}
