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
*       tcct_reset_task.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core reset task thread control / scheduling
*       function with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Reset_Task                       Reset the specified task
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/* Define external function references */
extern VOID                 TCC_Task_Shell(VOID);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Reset_Task
*
*   DESCRIPTION
*
*       This function resets the specified task.  Note that a task reset
*       can only be performed on tasks in a finished or terminated state.
*       The task is left in an unconditional suspended state.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       ESAL_GE_STK_Unsolicited_Set         Populates unsolicited
*                                           stack frame
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created task list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       argc                                Optional task parameter
*       argv                                Optional task parameter
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Indicates successful request
*           NU_NOT_TERMINATED               Indicates task was not
*                                           finished or terminated
*           NU_INVALID_TASK                 Indicates task pointer is
*                                           invalid
*
***********************************************************************/
STATUS NU_Reset_Task(NU_TASK *task_ptr, UNSIGNED argc, VOID *argv)
{
    R1 TC_TCB       *task;                  /* Task control block ptr   */
    STATUS          status = NU_SUCCESS;    /* Status of the request    */
    NU_SUPERV_USER_VARIABLES

    /* Move input task control block pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Determine if the task pointer is valid */
    NU_ERROR_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect system structures.  */
        TCCT_Schedule_Lock();

        /* Determine if the task is in the proper state.  */
        if ((task -> tc_status == NU_FINISHED) ||
                            (task -> tc_status == NU_TERMINATED))
        {

            /* Yes, a valid reset is present.  Indicate this in the status.  */
            status =  NU_SUCCESS;

            /* Fill in the new argument information and reset some of the other
               fields.  */
            task -> tc_argc =                   argc;
            task -> tc_argv =                   argv;
            task -> tc_status =                 NU_PURE_SUSPEND;
            task -> tc_delayed_suspend =        NU_FALSE;
            task -> tc_scheduled =              0;


    #if (NU_STACK_FILL == NU_TRUE)

            /* Fill stack with pattern */
            ESAL_GE_MEM_Set(task -> tc_stack_start, NU_STACK_FILL_PATTERN, task -> tc_stack_size);

    #endif  /* NU_STACK_FILL == NU_TRUE */

            /* Calculate stack end address */
            task -> tc_stack_end = (VOID *)((VOID_CAST)task -> tc_stack_start +
                                                       task -> tc_stack_size);

            /* Align the stack end address as required */
            task -> tc_stack_end = ESAL_GE_STK_ALIGN(task -> tc_stack_end);

            /* Populate architecture stack frame */
            task -> tc_stack_pointer = ESAL_GE_STK_Unsolicited_Set(task -> tc_stack_start,
                                                                   task -> tc_stack_end,
                                                                   TCC_Task_Shell);

#if (NU_STACK_CHECKING == NU_TRUE)

            /* Save the minimum amount of remaining stack memory remaining
               on stack */
            task -> tc_stack_minimum =  (UNSIGNED)((VOID_CAST)task -> tc_stack_pointer -
                                                   (VOID_CAST)task -> tc_stack_start);

#endif  /* NU_STACK_CHECKING == NU_TRUE */

            /* Reset the tasks debug suspend status */
            task -> tc_debug_suspend = NU_READY;

        }
        else
        {
            /* The requested task is not in a finished or terminated state.  */
            status =  NU_NOT_TERMINATED;
        }

        /* Trace log */
        T_TASK_RESET(task, status);

        /* Release the protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TASK_RESET(task, status);
    }

    /* Return completion status.  */
    return(status);
}
