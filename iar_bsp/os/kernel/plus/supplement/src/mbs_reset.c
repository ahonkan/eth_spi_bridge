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
*       mbs_reset.c
*
*   COMPONENT
*
*       MB - Mailbox Management
*
*   DESCRIPTION
*
*       This file contains the supplemental reset routine for the
*       Mailbox management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Reset_Mailbox                    Reset a mailbox
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       mailbox.h                           Mailbox functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/supplement/inc/mailbox.h"
#include        "services/nu_trace_os_mark.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Reset_Mailbox
*
*   DESCRIPTION
*
*       This function resets a mailbox back to the initial state.  Any
*       message in the mailbox is discarded.  Also, all tasks suspended
*       on the mailbox are resumed with the reset completion status.
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
*       TCCT_Schedule_Lock                  Protect mailbox
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       mailbox_ptr                         Mailbox control block
*                                           pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_MAILBOX                  Invalid mailbox supplied
*
***********************************************************************/
STATUS NU_Reset_Mailbox(NU_MAILBOX *mailbox_ptr)
{
    R1 MB_MCB       *mailbox;               /* Mailbox control block ptr */
    R2 MB_SUSPEND   *suspend_ptr;           /* Suspend block pointer     */
    MB_SUSPEND      *next_ptr;              /* Next suspend block pointer*/
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input mailbox pointer into internal pointer.  */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Determine if the mailbox pointer is valid.  */
    NU_ERROR_CHECK(((mailbox == NU_NULL) || (mailbox -> mb_id != MB_MAILBOX_ID)), status, NU_INVALID_MAILBOX);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against access to the mailbox.  */
        TCCT_Schedule_Lock();

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  mailbox -> mb_suspension_list;

        /* Walk the chain task(s) currently suspended on the mailbox.  */
        preempt =  0;
        while (suspend_ptr)
        {

            /* Resume the suspended task.  Insure that the status returned is
               NU_MAILBOX_RESET.  */
            suspend_ptr -> mb_return_status =  NU_MAILBOX_RESET;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (MB_SUSPEND *) (suspend_ptr -> mb_suspend_link.cs_next);

            /* Trace log */
            T_MBOX_RESET((VOID*)mailbox, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                    TCC_Resume_Task((NU_TASK *) suspend_ptr -> mb_suspended_task,
                                                            NU_MAILBOX_SUSPEND);

            /* Determine if the next is the same as the head pointer.  */
            if (next_ptr == mailbox -> mb_suspension_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Position the suspend pointer to the next suspend block.  */
                suspend_ptr =  next_ptr;
            }

        }

        /* Initialize the mailbox.  */
        mailbox -> mb_message_present =  NU_FALSE;
        mailbox -> mb_tasks_waiting =    0;
        mailbox -> mb_suspension_list =  NU_NULL;

        /* Trace log */
        T_MBOX_RESET((VOID*)mailbox, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_MBOX_RESET((VOID*)mailbox, status);
    }

    /* Return a successful completion.  */
    return(status);
}
