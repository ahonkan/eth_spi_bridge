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
*       pif_info.c
*
*   COMPONENT
*
*       PI - Pipe Management
*
*   DESCRIPTION
*
*       This file contains the Information routine to obtain facts about
*       the Pipe management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Pipe_Information                 Retrieve pipe information
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
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Pipe_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified pipe.
*       However, if the supplied pipe pointer is invalid, the
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
*       TCCT_Schedule_Lock                  Protect pipe
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pipe_ptr                            Pointer to the pipe
*       name                                Destination for the name
*       start_address                       Destination for the start
*                                           address of the pipe
*       pipe_size                           Destination for pipe size
*       available                           Destination for available
*                                           room in pipe
*       messages                            Destination for number of
*                                           messages piped
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
*           NU_SUCCESS                      If a valid pipe pointer
*                                           is supplied
*           NU_INVALID_PIPE                 If pipe pointer invalid
*
***********************************************************************/
STATUS NU_Pipe_Information(NU_PIPE *pipe_ptr, CHAR *name,
                           VOID **start_address, UNSIGNED *pipe_size,
                           UNSIGNED *available, UNSIGNED *messages,
                           OPTION *message_type, UNSIGNED *message_size,
                           OPTION *suspend_type, UNSIGNED *tasks_waiting,
                           NU_TASK **first_task)
{
    PI_PCB          *pipe;                  /* Pipe control block ptr    */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input pipe pointer into internal pointer.  */
    pipe =  (PI_PCB *) pipe_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this pipe id is valid.  */
    if ((pipe != NU_NULL) && (pipe -> pi_id == PI_PIPE_ID))
    {
        /* Setup protection of the pipe.  */
        TCCT_Schedule_Lock();

        /* The pipe pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the pipe's name.  */
        strncpy(name, pipe -> pi_name, NU_MAX_NAME);

        /* Determine the suspension type.  */
        if (pipe -> pi_fifo_suspend)
        {
            *suspend_type =  NU_FIFO;
        }
        else
        {
            *suspend_type =  NU_PRIORITY;
        }

        /* Determine the message type.  */
        if (pipe -> pi_fixed_size)
        {
            *message_type =  NU_FIXED_SIZE;
        }
        else
        {
            *message_type =  NU_VARIABLE_SIZE;
        }

        /* Get various information about the pipe.  */
        *start_address =  (VOID *) pipe -> pi_start;
        *pipe_size =      pipe -> pi_pipe_size;
        *available =      pipe -> pi_available;
        *messages =       pipe -> pi_messages;
        *message_size =   pipe -> pi_message_size;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  pipe -> pi_tasks_waiting;
        if (pipe -> pi_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task =  (NU_TASK *)
                (pipe -> pi_suspension_list) -> pi_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection of the pipe.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the pipe pointer is invalid.   */
        completion =  NU_INVALID_PIPE;
    }
    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
