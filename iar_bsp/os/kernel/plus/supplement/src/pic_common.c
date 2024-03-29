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
*       pic_common.c
*
*   COMPONENT
*
*       PI - Pipe Management
*
*   DESCRIPTION
*
*       This file contains the core routines for the pipe management
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Pipe                      Create a message pipe
*       NU_Delete_Pipe                      Delete a message pipe
*       NU_Send_To_Pipe                     Send message to a pipe
*       NU_Receive_From_Pipe                Receive a message from pipe
*       PIC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       common_services.h                   Common service constants
*       pipe.h                              Pipe functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/supplement/inc/pipe.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define external inner-component global data references.  */

extern CS_NODE         *PID_Created_Pipes_List;
extern UNSIGNED         PID_Total_Pipes;

/* Define internal component function prototypes.  */

VOID    PIC_Cleanup(VOID *information);

/* Internal macros */

#define PIC_OVERHEAD_CALC(message_size)     (sizeof(UNSIGNED) + (((message_size + sizeof(UNSIGNED) - 1)/sizeof(UNSIGNED)) * sizeof(UNSIGNED)) - message_size)

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Pipe
*
*   DESCRIPTION
*
*       This function creates a pipe and then places it on the list
*       of created pipes.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Add to node to linked-list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Data structure protect
*       TCCT_Schedule_Unlock                Un-protect data structure
*
*   INPUTS
*
*       pipe_ptr                            Pipe control block pointer
*       name                                Pipe name
*       start_address                       Starting address of actual
*                                           pipe area
*       pipe_size                           Total size of pipe in bytes
*       message_type                        Type of message supported by
*                                           the pipe (fixed/variable)
*       message_size                        Size of message.  Variable
*                                           message-length pipes, this
*                                           represents the maximum size
*       suspend_type                        Suspension type
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_PIPE                     Invalid pipe pointer
*       NU_INVALID_MEMORY                   Invalid pipe starting addr
*       NU_INVALID_SIZE                     Invalid pipe size and/or
*                                           size of message
*       NU_INVALID_MESSAGE                  Invalid message type
*       NU_INVALID_SUSPEND                  Invalid suspend type
*       NU_NOT_ALIGNED                      Start address is not aligned
*
***********************************************************************/
STATUS NU_Create_Pipe(NU_PIPE *pipe_ptr, CHAR *name,
                      VOID *start_address, UNSIGNED pipe_size,
                      OPTION message_type, UNSIGNED message_size,
                      OPTION suspend_type)
{
    R1 PI_PCB       *pipe;                  /* Pipe control block ptr    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pipe pointer into internal pointer.  */
    pipe =  (PI_PCB *) pipe_ptr;

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK(((pipe == NU_NULL) || (pipe -> pi_id == PI_PIPE_ID)), status, NU_INVALID_PIPE);

    /* Determine if the starting address of the pipe is invalid */
    NU_ERROR_CHECK((start_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Verify the start address is aligned */
    NU_ERROR_CHECK((ESAL_GE_MEM_ALIGNED_CHECK(start_address, sizeof(UNSIGNED)) == NU_FALSE), status, NU_NOT_ALIGNED);

    /* Determine if one or both of the size parameters are valid */
    NU_ERROR_CHECK(((pipe_size == 0) || (message_size == 0) || ((message_size + ((message_type == NU_VARIABLE_SIZE) ? PIC_OVERHEAD_CALC(message_size) : 0)) > pipe_size)), status, NU_INVALID_SIZE);

    /* Determine if the message type is valid */
    NU_ERROR_CHECK(((message_type != NU_FIXED_SIZE) && (message_type != NU_VARIABLE_SIZE)), status, NU_INVALID_MESSAGE);

    /* Determine if the suspend type is valid */
    NU_ERROR_CHECK(((suspend_type != NU_FIFO) && (suspend_type != NU_PRIORITY)), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(pipe, PI_PCB);

        /* Fill in the pipe name. */
        strncpy(pipe -> pi_name, name, (NU_MAX_NAME - 1));

        /* Setup the pipe suspension type.  */
        if (suspend_type == NU_FIFO)
        {
            /* FIFO suspension is selected, setup the flag accordingly.  */
            pipe -> pi_fifo_suspend =  NU_TRUE;
        }

        /* Setup the pipe message type.  */
        if (message_type == NU_FIXED_SIZE)
        {
            /* Fixed-size messages are required.  */
            pipe -> pi_fixed_size =  NU_TRUE;
        }

        /* Setup the message size.  */
        pipe -> pi_message_size =  message_size;

        /* Setup the actual pipe pointers.  */
        pipe -> pi_pipe_size =     pipe_size;

        /* Determine if the pipe's size needs to be adjusted.  */
        if (pipe -> pi_fixed_size)
        {
            /* The size of a fixed-size message pipe must be an even multiple of
               the actual message size.  */
            pipe_size =  (pipe_size/message_size) * message_size;
        }
        else
        {
            /* Insure that the size is in terms of UNSIGNED data elements.  This
               insures that the UNSIGNED word is never written past the end of
               the pipe.  */
            pipe_size =  (pipe_size/sizeof(UNSIGNED)) * sizeof(UNSIGNED);
        }

        pipe -> pi_available =     pipe_size;
        pipe -> pi_start =         (BYTE_PTR) start_address;
        pipe -> pi_end =           pipe -> pi_start + pipe_size;
        pipe -> pi_read =          (BYTE_PTR) start_address;
        pipe -> pi_write =         (BYTE_PTR) start_address;

        /* Protect against access to the list of created pipes.  */
        TCCT_Schedule_Lock();

        /* At this point the pipe is completely built.  The ID can now be
           set and it can be linked into the created pipe list.  */
        pipe -> pi_id =                     PI_PIPE_ID;

        /* Link the pipe into the list of created pipes and increment the
           total number of pipes in the system.  */
        NU_Place_On_List(&PID_Created_Pipes_List, &(pipe -> pi_created));
        PID_Total_Pipes++;

        /* Trace log */
        T_PIPE_CREATE((VOID*)pipe, (VOID*)start_address, name, pipe_size, message_size,
        message_type, suspend_type, OBJ_ACTION_SUCCESS);

        /* Release protection against access to the list of created pipes.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_PIPE_CREATE((VOID*)pipe, (VOID*)start_address, name, pipe_size, message_size,
        message_type, suspend_type, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Pipe
*
*   DESCRIPTION
*
*       This function deletes a pipe and removes it from the list of
*       created pipes.  All tasks suspended on the pipe are
*       resumed.  Note that this function does not free the memory
*       associated with the pipe.
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
*       pipe_ptr                            Pipe control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_PIPE                     Invalid pipe pointer
*
***********************************************************************/
STATUS NU_Delete_Pipe(NU_PIPE *pipe_ptr)
{
    R1 PI_PCB       *pipe;                  /* Pipe control block ptr    */
    PI_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    PI_SUSPEND      *next_ptr;              /* Next suspension block ptr */
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pipe pointer into internal pointer */
    pipe =  (PI_PCB *) pipe_ptr;

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK((pipe == NU_NULL), status, NU_INVALID_PIPE);

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK((pipe -> pi_id != PI_PIPE_ID), status, NU_INVALID_PIPE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against access to the pipe.  */
        TCCT_Schedule_Lock();

        /* Clear the pipe ID.  */
        pipe -> pi_id =  0;

        /* Remove the pipe from the list of created pipes.  */
        NU_Remove_From_List(&PID_Created_Pipes_List, &(pipe -> pi_created));

        /* Decrement the total number of created pipes.  */
        PID_Total_Pipes--;

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  pipe -> pi_suspension_list;

        /* Walk the chain task(s) currently suspended on the pipe.  */
        preempt =  0;
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_PIPE_DELETED.  */
            suspend_ptr -> pi_return_status =  NU_PIPE_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (PI_SUSPEND *) (suspend_ptr -> pi_suspend_link.cs_next);

            /* Trace log */
            T_PIPE_DELETE((VOID*)pipe, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> pi_suspended_task,
                                                            NU_PIPE_SUSPEND);

            /* Determine if the next is the same as the head pointer.  */
            if (next_ptr == pipe -> pi_suspension_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Setup the next suspension pointer.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Pickup the urgent message suspension list.  */
        suspend_ptr =  pipe -> pi_urgent_list;

        /* Walk the chain task(s) currently suspended on the pipe.  */
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_PIPE_DELETED.  */
            suspend_ptr -> pi_return_status =  NU_PIPE_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (PI_SUSPEND *) (suspend_ptr -> pi_suspend_link.cs_next);

            /* Trace log */
            T_PIPE_DELETE((VOID*)pipe, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> pi_suspended_task,
                                                            NU_PIPE_SUSPEND);

            /* Determine if the next is the same as the head pointer.  */
            if (next_ptr == pipe -> pi_urgent_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Position suspend pointer to the next block.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Trace log */
        T_PIPE_DELETE((VOID*)pipe, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the list of created pipes.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_PIPE_DELETE((VOID*)pipe, status);
    }

    /* Return a successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Send_To_Pipe
*
*   DESCRIPTION
*
*       This function sends a message to the specified pipe.  The
*       message length is determined by the caller.  If there are one
*       or more tasks suspended on the pipe for a message, the message
*       is copied into the message area of the first waiting task.  If
*       the task's request is satisfied, it is resumed.  Otherwise, if
*       the pipe cannot hold the message, suspension of the calling
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
STATUS NU_Send_To_Pipe(NU_PIPE *pipe_ptr, VOID *message, UNSIGNED size,
                       UNSIGNED suspend)
{
    R1 PI_PCB       *pipe;                  /* Pipe control block ptr    */
    PI_SUSPEND      suspend_block;          /* Allocate suspension block */
    PI_SUSPEND      *suspend_ptr;           /* Pointer to suspend block  */
    R2 BYTE_PTR     source;                 /* Pointer to source         */
    R3 BYTE_PTR     destination;            /* Pointer to destination    */
    UNSIGNED        copy_size;              /* Partial copy size         */
    R4 INT          i;                      /* Working counter           */
    UNSIGNED        pad =  0;               /* Number of pad bytes       */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          preempt;                /* Preempt flag              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pipe pointer into internal pointer */
    pipe =  (PI_PCB *) pipe_ptr;

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK((pipe == NU_NULL), status, NU_INVALID_PIPE);

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK((pipe -> pi_id != PI_PIPE_ID), status, NU_INVALID_PIPE);

    /* Determine if the pointer to the message is valid */
    NU_ERROR_CHECK((message == NU_NULL), status, NU_INVALID_POINTER);

    /* Determine if the message size is valid */
    NU_ERROR_CHECK((size == 0), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid */
    NU_ERROR_CHECK(((pipe -> pi_fixed_size) && (size != pipe -> pi_message_size)), status, NU_INVALID_SIZE);

    /* Determine if the message size is valid */
    NU_ERROR_CHECK(((!pipe -> pi_fixed_size) && (size > pipe -> pi_message_size)), status, NU_INVALID_SIZE);

    /* Verify suspension.  Only valid from a non-task thread */
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

            /* Calculate the number of pad bytes necessary to make keep the pipe
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
                suspend_ptr -> pi_pipe =                    pipe;
                suspend_ptr -> pi_suspend_link.cs_next =    NU_NULL;
                suspend_ptr -> pi_suspend_link.cs_previous= NU_NULL;
                suspend_ptr -> pi_message_area =            (BYTE_PTR) message;
                suspend_ptr -> pi_message_size =            size;
                task =                           (TC_TCB *) TCCT_Current_Thread();
                suspend_ptr -> pi_suspended_task =          task;

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
                T_PIPE_SEND((VOID*)pipe, (VOID*)message, size, suspend, OBJ_BLKD_CTXT);

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
                T_PIPE_SEND((VOID*)pipe, (VOID*)message, size, suspend, status);
            }
        }
        else
        {
            /* Determine if a task is waiting on an empty pipe.  */
            if ((pipe -> pi_suspension_list) && (pipe -> pi_messages == 0))
            {
                /* Task is waiting on pipe for a message.  */

                /* Decrement the number of tasks waiting on pipe.  */
                pipe -> pi_tasks_waiting--;

                /* Remove the first suspended block from the list.  */
                suspend_ptr =  pipe -> pi_suspension_list;
                NU_Remove_From_List((CS_NODE **) &(pipe -> pi_suspension_list),
                                    &(suspend_ptr -> pi_suspend_link));

                /* Initialize the return status.  */
                suspend_ptr -> pi_return_status =  NU_SUCCESS;

                /* Copy the message.  */
                memcpy(suspend_ptr -> pi_message_area, (BYTE_PTR) message, size);

                /* Return the size of the message copied.  */
                suspend_ptr -> pi_actual_size =  size;

                /* Trace log */
                T_PIPE_SEND((VOID*)pipe, (VOID*)message, size, suspend, OBJ_UNBLKD_CTXT);

                /* Wakeup the waiting task and check for preemption.  */
                preempt =
                    TCC_Resume_Task((NU_TASK *) suspend_ptr -> pi_suspended_task,
                                                                NU_PIPE_SUSPEND);

                /* Trace log */
                T_PIPE_SEND((VOID*)pipe, (VOID*)message, size, suspend, OBJ_ACTION_SUCCESS);

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
                T_PIPE_SEND((VOID*)pipe, (VOID*)message, size, suspend, OBJ_ACTION_SUCCESS);
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
        T_PIPE_SEND((VOID*)pipe, (VOID*)message, size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Receive_From_Pipe
*
*   DESCRIPTION
*
*       This function receives a message from the specified pipe.  The
*       size of the message is specified by the caller.  If there is a
*       message currently in the pipe, the message is removed from the
*       pipe and placed in the caller's area.  Suspension is possible
*       if the request cannot be satisfied.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       NU_Priority_Place_On_List           Places the specified node
*                                           after all other nodes on
*                                           the list of equal or greater
*                                           priority.  Note that lower
*                                           numerical values indicate
*                                           greater priority.
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
*       size                                Size of the message
*       actual_size                         Size of message received
*       suspend                             Suspension option if empty
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_PIPE_EMPTY                   If pipe is currently empty
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
STATUS NU_Receive_From_Pipe(NU_PIPE *pipe_ptr, VOID *message,
                            UNSIGNED size, UNSIGNED *actual_size,
                            UNSIGNED suspend)
{
    R1 PI_PCB       *pipe;                  /* Pipe control block ptr    */
    PI_SUSPEND      suspend_block;          /* Allocate suspension block */
    PI_SUSPEND      *suspend_ptr;           /* Pointer to suspend block  */
    R2 BYTE_PTR     source;                 /* Pointer to source         */
    R3 BYTE_PTR     destination;            /* Pointer to destination    */
    TC_TCB          *task;                  /* Task pointer              */
    UNSIGNED        copy_size;              /* Number of bytes to copy   */
    UNSIGNED        pad =  0;               /* Number of pad bytes       */
    R4 INT          i;                      /* Working counter           */
    STATUS          preempt;                /* Preemption flag           */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pipe pointer into internal pointer */
    pipe =  (PI_PCB *) pipe_ptr;

    /* Determine if there is an error with the pipe pointer */
    NU_ERROR_CHECK((pipe == NU_NULL), status, NU_INVALID_PIPE);

    /* Determine if there is an error with the pipe pointer */
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

        /* Determine if an urgent message request is currently suspended.  */
        if (pipe -> pi_urgent_list)
        {
            /* If so, copy the message from the suspended request block and
               resume the associated task.  */

            /* Decrement the number of tasks waiting on pipe.  */
            pipe -> pi_tasks_waiting--;

            /* Remove the first suspended block from the list.  */
            suspend_ptr =  pipe -> pi_urgent_list;
            NU_Remove_From_List((CS_NODE **) &(pipe -> pi_urgent_list),
                                &(suspend_ptr -> pi_suspend_link));

            /* Initialize the return status.  */
            suspend_ptr -> pi_return_status =  NU_SUCCESS;

            /* Copy the message.  */
            memcpy((BYTE_PTR) message, suspend_ptr -> pi_message_area, size);

            /* Return the size of the message copied.  */
            *actual_size =  suspend_ptr -> pi_message_size;

            /* Trace log */
            T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_UNBLKD_CTXT);

            /* Wakeup the waiting task and check for preemption.  */
            preempt =
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> pi_suspended_task,
                                                         NU_PIPE_SUSPEND);

            /* Trace log */
            T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_ACTION_SUCCESS);

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

        /* Determine if there are messages in the pipe.  */
        else if (pipe -> pi_messages)
        {
            /* Copy message from pipe into the caller's area.  */

            /* Setup the source and destination pointers.  */
            destination =  (BYTE_PTR) message;
            source =       pipe -> pi_read;

            /* Process according to the type of message supported by the pipe.  */
            if (pipe -> pi_fixed_size)
            {
                /* Pipe supports fixed-size messages.  */
                memcpy(destination, source, size);
                source += size;
            }
            else
            {
                /* Pipe supports variable-size messages.  */

                /* Variable length message size is actually in the pipe area. */
                size =  *((UNSIGNED *) source);
                source =  source + sizeof(UNSIGNED);

                /* Check for a wrap-around condition on the pipe.  */
                if (source >= pipe -> pi_end)
                {
                    /* Wrap the read pointer back to the top of the pipe
                       area.  */
                    source =  pipe -> pi_start;
                }
                /* Increment the number of available bytes in the pipe.  */
                pipe -> pi_available =  pipe -> pi_available + sizeof(UNSIGNED);

                /* Calculate the number of pad bytes necessary to keep
                   the pipe read pointer on an UNSIGNED data element alignment.*/
                pad =  (((size + sizeof(UNSIGNED) - 1)/sizeof(UNSIGNED)) *
                        sizeof(UNSIGNED)) - size;

                /* Calculate the number of bytes remaining from the read pointer
                   to the bottom of the pipe.  */
                copy_size =  pipe -> pi_end - source;

                /* Determine if the message needs to be wrapped around the
                   edge of the pipe area.  */
                if (copy_size >= size)
                {
                    /* Copy the whole message at once.  */
                    memcpy(destination, source, size);
                    source += size;
                }
                else
                {
                    /* Copy the first half of the message.  */
                    memcpy(destination, source, copy_size);
                    destination += copy_size;

                    /* Copy the second half of the message.  */
                    source =  pipe -> pi_start;
                    memcpy(destination, source, (size - copy_size));
                    source += (size - copy_size);
                }
            }

            /* Check again for wrap-around condition on the read pointer. */
            if (source >= pipe -> pi_end)
            {
                /* Move the read pointer to the top of the pipe area.  */
                source =  pipe -> pi_start;
            }
            /* Determine if the pipe supports variable-length messages.  If
               so, pad bytes are needed to keep UNSIGNED alignment.  */
            if (pad)
            {
                /* Variable-size message.  Add pad bytes to the read
                   pointer.  */

                /* Calculate the number of bytes remaining from the read
                   pointer to the bottom of the pipe.  */
                copy_size =  pipe -> pi_end - source;

                /* If there is not enough room at the bottom of the pipe, the
                   pad bytes must be wrapped around to the top.  */
                if (copy_size <= pad)
                {
                    /* Move read pointer to the top of the pipe and make the
                       necessary adjustment.  */
                    source =  pipe -> pi_start + (pad - copy_size);
                }
                else
                {
                    /* There is enough room in the pipe to simply add the
                       the pad bytes to the read pointer.  */
                    source =  source + pad;
                }
                /* Add pad bytes to the available bytes count.  */
                pipe -> pi_available =  pipe -> pi_available + pad;
            }

            /* Adjust the actual read pointer.  */
            pipe -> pi_read =  source;

            /* Increment the number of available bytes.  */
            pipe -> pi_available =  pipe -> pi_available + size;

            /* Decrement the number of messages in the pipe.  */
            pipe -> pi_messages--;

            /* Return the number of bytes received.  */
            *actual_size =  size;

            /* Determine if any tasks suspended on a full pipe can be woken
               up.  */
            if (pipe -> pi_suspension_list)
            {
                /* Pickup the suspension list and examine suspension blocks
                   to see if the message could now fit in the pipe.  */
                suspend_ptr =  pipe -> pi_suspension_list;
                preempt =      NU_FALSE;
                size =         suspend_ptr -> pi_message_size;
                i =            0;
                pad =          0;

                /* Overhead of each pipe message.  */
                if (!pipe -> pi_fixed_size)
                {
                    /* Variable messages have one additional word of overhead.  */
                    i =  sizeof(UNSIGNED);

                    /* Calculate the number of pad bytes necessary to keep
                       the pipe write pointer on an UNSIGNED data element
                       alignment.  */
                    pad =  (((size + sizeof(UNSIGNED) - 1)/sizeof(UNSIGNED)) *
                                            sizeof(UNSIGNED)) - size;

                    /* Insure that padding is included in the overhead.  */
                    i =  i + ((INT) pad);
                }

                while ((suspend_ptr) && ((size + i) <= pipe -> pi_available))
                {
                    /* Place the suspended task's message into the pipe.  */

                    /* Setup the source and destination pointers.  */
                    source =       suspend_ptr -> pi_message_area;
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

                    /* Check again for wrap-around condition on the write
                       pointer. */
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

                        /* If there is not enough room at the bottom of the pipe,
                           the pad bytes must be wrapped around to the top.  */
                        if (copy_size <= pad)
                        {
                            /* Move write pointer to the top of the pipe and make
                               the necessary adjustment.  */
                            destination =  pipe -> pi_start + (pad - copy_size);
                        }
                        else
                        {
                            /* There is enough room in the pipe to simply add
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

                    /* Decrement the number of tasks waiting counter.  */
                    pipe -> pi_tasks_waiting--;

                    /* Remove the first suspended block from the list.  */
                    NU_Remove_From_List((CS_NODE **)
                                        &(pipe -> pi_suspension_list),
                                        &(suspend_ptr -> pi_suspend_link));

                    /* Return a successful status.  */
                    suspend_ptr -> pi_return_status =  NU_SUCCESS;

                    /* Trace log */
                    T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_UNBLKD_CTXT);

                    /* Resume the suspended task.  */
                    preempt =  preempt |
                     TCC_Resume_Task((NU_TASK *) suspend_ptr -> pi_suspended_task,
                                                           NU_PIPE_SUSPEND);

                    /* Setup suspend pointer to the head of the list.  */
                    suspend_ptr =  pipe -> pi_suspension_list;

                    /* Determine if there really is another suspended block.  If
                       there is and the pipe supports variable length messages,
                       calculate new size and padding parameters.  */
                    if ((suspend_ptr) && (!pipe -> pi_fixed_size))
                    {

                        /* Get the next message size.  */
                        size =   suspend_ptr -> pi_message_size;

                        /* Variable messages have one additional word of
                           overhead.  */
                        i =  sizeof(UNSIGNED);

                        /* Calculate the number of pad bytes necessary to
                           keep the pipe write pointer on an UNSIGNED data element
                           alignment.  */
                        pad =  (((size + sizeof(UNSIGNED) - 1)/sizeof(UNSIGNED)) *
                                              sizeof(UNSIGNED)) - size;

                        /* Insure that padding is included in the overhead.  */
                        i =  i + ((INT) pad);
                    }
                }

                /* Trace log */
                T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_ACTION_SUCCESS);

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
                T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_ACTION_SUCCESS);
            }
        }
        else
        {

            /* pipe is empty.  Determine if the task wants to suspend.  */
            if (suspend)
            {

                /* Increment the number of tasks waiting on the pipe counter. */
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

                /* Zero the actual size to ensure a timeout does not report
                   incorrect actual size. */
                suspend_ptr -> pi_actual_size = 0;

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
                T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the pipe.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_PIPE_SUSPEND,
                                            PIC_Cleanup, suspend_ptr, suspend);

                /* Pickup the status of the request.  */
                status =  suspend_ptr -> pi_return_status;
                *actual_size =  suspend_ptr -> pi_actual_size;
            }
            else
            {

                /* Return a status of NU_PIPE_EMPTY because there are no
                   messages in the pipe.  */
                status =  NU_PIPE_EMPTY;

                /* Trace log */
                T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, status);
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
        T_PIPE_RECV((VOID*)pipe, (VOID*)message, size, (UINT32)*actual_size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       PIC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a pipe.  It is not called unless a timeout or a task
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
VOID  PIC_Cleanup(VOID *information)
{
    PI_SUSPEND      *suspend_ptr;           /* Suspension block pointer  */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (PI_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> pi_return_status =  NU_TIMEOUT;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> pi_pipe) -> pi_tasks_waiting--;

    /* Determine if the suspend block is one the urgent list. */
    if ((suspend_ptr -> pi_pipe) -> pi_urgent_list)
    {
        /* Unlink the suspend block from the urgent list.  */
        NU_Remove_From_List((CS_NODE **)
                            &((suspend_ptr -> pi_pipe) -> pi_urgent_list),
                            &(suspend_ptr -> pi_suspend_link));
    }
    else
    {
        /* Unlink the suspend block from the suspension list.  */
        NU_Remove_From_List((CS_NODE **)
                            &((suspend_ptr -> pi_pipe) -> pi_suspension_list),
                            &(suspend_ptr -> pi_suspend_link));
    }

    /* Return to user mode */
    NU_USER_MODE();
}
