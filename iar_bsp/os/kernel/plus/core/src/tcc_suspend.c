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
*       tcc_suspend.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core suspend service routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Suspend_Task                     Suspend a task service call
*       TCC_Debug_Suspend_Service           Debug suspend a task service
*                                           call
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                           Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Suspend_Task
*
*   DESCRIPTION
*
*       This function provides a suitable interface to the actual
*       service to suspend a task.
*
*   CALLED BY
*
*       Application
*       TCCE_Suspend_Service                Error checking function
*
*   CALLS
*
*       TCC_Suspend_Task                    Suspend a task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect system structures
*       TCCT_Schedule_Unlock                Release system structures
*       [DBG_OS_Task_Is_Debug]              Task debug status function
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS                          If successful completion
*       NU_INVALID_TASK                     Task pointer is invalid
*
***********************************************************************/
STATUS NU_Suspend_Task(NU_TASK *task_ptr)
{
    STATUS status = NU_SUCCESS;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Determine if the task pointer is valid.  */
    NU_ERROR_CHECK(((task_ptr == NU_NULL) || (task_ptr -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    /* Can't suspend a task in a finished or terminated state */
    NU_ERROR_CHECK(((task_ptr -> tc_status == NU_FINISHED) ||  (task_ptr -> tc_status == NU_TERMINATED)), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect system data structures.  */
        TCCT_Schedule_Lock();

        /* Call the actual routine to suspend the task.  */
        TCC_Suspend_Task(task_ptr, NU_PURE_SUSPEND, NU_NULL, NU_NULL, NU_SUSPEND);

        /* Release system protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }

    /* Always return a successful status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       TCC_Debug_Suspend_Service
*
*   DESCRIPTION
*
*       This function provides a suitable interface to the
*       actual service to debug suspend a task.
*
*   CALLED BY
*
*       Debugging services
*
*   CALLS
*
*       TCC_Suspend_Task                    Suspend a task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect system structures
*       TCCT_Schedule_Unlock                Release system structures
*
*   INPUTS
*
*       task_ptr                            The control block pointer for
*                                           the task to be suspended.
*
*   OUTPUTS
*
*       NU_SUCCESS                          Always a successful status
*
***********************************************************************/
STATUS  TCC_Debug_Suspend_Service(NU_TASK *task_ptr)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Protect system data structures.  */
    TCCT_Schedule_Lock();

    /* Call the actual routine to suspend the task.  */
    TCC_Suspend_Task(task_ptr, NU_DEBUG_SUSPEND, NU_NULL, NU_NULL, NU_SUSPEND);

    /* Release system protection.  */
    TCCT_Schedule_Unlock();

    /* Always return a successful status.  */
    return(NU_SUCCESS);
}
