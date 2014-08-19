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
*       tcc_resume.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core resume service routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Resume_Task                      Resume a task service call
*       TCC_Debug_Resume_Service            Debug resume service call
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
#include        "services/nu_trace_os_mark.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Resume_Task
*
*   DESCRIPTION
*
*       This function provides an interface identical to the application
*       service call to resume a task.
*
*   CALLED BY
*
*       Application
*       TCCE_Resume_Service                 Error checking function
*
*   CALLS
*
*       TCC_Resume_Task                     Resume a task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect system structures
*       TCCT_Schedule_Unlock                Release system protection
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
*       NU_INVALID_RESUME                   Not previously suspended
*
***********************************************************************/
STATUS NU_Resume_Task(NU_TASK *task_ptr)
{
    STATUS status = NU_SUCCESS;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Determine if the task pointer is valid.  */
    NU_ERROR_CHECK(((task_ptr == NU_NULL) || (task_ptr -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    /* Make sure that the task is suspended in an identical manner.  */
    NU_ERROR_CHECK((TCCE_Validate_Resume(NU_PURE_SUSPEND, task_ptr)), status, NU_INVALID_RESUME);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect system structures.  */
        TCCT_Schedule_Lock();

        if (TCC_Resume_Task(task_ptr, NU_PURE_SUSPEND))
        {
            /* Trace log */
            T_TASK_SUSPEND((VOID*)TCCT_Current_Thread(), NU_PURE_SUSPEND);

            /* Transfer control back to the system for a context switch.  */
            TCCT_Control_To_System();
        }

        /* Release protection of system structures.  */
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
*       TCC_Debug_Resume_Service
*
*   DESCRIPTION
*
*       This (conditionally compiled) function provides a suitable
*       interface to the actual service to debug resume a task.
*
*   CALLED BY
*
*       Debugging services
*
*   CALLS
*
*       TCC_Resume_Task                     Resume a task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect system structures
*       TCCT_Schedule_Unlock                Release system protection
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS                          Always a successful status
*
***********************************************************************/
STATUS  TCC_Debug_Resume_Service(NU_TASK *task_ptr)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Protect system structures.  */
    TCCT_Schedule_Lock();

    /* Call the actual resume task function.  If the function returns a
       NU_TRUE, context switching is needed.  */
    if (TCC_Resume_Task(task_ptr, NU_DEBUG_SUSPEND))
    {
        /* Trace log */
        T_TASK_SUSPEND((VOID*)TCCT_Current_Thread(), NU_DEBUG_SUSPEND);

        /* Transfer control back to the system for a context switch.  */
        TCCT_Control_To_System();
    }

    /* Release protection of system structures.  */
    TCCT_Schedule_Unlock();

    /* Always return a successful status.  */
    return(NU_SUCCESS);
}
