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
*       tcc_terminate.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core terminate task routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Terminate_Task                   Terminate the specified task
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                           Thread Control functions
*       timer.h                           Timer Control function
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "services/nu_trace_os_mark.h"
#include        "os/kernel/plus/core/inc/semaphore.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Terminate_Task
*
*   DESCRIPTION
*
*       This function terminates the specified task.  If the task is
*       already terminated, this function does nothing.  If the task
*       to terminate is currently suspended, the specified cleanup
*       routine is also invoked to cleanup suspension data structures.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       Cleanup routine                     Task's suspend cleanup func
*       TCC_Suspend_Task                    Suspend a ready task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created task list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_TASK                     Indicates task pointer is
*                                           invalid
*
***********************************************************************/
STATUS NU_Terminate_Task(NU_TASK *task_ptr)
{
    R1 TC_TCB    *task;                 /* Task control block ptr    */
    STATUS        status = NU_SUCCESS;  /* Task status               */
    CS_NODE *     node;
    NU_SUPERV_USER_VARIABLES

    /* Move input task control block pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Determine if the task pointer is valid.  */
    NU_ERROR_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect system  data structures.  */
        TCCT_Schedule_Lock();

        /* Release all priority mutexes owned by the task being terminated */
        while (task -> tc_semaphore_count != 0)
        {
            /* Get CS node pointer */
            node = task -> tc_semaphore_list;

            /* Release a priority mutex */
            SMC_Kill_Semaphore_Owner(NU_STRUCT_BASE(node,sm_semaphore_list,NU_SEMAPHORE), task);
        }

        /* Determine if the calling task is the current task.  */
        if (task == (TC_TCB *) TCD_Current_Thread)
        {
            /* Trace log */
            T_TASK_TERMINATED((VOID*)task, status);

            /* Suspend the calling task with the NU_TERMINATED status.  */
            TCC_Suspend_Task(task_ptr, NU_TERMINATED, NU_NULL, NU_NULL,
                                                                NU_SUSPEND);

            /* No need to un-protect, since control never comes back to this
               point and the protection is cleared in TCCT_Control_To_System.  */
        }
        else
        {
            /* Keep trying to terminate the specified task until its status
               indicates that it is terminated or finished.  */
            while ((task -> tc_status != NU_FINISHED) &&
                      task -> tc_status != NU_TERMINATED)
            {
                /* Is the task in a ready state?  */
                if (task -> tc_status == NU_READY)
                {
                    /* Terminate the specified task.  */
                    TCC_Suspend_Task(task_ptr, NU_TERMINATED, NU_NULL,
                                                            NU_NULL,NU_SUSPEND);

                    /* Clear system protection.  */
                    TCCT_Schedule_Unlock();
                }
                else
                {
                    /* Call cleanup routine, if there is one.  */
                    if (task -> tc_cleanup)
                    {
                        /* Call cleanup function.  */
                        (*(task -> tc_cleanup)) (task -> tc_cleanup_info);
                    }

                    /* Status the task as terminated.  */
                    task -> tc_status =  NU_TERMINATED;

                    /* Determine if there is a timer active.  */
                    if (task -> tc_timer_active)
                    {
                        /* Stop the task timer */
                        TMC_Stop_Task_Timer(&(task -> tc_timer_control));

                        /* Clear the timer active flag.  */
                        task -> tc_timer_active =  NU_FALSE;
                    }
                }
            }

            /* Trace log */
            T_TASK_TERMINATED((VOID*)task, status);

            /* Release the protection.  */
            TCCT_Schedule_Unlock();
        }

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
         /* Trace log */
         T_TASK_TERMINATED((VOID*)task, status);
    }

    /* Return successful completion.  */
    return(status);
}
