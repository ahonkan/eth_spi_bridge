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
*       mbc_common.c
*
*   COMPONENT
*
*       MB - Mailbox Management
*
*   DESCRIPTION
*
*       This file contains the core routines for the Mailbox management
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Mailbox                   Create a mailbox
*       NU_Delete_Mailbox                   Delete a mailbox
*       NU_Send_To_Mailbox                  Send a mailbox message
*       NU_Receive_From_Mailbox             Receive a mailbox message
*       MBC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       common_services.h                   Common service constants
*       thread_control.h                    Thread Control functions
*       mailbox.h                           Mailbox functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/supplement/inc/mailbox.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define external inner-component global data references.  */

extern CS_NODE         *MBD_Created_Mailboxes_List;
extern UNSIGNED         MBD_Total_Mailboxes;

/* Define internal component function prototypes.  */

VOID    MBC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Mailbox
*
*   DESCRIPTION
*
*       This function creates a mailbox and then places it on the list
*       of created mailboxes.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Add node to linked-list
*                                           (conditionally compiled)
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Data structure protect
*       TCCT_Schedule_Unlock                Un-protect data structure
*
*   INPUTS
*
*       mailbox_ptr                         Mailbox control block
*                                           pointer
*       name                                Mailbox name
*       suspend_type                        Suspension type
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_MAILBOX                  Mailbox pointer is NULL
*       NU_INVALID_SUSPEND                  Suspension type is invalid
*
***********************************************************************/
STATUS NU_Create_Mailbox(NU_MAILBOX *mailbox_ptr, CHAR *name,
                         OPTION suspend_type)
{
    R1 MB_MCB       *mailbox;               /* Mailbox control block ptr */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input mailbox pointer into internal pointer.  */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Check for a NULL mailbox pointer or an already created mailbox */
    NU_ERROR_CHECK(((mailbox == NU_NULL) || (mailbox -> mb_id == MB_MAILBOX_ID)), status, NU_INVALID_MAILBOX);

    /* Verify valid suspension type */
    NU_ERROR_CHECK(((suspend_type != NU_FIFO) && (suspend_type != NU_PRIORITY)), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(mailbox, MB_MCB);

        /* Fill in the mailbox name. */
        strncpy(mailbox -> mb_name, name, (NU_MAX_NAME - 1));

        /* Setup the mailbox suspension type.  */
        if (suspend_type == NU_FIFO)
        {
            /* FIFO suspension is selected, setup the flag accordingly.  */
            mailbox -> mb_fifo_suspend =  NU_TRUE;
        }

        /* Protect against access to the list of created mailboxes.  */
        TCCT_Schedule_Lock();

        /* At this point the mailbox is completely built.  The ID can now be
           set and it can be linked into the created mailbox list.  */
        mailbox -> mb_id =                     MB_MAILBOX_ID;

        /* Link the mailbox into the list of created mailboxes and increment the
           total number of mailboxes in the system.  */
        NU_Place_On_List(&MBD_Created_Mailboxes_List, &(mailbox -> mb_created));
        MBD_Total_Mailboxes++;

        /* Trace log */
        T_MBOX_CREATE((VOID*)mailbox, (VOID*)name, suspend_type, OBJ_ACTION_SUCCESS);

        /* Release protection against access to the list of created mailboxes.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_MBOX_CREATE((VOID*)mailbox, (VOID*)name, suspend_type, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Mailbox
*
*   DESCRIPTION
*
*       This function deletes a mailbox and removes it from the list of
*       created mailboxes.  All tasks suspended on the mailbox are
*       resumed.  Note that this function does not free the memory
*       associated with the mailbox control block.
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
*       mailbox_ptr                         Mailbox control block
*                                           pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_MAILBOX                  Invalid mailbox supplied
*
***********************************************************************/
STATUS NU_Delete_Mailbox(NU_MAILBOX *mailbox_ptr)
{
    R1 MB_MCB       *mailbox;               /* Mailbox control block ptr */
    MB_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    MB_SUSPEND      *next_ptr;              /* Next suspend block pointer*/
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input mailbox pointer into internal pointer.  */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Determine if the mailbox pointer is valid */
    NU_ERROR_CHECK(((mailbox == NU_NULL) || (mailbox -> mb_id != MB_MAILBOX_ID)), status, NU_INVALID_MAILBOX);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Call protection just in case another thread is using the mailbox.  */
        TCCT_Schedule_Lock();

        /* Clear the mailbox ID.  */
        mailbox -> mb_id =  0;

        /* Remove the mailbox from the list of created mailboxes.  */
        NU_Remove_From_List(&MBD_Created_Mailboxes_List,
                            &(mailbox -> mb_created));

        /* Decrement the total number of created mailboxes.  */
        MBD_Total_Mailboxes--;

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  mailbox -> mb_suspension_list;

        /* Walk the chain task(s) currently suspended on the mailbox.  */
        preempt =  0;
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_MAILBOX_DELETED.  */
            suspend_ptr -> mb_return_status =  NU_MAILBOX_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (MB_SUSPEND *) (suspend_ptr -> mb_suspend_link.cs_next);

            /* Trace log */
            T_MBOX_DELETE((VOID*)mailbox, OBJ_UNBLKD_CTXT);

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

        /* Trace log */
        T_MBOX_DELETE((VOID*)mailbox, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the list of created mailboxes.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_MBOX_DELETE((VOID*)mailbox, status);
    }

    /* Return a successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Send_To_Mailbox
*
*   DESCRIPTION
*
*       This function sends a 4-word message to the specified mailbox.
*       If there are one or more tasks suspended on the mailbox for a
*       message, the message is copied into the message area of the
*       first task waiting and that task is resumed.  If the mailbox
*       is full, suspension of the calling task is possible.
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
*       TCCT_Schedule_Lock                  Protect against system
*                                           access
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
STATUS NU_Send_To_Mailbox(NU_MAILBOX *mailbox_ptr, VOID *message,
                          UNSIGNED suspend)
{
    R1 MB_MCB       *mailbox;               /* Mailbox control block ptr */
    MB_SUSPEND      suspend_block;          /* Allocate suspension block */
    R2 MB_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    R3 UNSIGNED     *source_ptr;            /* Pointer to source         */
    R4 UNSIGNED     *destination_ptr;       /* Pointer to destination    */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          preempt;                /* Preempt flag              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input mailbox pointer into internal pointer.  */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Determine if mailbox pointer is valid.  */
    NU_ERROR_CHECK((mailbox == NU_NULL), status, NU_INVALID_MAILBOX);

    /* Determine if mailbox pointer is valid */
    NU_ERROR_CHECK((mailbox -> mb_id != MB_MAILBOX_ID), status, NU_INVALID_MAILBOX);

    /* Determine if message pointer is valid */
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

                /* Suspension is requested.   */

                /* Increment the number of tasks waiting.  */
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
                T_MBOX_SEND((VOID*)mailbox, (VOID*)message, suspend, OBJ_BLKD_CTXT);

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
                T_MBOX_SEND((VOID*)mailbox, (VOID*)message, suspend, status);

            }
        }
        else
        {

            /* Determine if a task is waiting on the mailbox.  */
            if (mailbox -> mb_suspension_list)
            {

                /* Task is waiting on mailbox for a message.  */

                /* Decrement the number of tasks waiting on mailbox.  */
                mailbox -> mb_tasks_waiting--;

                /* Remove the first suspended block from the list.  */
                suspend_ptr =  mailbox -> mb_suspension_list;
                NU_Remove_From_List((CS_NODE **)
                                    &(mailbox -> mb_suspension_list),
                                    &(suspend_ptr -> mb_suspend_link));

                /* Setup the source and destination pointers.  */
                source_ptr =       (UNSIGNED *) message;
                destination_ptr =  suspend_ptr -> mb_message_area;

                /* Copy the message directly into the waiting task's
                   destination.  */
                *destination_ptr       =  *source_ptr;
                *(destination_ptr + 1) =  *(source_ptr + 1);
                *(destination_ptr + 2) =  *(source_ptr + 2);
                *(destination_ptr + 3) =  *(source_ptr + 3);

                /* Setup the appropriate return value.  */
                suspend_ptr -> mb_return_status =  NU_SUCCESS;

                /* Trace log */
                T_MBOX_SEND((VOID*)mailbox, (VOID*)message, suspend, OBJ_UNBLKD_CTXT);

                /* Wakeup the waiting task and check for preemption.  */
                preempt =
                   TCC_Resume_Task((NU_TASK *) suspend_ptr -> mb_suspended_task,
                                                    NU_MAILBOX_SUSPEND);

                /* Trace log */
                T_MBOX_SEND((VOID*)mailbox, (VOID*)message, suspend, OBJ_ACTION_SUCCESS);

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

                /* Trace log */
                T_MBOX_SEND((VOID*)mailbox, (VOID*)message, suspend, OBJ_ACTION_SUCCESS);
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
        T_MBOX_SEND((VOID*)mailbox, (VOID*)message, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Receive_From_Mailbox
*
*   DESCRIPTION
*
*       This function receives a message from the specified mailbox.
*       If there is a message currently in the mailbox, the message is
*       removed from the mailbox and placed in the caller's area.
*       Otherwise, if no message is present in the mailbox, suspension
*       of the calling task is possible.
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
*       TCC_Task_Priority                   Pickup task priority
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
*       suspend                             Suspension option if empty
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_MAILBOX_EMPTY                If mailbox is currently
*                                           empty
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
STATUS NU_Receive_From_Mailbox(NU_MAILBOX *mailbox_ptr, VOID *message,
                               UNSIGNED suspend)
{
    R1 MB_MCB       *mailbox;               /* Mailbox control block ptr */
    MB_SUSPEND      suspend_block;          /* Allocate suspension block */
    R2 MB_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    R3 UNSIGNED     *source_ptr;            /* Pointer to source         */
    R4 UNSIGNED     *destination_ptr;       /* Pointer to destination    */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          preempt;                /* Preemption flag           */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input mailbox pointer into internal pointer */
    mailbox =  (MB_MCB *) mailbox_ptr;

    /* Determine if mailbox pointer is invalid */
    NU_ERROR_CHECK((mailbox == NU_NULL), status, NU_INVALID_MAILBOX);

    /* Determine if mailbox pointer is invalid */
    NU_ERROR_CHECK((mailbox -> mb_id != MB_MAILBOX_ID), status, NU_INVALID_MAILBOX);

    /* Determine if message pointer is valid */
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

            /* Copy message from mailbox into the caller's area.  */

            /* Setup the source and destination pointers.  */
            source_ptr =       &(mailbox -> mb_message_area[0]);
            destination_ptr =  (UNSIGNED *) message;

            /* Copy the message directly into the waiting task's
               destination.  */
            *destination_ptr =        *source_ptr;
            *(destination_ptr + 1) =  *(source_ptr + 1);
            *(destination_ptr + 2) =  *(source_ptr + 2);
            *(destination_ptr + 3) =  *(source_ptr + 3);

            /* Determine if another task is waiting to place something into the
               mailbox.  */
            if (mailbox -> mb_suspension_list)
            {

                /* Yes, another task is waiting to send something to the
                   mailbox.  */

                /* Decrement the number of tasks waiting counter.  */
                mailbox -> mb_tasks_waiting--;

                /* Remove the first suspended block from the list.  */
                suspend_ptr =  mailbox -> mb_suspension_list;
                NU_Remove_From_List((CS_NODE **)
                                    &(mailbox -> mb_suspension_list),
                                    &(suspend_ptr -> mb_suspend_link));

                /* Setup the source and destination pointers.  */
                source_ptr =       suspend_ptr -> mb_message_area;
                destination_ptr =  &(mailbox -> mb_message_area[0]);

                /* Copy the message directly into the waiting task's
                   destination.  */
                *destination_ptr =        *source_ptr;
                *(destination_ptr + 1) =  *(source_ptr + 1);
                *(destination_ptr + 2) =  *(source_ptr + 2);
                *(destination_ptr + 3) =  *(source_ptr + 3);

                /* Setup the appropriate return value.  */
                suspend_ptr -> mb_return_status =  NU_SUCCESS;

                /* Trace log */
                T_MBOX_RECV((VOID*)mailbox, (VOID*)message, suspend, OBJ_UNBLKD_CTXT);

                /* Resume the suspended task.  */
                preempt =
                   TCC_Resume_Task((NU_TASK *) suspend_ptr -> mb_suspended_task,
                                                           NU_MAILBOX_SUSPEND);

                /* Trace log */
                T_MBOX_RECV((VOID*)mailbox, (VOID*)message, suspend, OBJ_ACTION_SUCCESS);

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

                /* Clear the message present flag.  */
                mailbox -> mb_message_present =  NU_FALSE;

                /* Trace log */
                T_MBOX_RECV((VOID*)mailbox, (VOID*)message, suspend, OBJ_ACTION_SUCCESS);

            }

        }
        else
        {

            /* Mailbox is empty.  Determine if suspension is required.  */
            if (suspend)
            {

                /* Suspension is required.  */

                /* Increment the number of tasks waiting on the mailbox counter. */
                mailbox -> mb_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
                suspend_ptr -> mb_mailbox =                  mailbox;
                suspend_ptr -> mb_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> mb_suspend_link.cs_previous = NU_NULL;
                suspend_ptr -> mb_message_area =             (UNSIGNED *) message;
                task =                            (TC_TCB *) TCCT_Current_Thread();
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
                                              &(suspend_block.mb_suspend_link));
                }

                /* Trace log */
                T_MBOX_RECV((VOID*)mailbox, (VOID*)message, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the mailbox.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_MAILBOX_SUSPEND,
                                            MBC_Cleanup, &suspend_block, suspend);

                /* Pickup the return status.  */
                status =  suspend_ptr -> mb_return_status;

            }
            else
            {

                /* Return a status of NU_MAILBOX_EMPTY because there is
                   nothing in the mailbox.  */
                status =  NU_MAILBOX_EMPTY;

                /* Trace log */
                T_MBOX_RECV((VOID*)mailbox, (VOID*)message, suspend, status);

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
        T_MBOX_RECV((VOID*)mailbox, (VOID*)message, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       MBC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a mailbox.  It is not called unless a timeout or a task
*       terminate is in progress.  Note that protection is already in
*       effect - the same protection at suspension time.
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
VOID  MBC_Cleanup(VOID *information)
{
    MB_SUSPEND      *suspend_ptr;           /* Suspension block pointer  */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (MB_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> mb_return_status =  NU_TIMEOUT;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> mb_mailbox) -> mb_tasks_waiting--;

    /* Unlink the suspend block from the suspension list.  */
    NU_Remove_From_List((CS_NODE **)
                        &((suspend_ptr -> mb_mailbox) -> mb_suspension_list),
                        &(suspend_ptr -> mb_suspend_link));

    /* Return to user mode */
    NU_USER_MODE();
}
