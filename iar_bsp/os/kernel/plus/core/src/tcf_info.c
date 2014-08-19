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
*       tcf_info.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains information (fact) Information routines for
*       the Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Task_Information                 Retrieve task information
*       NU_HISR_Information                 Retrieve HISR information
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
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Task_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified task.
*       However, if the supplied task pointer is invalid, the function
*       simply returns an error status.
*
*   CALLED BY
*
*       Application
*       TCFE_Task_Information               Performs error checking
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect scheduling info
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       task_ptr                            Pointer to the task
*       name                                Destination for the name
*       status                              Destination for task status
*       scheduled_count                     Destination for scheduled
*                                           count of the task
*       priority                            Destination for task
*                                           priority
*       preempt                             Destination for preempt flag
*       time_slice                          Destination for time slice
*       stack_base                          Destination for pointer to
*                                           base of task's stack
*       stack_size                          Destination for stack size
*       minimum_stack                       Destination for the minimum
*                                           running size of the stack
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid task pointer is
*                                           supplied
*           NU_INVALID_TASK                 If task pointer is invalid
*
***********************************************************************/
STATUS NU_Task_Information(NU_TASK *task_ptr, CHAR *name,
                           DATA_ELEMENT *status, UNSIGNED *scheduled_count,
                           DATA_ELEMENT *priority, OPTION *preempt,
                           UNSIGNED *time_slice, VOID **stack_base,
                           UNSIGNED *stack_size, UNSIGNED *minimum_stack)
{
    R1 TC_TCB       *task;                  /* Task control block ptr    */
    STATUS          completion = NU_SUCCESS;/* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Verify return pointer for name is valid */
    NU_ERROR_CHECK((name == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for preempt is valid */
    NU_ERROR_CHECK((preempt == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for status is valid */
    NU_ERROR_CHECK((status == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for scheduled_count is valid */
    NU_ERROR_CHECK((scheduled_count == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for priority is valid */
    NU_ERROR_CHECK((priority == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for time_slice is valid */
    NU_ERROR_CHECK((time_slice == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for stack_base is valid */
    NU_ERROR_CHECK((stack_base == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for stack_size is valid */
    NU_ERROR_CHECK((stack_size == NU_NULL), completion, NU_INVALID_POINTER);

    /* Verify return pointer for minimum_stack is valid */
    NU_ERROR_CHECK((minimum_stack == NU_NULL), completion, NU_INVALID_POINTER);

    if (completion == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Move task control block pointer into internal pointer.  */
        task =  (TC_TCB *) task_ptr;

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Determine if this task is valid.  */
        if ((task != NU_NULL) && (task -> tc_id == TC_TASK_ID))
        {
            /* Protect against scheduling changes.  */
            TCCT_Schedule_Lock();

            /* The task pointer is successful.  Reflect this in the completion
               status and fill in the actual information.  */
            completion =  NU_SUCCESS;

            /* Copy the task's name.  */
            strncpy(name, task -> tc_name, NU_MAX_NAME);

            /* Determine the preemption posture.  */
            if (task -> tc_preemption)
            {
                *preempt =          NU_PREEMPT;
            }
            else
            {
                *preempt =          NU_NO_PREEMPT;
            }

            /* Setup the remaining fields.  */
            *status =           task -> tc_status;
            *scheduled_count =  task -> tc_scheduled;
            *priority =         task -> tc_priority;
            *time_slice =       task -> tc_time_slice;
            *stack_base =       task -> tc_stack_start;
            *stack_size =       task -> tc_stack_size;
            *minimum_stack =    task -> tc_stack_minimum;

            /* Release protection.  */
            TCCT_Schedule_Unlock();
        }
        else
        {
            /* Indicate that the task pointer is invalid.   */
            completion =  NU_INVALID_TASK;
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    /* Return the appropriate completion status.  */
    return(completion);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_HISR_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified HISR.
*       However, if the supplied HISR pointer is invalid, the function
*       simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect scheduling info
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       hisr_ptr                            Pointer to the hisr
*       name                                Destination for the name
*       scheduled_count                     Destination for scheduled
*                                           count of the HISR
*       priority                            Destination for HISR
*                                           priority
*       stack_base                          Destination for pointer to
*                                           base of HISR's stack
*       stack_size                          Destination for stack size
*       minimum_stack                       Destination for the minimum
*                                           running size of the stack
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid HISR pointer is
*                                           supplied
*           NU_INVALID_HISR                 If HISR pointer is invalid
*
***********************************************************************/
STATUS NU_HISR_Information(NU_HISR *hisr_ptr, CHAR *name,
                           UNSIGNED *scheduled_count, DATA_ELEMENT *priority,
                           VOID **stack_base, UNSIGNED *stack_size,
                           UNSIGNED *minimum_stack)
{
    R1 TC_HCB       *hisr;                  /* HISR control block ptr    */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input HISR control block pointer into internal pointer.  */
    hisr =  (TC_HCB *) hisr_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this HISR is valid.  */
    if ((hisr != NU_NULL) && (hisr -> tc_id == TC_HISR_ID))
    {
        /* Protect against scheduling changes.  */
        TCCT_Schedule_Lock();

        /* The HISR pointer is successful.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the hisr's name.  */
        strncpy(name, hisr -> tc_name, NU_MAX_NAME);

        /* Setup the remaining fields.  */
        *scheduled_count =  hisr -> tc_scheduled;
        *priority =         hisr -> tc_priority;
        *stack_base =       hisr -> tc_stack_start;
        *stack_size =       hisr -> tc_stack_size;
        *minimum_stack =    hisr -> tc_stack_minimum;

        /* Release protection.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the HISR pointer is invalid.   */
        completion =  NU_INVALID_HISR;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
