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
*       pis_reset.c
*
*   COMPONENT
*
*       PI - Pipe Management
*
*   DESCRIPTION
*
*       This file contains the Reset supplemental routine for the pipe
*       management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Reset_Pipe                       Reset a pipe
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

/***********************************************************************
*
*   FUNCTION
*
*       NU_Reset_Pipe
*
*   DESCRIPTION
*
*       This function resets the specified pipe back to the original
*       state.  Any messages in the pipe are discarded.  Also, any
*       tasks currently suspended on the pipe are resumed with the
*       reset status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect against system
*                                           access
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
STATUS NU_Reset_Pipe(NU_PIPE *pipe_ptr)
{
    R1 PI_PCB       *pipe;                  /* Pipe control block ptr    */
    PI_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    PI_SUSPEND      *next_ptr;              /* Next suspend block pointer*/
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

        /* Pickup the suspended task suspension list.  */
        suspend_ptr =  pipe -> pi_suspension_list;

        /* Walk the chain task(s) currently suspended on the pipe.  */
        preempt =  0;
        while (suspend_ptr)
        {

            /* Resume the suspended task.  Insure that the status returned is
               NU_PIPE_RESET.  */
            suspend_ptr -> pi_return_status =  NU_PIPE_RESET;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (PI_SUSPEND *) (suspend_ptr -> pi_suspend_link.cs_next);

            /* Trace log */
            T_PIPE_RESET((VOID*)pipe, OBJ_UNBLKD_CTXT);

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
                /* Position the suspend pointer to the next block.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Pickup the urgent message suspension list.  */
        suspend_ptr =  pipe -> pi_urgent_list;

        /* Walk the chain task(s) currently suspended on the pipe.  */
        while (suspend_ptr)
        {

            /* Resume the suspended task.  Insure that the status returned is
               NU_PIPE_RESET.  */
            suspend_ptr -> pi_return_status =  NU_PIPE_RESET;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (PI_SUSPEND *) (suspend_ptr -> pi_suspend_link.cs_next);

            /* Trace log */
            T_PIPE_RESET((VOID*)pipe, OBJ_UNBLKD_CTXT);

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
                /* Position the suspend pointer to the next active block.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Initialize various elements of the pipe.  */
        pipe -> pi_available =             pipe -> pi_end - pipe -> pi_start;
        pipe -> pi_messages =              0;
        pipe -> pi_read =                  pipe -> pi_start;
        pipe -> pi_write =                 pipe -> pi_start;
        pipe -> pi_tasks_waiting =         0;
        pipe -> pi_suspension_list =       NU_NULL;
        pipe -> pi_urgent_list =           NU_NULL;

        /* Trace log */
        T_PIPE_RESET((VOID*)pipe, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the pipe.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_PIPE_RESET((VOID*)pipe, status);
    }

    /* Return a successful completion.  */
    return(status);
}
