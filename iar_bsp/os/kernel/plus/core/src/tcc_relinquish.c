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
*       tcc_relinquish.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core relinquish routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Relinquish                       Relinquish task execution
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
*       NU_Relinquish
*
*   DESCRIPTION
*
*       This function moves the calling task to the end of other tasks
*       at the same priority level.  The calling task does not execute
*       again until all the other tasks of the same priority get a
*       chance to execute.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect system structures
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_Relinquish(VOID)
{
    TC_TCB          *task;                  /* Pointer to task           */
    NU_SUPERV_USER_VARIABLES

    /* Pickup the current thread and place it in the task pointer.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Determine if the current thread is a task, if not ignore the request.  */
    NU_PARAM_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), task, NU_NULL);

    if (task != NU_NULL)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against multiple access to the system structures.  */
        TCCT_Schedule_Lock();

        /* Determine if another task is ready to run.  */
        if ((task -> tc_ready_next != task) ||
            (task -> tc_priority != (DATA_ELEMENT) TCD_Highest_Priority))
        {
            /* Trace log */
            T_TASK_READY((VOID*)task);

            /* Move the executing task to the end of tasks having the same
               priority. */
            *(task -> tc_priority_head) =  task -> tc_ready_next;

            /* Setup the next task to execute.  */
            TCCT_Set_Execute_Task(TCD_Priority_List[TCD_Highest_Priority]);

            /* Ensure the task gets a full time-slice next time
               it is scheduled */
            task -> tc_cur_time_slice = task -> tc_time_slice;

            /* Transfer control back to the system.  */
            TCCT_Control_To_System();
        }

        /* Release protection of system structures.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
}
