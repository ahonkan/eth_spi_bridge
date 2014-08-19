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
*       tcs_preemption.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the supplemental Change Preemption routine
*       for the Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Change_Preemption                Change task's preemption
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

/* Define external inner-component global data references.  */

extern TC_TCB               *TCD_Priority_List[TC_PRIORITIES+1];
extern INT                  TCD_Highest_Priority;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Change_Preemption
*
*   DESCRIPTION
*
*       This function changes the preemption posture of the calling
*       task.  Preemption for a task may be enabled or disabled.  If
*       it is disabled, the task runs until it suspends or relinquishes.
*       If a preemption is pending, a call to this function to enable
*       preemption causes a context switch.
*
*   CALLED BY
*
*       Application
*       TCSE_Change_Preemption              Error checking function
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect scheduling info
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TCCT_Schedule_Unlock                Release protection of info
*
*   INPUTS
*
*       preempt                             Preempt selection parameter
*
*   OUTPUTS
*
*       old_preempt                         Original preempt value
*
***********************************************************************/
OPTION NU_Change_Preemption(OPTION preempt)
{
    TC_TCB          *task;                  /* Pointer to task           */
    OPTION          old_preempt = ~preempt;
    NU_SUPERV_USER_VARIABLES

    /* Pickup the current thread and place it in the task pointer.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Determine if the current thread is really a task thread.
       Upon error return the original request */
    NU_PARAM_CHECK(((task == NU_NULL) || ((task -> tc_id != TC_TASK_ID))), old_preempt, preempt);

    if (old_preempt != preempt)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect the scheduling information.  */
        TCCT_Schedule_Lock();

        /* Save the old preempt value.  */
        if (task -> tc_preemption)
        {
            /* Previously enabled.  */
            old_preempt =  NU_PREEMPT;
        }
        else
        {
            /* Previously disabled.  */
            old_preempt =  NU_NO_PREEMPT;
        }

        /* Trace log */
        T_TASK_CHG_PREMPTION((VOID*)task, preempt, old_preempt);

        /* Process the new value.  */
        if (preempt == NU_NO_PREEMPT)
        {
            /* Disable preemption.  */
            TCD_Execute_Task -> tc_preemption =  NU_FALSE;
        }
        else
        {

            /* Enable preemption.  */
            task -> tc_preemption =  NU_TRUE;

            /* Check for a preemption condition.  */
            if ((task == TCD_Execute_Task) &&
                (TCD_Highest_Priority < ((INT) TCD_Execute_Task -> tc_priority)))
            {
                /* Preempt the current task.  */
                TCCT_Set_Execute_Task(TCD_Priority_List[TCD_Highest_Priority]);

                /* Trace log */
                T_TASK_SUSPEND((VOID*)TCCT_Current_Thread(), NU_PURE_SUSPEND);

                /* Transfer control to the system.  */
                TCCT_Control_To_System();
            }
        }

        /* Release protection of information.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TASK_CHG_PREMPTION((VOID*)task, preempt, old_preempt);
    }

    /* Return the previous preemption posture.  */
    return(old_preempt);
}
