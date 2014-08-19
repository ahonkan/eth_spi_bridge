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
*       tcc_current_task.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core current task routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Current_Task_Pointer             Retrieve current task ptr
*       TCC_Current_Appliation_Task_Pointer Retrieve current app ptr
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Current_Task_Pointer
*
*   DESCRIPTION
*
*       This function returns the pointer of the currently executing
*       task.  If the current thread is not a task thread, a NU_NULL
*       is returned.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Task Pointer                        Active tasks pointer or
*                                           NU_NULL if not a task
*
***********************************************************************/
NU_TASK *NU_Current_Task_Pointer(VOID)
{
    /* Determine if a task thread is executing.  */
    if ((TCD_Current_Thread) &&
        (((TC_TCB *) TCD_Current_Thread) -> tc_id == TC_TASK_ID) &&
        (ESAL_GE_ISR_EXECUTING() == NU_FALSE))
    {
        /* Task thread is running, return the pointer.  */
        return((NU_TASK *) TCD_Current_Thread);
    }
    else
    {
        /* No, task thread is not running, return a NU_NULL.  */
        return(NU_NULL);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       TCC_Current_Application_Task_Pointer
*
*   DESCRIPTION
*
*       This function returns the pointer of the current or last
*       executed application task.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Task Pointer                        Active app task pointer
*
***********************************************************************/
NU_TASK * TCC_Current_Application_Task_Pointer(VOID)
{
    return (NU_TASK *) TCD_Current_App_Task;
}
