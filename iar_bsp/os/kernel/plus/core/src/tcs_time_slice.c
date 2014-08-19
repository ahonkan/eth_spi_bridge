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
*       tcs_time_slice.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the supplemental Change Time Slice routine for
*       the Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Change_Time_Slice                Change task's time-slice
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
*       NU_Change_Time_Slice
*
*   DESCRIPTION
*
*       This function changes the time slice of the specified task.  A
*       time slice value of 0 disables time slicing.
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
*       TCCT_Schedule_Lock                  Protect scheduling info
*       TCCT_Schedule_Unlock                Release protection of info
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       time_slice                          New time slice value
*
*   OUTPUTS
*
*       old_time_slice                      Original time slice value
*
***********************************************************************/
UNSIGNED NU_Change_Time_Slice(NU_TASK *task_ptr, UNSIGNED time_slice)
{
    TC_TCB          *task;                  /* Task control block ptr    */
    UNSIGNED        old_time_slice = ~time_slice; /* Old time slice value      */
    NU_SUPERV_USER_VARIABLES

    /* Move input task pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Determine if the task pointer is valid.  Upon error
       return the current request */
    NU_PARAM_CHECK(((task == NU_NULL) || ((task -> tc_id != TC_TASK_ID))), old_time_slice, time_slice);

    if (old_time_slice != time_slice)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect the scheduling information.  */
        TCCT_Schedule_Lock();

        /* Save the old time slice value.  */
        old_time_slice =  task -> tc_time_slice;

        /* Store the new time slice value.  */
        task -> tc_time_slice =      time_slice;
        task -> tc_cur_time_slice =  time_slice;

        /* Trace log */
        T_TASK_CHG_TIMESLICE((VOID*)task, time_slice, old_time_slice);

        /* Release protection of information.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TASK_CHG_TIMESLICE((VOID*)task, time_slice, old_time_slice);
    }

    /* Return the previous time slice value.  */
    return(old_time_slice);
}
