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
*       tcc_task_sleep.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core task sleep routine for the Thread
*       Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Sleep                            Task sleep request
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
*       NU_Sleep
*
*   DESCRIPTION
*
*       This function provides task sleep suspensions.  Its primary
*       purpose is to interface with the actual task suspension function.
*
*   CALLED BY
*
*       Application
*       TCCE_Task_Sleep                     Error checking
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
*       ticks                               Number of timer ticks
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_Sleep(UNSIGNED ticks)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Protect system data structures.  */
    TCCT_Schedule_Lock();

    /* Call the actual routine to suspend the task.  */
    TCC_Suspend_Task((NU_TASK *) TCD_Current_Thread, NU_SLEEP_SUSPEND,
                                                NU_NULL, NU_NULL, ticks);

    /* Release system protection.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();
}
