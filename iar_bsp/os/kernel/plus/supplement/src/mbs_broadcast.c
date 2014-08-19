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
*       mbs_broadcast.c
*
*   COMPONENT
*
*       MB - Mailbox Management
*
*   DESCRIPTION
*
*       This file contains the supplemental broadcast routine for the
*       Mailbox management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Broadcast_To_Mailbox             Broadcast a mailbox message
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

/* Define internal component function prototypes.  */

VOID    MBC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Broadcast_To_Mailbox
*
*   DESCRIPTION
*
*       This function sends a message to all tasks currently waiting for
*       a message from the mailbox.  If no tasks are waiting, this
*       service behaves like a normal send message.
*
*   CALLED BY
*
*       Application
*       MBSE_Broadcast_To_Mailbox           Broadcast to a mailbox
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       NU_Priority_Place_On_List           Place on priority list
*       TCC_Resume_Task                     Resume a suspended task
*       TCC_Suspend_Task                    Suspend calling task
*       TCC_Task_Priority                   Priority of specified task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Current_Thread                 Pickup current thread
*                                           pointer
*       TCCT_Schedule_Lock                  Protect mailbox
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       mailbox_ptr                         Mailbox control block
*                                           pointer
*       message                             Pointer to message to send
*       suspend                             Suspension option if full
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_MAILBOX_FULL                 If mailbox is currently full
*           NU_TIMEOUT                      If timeout on service
*                                           expires
*           NU_MAILBOX_DELETED              If mailbox is deleted during
*                                           suspension
*           NU_MAILBOX_RESET                If mailbox is deleted during
*                                           suspension
*           NU_INVALID_MAILBOX              Mailbox pointer is invalid
*           NU_INVALID_POINTER              Message pointer is invalid
*           NU_INVALID_SUSPEND              Invalid suspend request
*
***********************************************************************/
STATUS NU_Broadcast_To_Mailbox(NU_MAILBOX *mailbox_ptr, VOID *message,
                               UNSIGNED suspend)
{
    R1 MB_MCB       *mailbox;               /* Mailbox control block ptr */
    MB_SUSPEND      suspend_block;          /* Allocate suspension block */
    R2 MB_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    MB_SUSPEND      *suspend_head;          /* Pointer to suspend head   */
    MB_SUSPEND      *next_suspend_ptr;      /* Get before restarting task*/
    STATUS          preempt;                /* Preemption flag           */
    R3 UNSIGNED     *source_ptr;            /* Pointer to source         */
    R4 UNSIGNED     *destination_ptr;       /* Pointer to destination    */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input mailbox pointer into internal pointer.  */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Determine if mailbox pointer is valid */
    NU_ERROR_CHECK((mailbox == NU_NULL), status, NU_INVALID_MAILBOX);

    /* Determine if mailbox pointer is valid */
    NU_ERROR_CHECK((mailbox -> mb_id != MB_MAILBOX_ID), status, NU_INVALID_MAILBOX);

    /* Determine if the message pointer is valid */
    NU_ERROR_CHECK((message == NU_NULL), status, NU_INVALID_POINTER);

    /* Verify suspension.  Only valid from a task thread */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the mailbox.  */
        TCCT_Schedule_Lock();

        /* Determine if the mailbox is empty or full.  */
        if (mailbox -> mb_message_present)
        {

            /* Mailbox already has a message.  Determine if suspension is
               required.  */
            if (suspend)
            {

                /* Suspension is requested.  */

                /* Increment the number of tasks suspended on the mailbox. */
                mailbox -> mb_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
                suspend_ptr -> mb_mailbox =                  mailbox;
                suspend_ptr -> mb_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> mb_suspend_link.cs_previous = NU_NULL;
                suspend_ptr -> mb_message_area =             (UNSIGNED *) message;
                task =                           (TC_TCB *)  TCCT_Current_Thread();
                suspend_ptr -> mb_suspended_task =           task;

                /* Determine if priority or FIFO suspension is associated with the
                   mailbox.  */
                if (mailbox -> mb_fifo_suspend)
                {

                    /* FIFO suspension is required.  Link the suspend block into
                       the list of suspended tasks on this mailbox.  */
                    NU_Place_On_List((CS_NODE **) &(mailbox ->mb_suspension_list),
                                     &(suspend_ptr -> mb_suspend_link));
                }
                else
                {

                    /* Get the priority of the current thread so the suspend block
                       can be placed in the appropriate place.  */
                    suspend_ptr -> mb_suspend_link.cs_priority =
                                                        TCC_Task_Priority(task);

                    NU_Priority_Place_On_List((CS_NODE **)
                                              &(mailbox -> mb_suspension_list),
                                              &(suspend_ptr -> mb_suspend_link));
                }

                /* Trace log */
                T_MBOX_BCAST((VOID*)mailbox, (VOID*)message, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the mailbox.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_MAILBOX_SUSPEND,
                                            MBC_Cleanup, suspend_ptr, suspend);

                /* Pickup the return status.  */
                status =  suspend_ptr -> mb_return_status;
            }
            else
            {

                /* Return a status of NU_MAILBOX_FULL because there is no
                   room in the mailbox for the message.  */
                status =  NU_MAILBOX_FULL;

                /* Trace log */
                T_MBOX_BCAST((VOID*)mailbox, (VOID*)message, suspend, status);
            }
        }
        else
        {

            /* Determine if a task is waiting on the mailbox.  */
            if (mailbox -> mb_suspension_list)
            {
                /* At least one task is waiting on mailbox for a message.  */

                /* Save off the suspension list and and then clear out the
                   mailbox suspension.  */
                suspend_head =  mailbox -> mb_suspension_list;
                mailbox -> mb_suspension_list =  NU_NULL;

                /* Loop to wakeup all of the tasks waiting on the mailbox for
                   a message.  */
                suspend_ptr =  suspend_head;
                preempt =      0;
                do
                {

                    /* Setup the source and destination pointers.  */
                    source_ptr =       (UNSIGNED *) message;
                    destination_ptr =  suspend_ptr -> mb_message_area;

                    /* Copy the message directly into the waiting task's
                       destination.  */
                    *destination_ptr =        *source_ptr;
                    *(destination_ptr + 1) =  *(source_ptr + 1);
                    *(destination_ptr + 2) =  *(source_ptr + 2);
                    *(destination_ptr + 3) =  *(source_ptr + 3);

                    /* Setup the appropriate return value.  */
                    suspend_ptr -> mb_return_status =  NU_SUCCESS;

                    /* Move the suspend pointer along to the next block. */
                    next_suspend_ptr =  (MB_SUSPEND *)
                                    suspend_ptr -> mb_suspend_link.cs_next;

                    /* Trace log */
                    T_MBOX_BCAST((VOID*)mailbox, (VOID*)message, suspend, OBJ_UNBLKD_CTXT);

                    /* Wakeup each task waiting.  */
                    preempt =  preempt |
                     TCC_Resume_Task((NU_TASK *) suspend_ptr -> mb_suspended_task,
                                                            NU_MAILBOX_SUSPEND);
                    suspend_ptr = next_suspend_ptr;

                } while (suspend_ptr != suspend_head);

                /* Clear the number of tasks waiting counter of the mailbox.  */
                mailbox -> mb_tasks_waiting =  0;

                /* Trace log */
                T_MBOX_BCAST((VOID*)mailbox, (VOID*)message, suspend, OBJ_ACTION_SUCCESS);

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

                /* Mailbox is empty and no task is waiting.  */

                /* Setup the source and destination pointers.  */
                source_ptr =       (UNSIGNED *) message;
                destination_ptr =  &(mailbox -> mb_message_area[0]);

                /* Place the message in the mailbox. */
                *destination_ptr =        *source_ptr;
                *(destination_ptr + 1) =  *(source_ptr + 1);
                *(destination_ptr + 2) =  *(source_ptr + 2);
                *(destination_ptr + 3) =  *(source_ptr + 3);

                /* Indicate that the mailbox has a message.  */
                mailbox -> mb_message_present =  NU_TRUE;

                T_MBOX_BCAST((VOID*)mailbox, (VOID*)message, suspend, OBJ_ACTION_SUCCESS);
            }
        }

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_MBOX_BCAST((VOID*)mailbox, (VOID*)message, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}
