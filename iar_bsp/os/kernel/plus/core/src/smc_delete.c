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
*       smc_delete.c
*
*   COMPONENT
*
*       SM - Semaphore Management
*
*   DESCRIPTION
*
*       This file contains the core Delete routines for the Semaphore
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Semaphore                 Delete a semaphore
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       semaphore.h                         Semaphore functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/semaphore.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *SMD_Created_Semaphores_List;
extern UNSIGNED         SMD_Total_Semaphores;
extern STATUS  SMC_Free_PI_Semaphore (SM_SCB *pi_semaphore, TC_TCB *task);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Semaphore
*
*   DESCRIPTION
*
*       This function deletes a semaphore and removes it from the list
*       of created semaphores.  All tasks suspended on the semaphore are
*       resumed.  Note that this function does not free the memory
*       associated with the semaphore control block.
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
*       SMC_Free_PI_Semaphore               Frees a PI Semaphore
*
*   INPUTS
*
*       semaphore_ptr                       Semaphore control block ptr
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Success
*           NU_SEMAPHORE_INVALID_OWNER      Semaphore is being deleted
*                                           by task other than owner
*           NU_INVALID_SEMAPHORE            Invalid semaphore pointer
*
***********************************************************************/
STATUS NU_Delete_Semaphore(NU_SEMAPHORE *semaphore_ptr)
{
    R1 SM_SCB       *semaphore;             /* Semaphore CB ptr          */
    SM_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    SM_SUSPEND      *next_ptr;              /* Next suspend block        */
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;    /* Status                    */
    TC_TCB          *task;                  /* Task pointer              */
    NU_SUPERV_USER_VARIABLES

    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Determine if the semaphore pointer is valid  */
    NU_ERROR_CHECK(((semaphore == NU_NULL) || (semaphore -> sm_id != SM_SEMAPHORE_ID)), status, NU_INVALID_SEMAPHORE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Initialize variables */
        preempt = NU_FALSE;

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Get a pointer the currently executing task */
        task = (TC_TCB *) TCCT_Current_Thread();

        /* Protect against access to the semaphore.  */
        TCCT_Schedule_Lock();

        /* Check if this is a PI Semaphore */
        if (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT)
        {
            /* Check if the current task is the owner */
            if (semaphore -> sm_semaphore_owner == task)
            {
                /* Free Semaphore and adjust priority of the task
                   resetting the semaphore */
                preempt = SMC_Free_PI_Semaphore (semaphore, task);

            }
            else
            {
                /* We should be able to delete an un-owned semaphore */
                /* Ensure semaphore is not owned */
                if (semaphore -> sm_semaphore_owner != NU_NULL)
                {
                    /* PI Semaphore is being deleted from a task other than the owner */
                    status = NU_SEMAPHORE_INVALID_OWNER;

                    /* Trace log */
                    T_SEM_DELETE((VOID*)semaphore, (VOID*)task, status);
                }
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Clear the semaphore ID.  */
            semaphore -> sm_id =  0;

            /* Remove the semaphore from the list of created semaphores.  */
            NU_Remove_From_List(&SMD_Created_Semaphores_List,
                                &(semaphore -> sm_created));

            /* Decrement the total number of created semaphores.  */
            SMD_Total_Semaphores--;

            /* Pickup the suspended task pointer list.  */
            suspend_ptr =  semaphore -> sm_suspension_list;

            /* Walk the chain task(s) currently suspended on the semaphore.  */
            while (suspend_ptr)
            {
                /* Resume the suspended task.  Insure that the status returned is
                   NU_SEMAPHORE_DELETED.  */
                suspend_ptr -> sm_return_status =  NU_SEMAPHORE_DELETED;

                /* Point to the next suspend structure in the link.  */
                next_ptr =  (SM_SUSPEND *) (suspend_ptr -> sm_suspend_link.cs_next);

                /* Trace log */
                T_SEM_DELETE((VOID*)semaphore, (VOID*)task, OBJ_UNBLKD_CTXT);

                /* Resume the specified task.  */
                preempt =  preempt |
                    TCC_Resume_Task((NU_TASK *) suspend_ptr -> sm_suspended_task,
                                                        NU_SEMAPHORE_SUSPEND);

                /* Determine if the next is the same as the current pointer.  */
                if (next_ptr == semaphore -> sm_suspension_list)
                {
                    /* Clear the suspension pointer to signal the end of the list
                       traversal.  */
                    suspend_ptr =  NU_NULL;
                }
                else
                {
                    /* Move the next pointer into the suspend block pointer.  */
                    suspend_ptr =  next_ptr;
                }
            }

            /* Trace log */
            T_SEM_DELETE((VOID*)semaphore, (VOID*)task, OBJ_ACTION_SUCCESS);

            /* Determine if preemption needs to occur.  */
            if (preempt)
            {
                /* Trace log */
                T_TASK_READY((VOID*)TCCT_Current_Thread());

                /* Transfer control to system to facilitate preemption.  */
                TCCT_Control_To_System();
            }
        }

        /* Release protection against access to the list of created semaphores. */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SEM_DELETE((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), status);
    }

    /* Return a successful completion.  */
    return (status);
}
