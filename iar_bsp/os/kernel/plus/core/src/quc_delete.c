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
*       quc_delete.c
*
*   COMPONENT
*
*       QU - Queue Management
*
*   DESCRIPTION
*
*       This file contains the core Delete routine for the
*       Queue management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Queue                     Delete a message queue
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       queue.h                             Queue functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/queue.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *QUD_Created_Queues_List;
extern UNSIGNED         QUD_Total_Queues;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Queue
*
*   DESCRIPTION
*
*       This function deletes a queue and removes it from the list of
*       created queues.  All tasks suspended on the queue are
*       resumed.  Note that this function does not free the memory
*       associated with the queue.
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
*       queue_ptr                           Queue control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_QUEUE                    Invalid queue pointer
*
***********************************************************************/
STATUS NU_Delete_Queue(NU_QUEUE *queue_ptr)
{
    R1 QU_QCB       *queue;                 /* Queue control block ptr   */
    QU_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    QU_SUSPEND      *next_ptr;              /* Next suspend block pointer*/
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Move input queue pointer into internal pointer. */
    queue =  (QU_QCB *) queue_ptr;

    /* Determine if there is an error with the queue pointer. */
    NU_ERROR_CHECK((queue == NU_NULL), status, NU_INVALID_QUEUE);

    /* Determine if the queue pointer is valid. */
    NU_ERROR_CHECK((queue -> qu_id != QU_QUEUE_ID), status, NU_INVALID_QUEUE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against access to the queue.  */
        TCCT_Schedule_Lock();

        /* Clear the queue ID.  */
        queue -> qu_id =  0;

        /* Remove the queue from the list of created queues.  */
        NU_Remove_From_List(&QUD_Created_Queues_List, &(queue -> qu_created));

        /* Decrement the total number of created queues.  */
        QUD_Total_Queues--;

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  queue -> qu_suspension_list;

        /* Walk the chain task(s) currently suspended on the queue.  */
        preempt =  0;
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_QUEUE_DELETED.  */
            suspend_ptr -> qu_return_status =  NU_QUEUE_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (QU_SUSPEND *) (suspend_ptr -> qu_suspend_link.cs_next);

            /* Trace log */
            T_Q_DELETE((VOID*)queue, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> qu_suspended_task,
                                                    NU_QUEUE_SUSPEND);

            /* Determine if the next is the same as the head pointer.  */
            if (next_ptr == queue -> qu_suspension_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Position suspend pointer to the next pointer.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Pickup the urgent message suspension list.  */
        suspend_ptr =  queue -> qu_urgent_list;

        /* Walk the chain task(s) currently suspended on the queue.  */
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_QUEUE_DELETED.  */
            suspend_ptr -> qu_return_status =  NU_QUEUE_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (QU_SUSPEND *) (suspend_ptr -> qu_suspend_link.cs_next);

            /* Trace log */
            T_Q_DELETE((VOID*)queue, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                    TCC_Resume_Task((NU_TASK *) suspend_ptr -> qu_suspended_task,
                                                    NU_QUEUE_SUSPEND);

            /* Determine if the next is the same as the head pointer.  */
            if (next_ptr == queue -> qu_urgent_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Position to the next suspend block in the list.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Trace log */
        T_Q_DELETE((VOID*)queue, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the list of created queues.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_Q_DELETE((VOID*)queue, status);
    }

    /* Return a successful completion.  */
    return(status);
}
