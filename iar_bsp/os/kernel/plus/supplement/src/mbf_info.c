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
*       mbf_info.c
*
*   COMPONENT
*
*       MB - Mailbox Management
*
*   DESCRIPTION
*
*       This file contains the Information routine to obtain facts about
*       the Mailbox management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Mailbox_Information              Retrieve mailbox information
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
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Mailbox_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified mailbox.
*       However, if the supplied mailbox pointer is invalid, the
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
*       TCCT_Schedule_Lock                  Protect mailbox
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       mailbox_ptr                         Pointer to the mailbox
*       name                                Destination for the name
*       suspend_type                        Destination for the type of
*                                           suspension
*       message_present                     Destination for the message
*                                           present flag
*       tasks_waiting                       Destination for the tasks
*                                           waiting count
*       first_task                          Destination for the pointer
*                                           to the first task waiting
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid mailbox pointer
*                                           is supplied
*           NU_INVALID_MAILBOX              If mailbox pointer invalid
*
***********************************************************************/
STATUS NU_Mailbox_Information(NU_MAILBOX *mailbox_ptr, CHAR *name,
                              OPTION *suspend_type,
                              DATA_ELEMENT *message_present,
                              UNSIGNED *tasks_waiting, NU_TASK **first_task)
{
    MB_MCB          *mailbox;               /* Mailbox control block ptr */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input mailbox pointer into internal pointer.  */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this mailbox id is valid.  */
    if ((mailbox != NU_NULL) && (mailbox -> mb_id == MB_MAILBOX_ID))
    {
        /* Setup protection of the mailbox.  */
        TCCT_Schedule_Lock();

        /* The mailbox pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the mailbox's name.  */
        strncpy(name, mailbox -> mb_name, NU_MAX_NAME);

        /* Determine the suspension type.  */
        if (mailbox -> mb_fifo_suspend)
        {
            *suspend_type =          NU_FIFO;
        }
        else
        {
            *suspend_type =          NU_PRIORITY;
        }

        /* Indicate whether or not there is a message in the mailbox.  */
        *message_present =  mailbox -> mb_message_present;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  mailbox -> mb_tasks_waiting;
        if (mailbox -> mb_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task = (NU_TASK *)
                (mailbox -> mb_suspension_list) -> mb_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the mailbox pointer is invalid.   */
        completion =  NU_INVALID_MAILBOX;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
