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
*       sms_reset.c
*
*   COMPONENT
*
*       SM - Semaphore Management
*
*   DESCRIPTION
*
*       This file contains the supplemental Reset routine for the
*       Semaphore Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Reset_Semaphore                  Reset semaphore
*                                           terminate condition
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

extern STATUS  SMC_Free_PI_Semaphore (SM_SCB *pi_semaphore, TC_TCB *task);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Reset_Semaphore
*
*   DESCRIPTION
*
*       This function resets a semaphore back to the initial state.  All
*       tasks suspended on the semaphore are resumed with the reset
*       completion status.
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
*       TCCT_Schedule_Lock                  Protect semaphore
*       TCCT_Schedule_Unlock                Release protection
*       SMC_Free_PI_Semaphore               Frees a PI Semaphore
*
*   INPUTS
*
*       semaphore_ptr                       Semaphore control block ptr
*       initial_count                       Initial count to reset the
*                                           semaphore to
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If Semaphore is reset
*           NU_INVALID_SEMAPHORE            Invalid semaphore pointer
*           NU_INVALID_COUNT                Invalid count for PI Semaphore
*
***********************************************************************/
STATUS NU_Reset_Semaphore(NU_SEMAPHORE *semaphore_ptr,
                          UNSIGNED initial_count)
{
    R1 SM_SCB       *semaphore;             /* Semaphore control block ptr  */
    R2 SM_SUSPEND   *suspend_ptr;           /* Suspend block pointer        */
    R3 SM_SUSPEND   *next_ptr;              /* Next suspend block pointer   */
    STATUS          preempt;                /* Status for resume call       */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Determine if semaphore pointer is valid */
    NU_ERROR_CHECK((semaphore == NU_NULL), status, NU_INVALID_SEMAPHORE);

    /* Determine if semaphore pointer is valid */
    NU_ERROR_CHECK((semaphore -> sm_id != SM_SEMAPHORE_ID), status, NU_INVALID_SEMAPHORE);

    /* Determine if count is valid for PI Semaphore */
    NU_ERROR_CHECK(((semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT) && (initial_count != 1)), status, NU_INVALID_COUNT);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Initialize variables */
        preempt = NU_FALSE;

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against access to the semaphore.  */
        TCCT_Schedule_Lock();

        /* Check if this is a PI Semaphore */
        if (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT)
        {
            /* Check if the semaphore is owned */
            if (semaphore -> sm_semaphore_owner != NU_NULL)
            {
                /* Free Semaphore and adjust priority of the owner task. */
                preempt = SMC_Free_PI_Semaphore (semaphore,
                                            semaphore -> sm_semaphore_owner);
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Pickup the suspended task pointer list.  */
            suspend_ptr =  semaphore -> sm_suspension_list;

            /* Walk the chain task(s) currently suspended on the semaphore.  */
            while (suspend_ptr)
            {
                /* Resume the suspended task.  Insure that the status returned is
                   NU_SEMAPHORE_RESET.  */
                suspend_ptr -> sm_return_status =  NU_SEMAPHORE_RESET;

                /* Point to the next suspend structure in the link.  */
                next_ptr =  (SM_SUSPEND *) (suspend_ptr -> sm_suspend_link.cs_next);

                /* Trace log */
                T_SEM_RESET((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), initial_count, OBJ_UNBLKD_CTXT);

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

            /* Initialize the semaphore.  */
            semaphore -> sm_semaphore_count =  initial_count;
            semaphore -> sm_tasks_waiting =    0;
            semaphore -> sm_suspension_list =  NU_NULL;
            if (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT)
            {
                /* Clear the owner field */
                semaphore -> sm_semaphore_owner = NU_NULL;
            }

            /* Trace log */
            T_SEM_RESET((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), initial_count, OBJ_ACTION_SUCCESS);

            /* Determine if preemption needs to occur.  */
            if (preempt)
            {
                /* Trace log */
                T_TASK_READY((VOID*)TCCT_Current_Thread());

                /* Transfer control to system to facilitate preemption.  */
                TCCT_Control_To_System();
            }
        }

        /* Release protection against access to the semaphore.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SEM_RESET((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), initial_count, status);
    }

    /* Return completion status.  */
    return(status);
}
