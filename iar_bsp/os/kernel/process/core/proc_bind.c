/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       proc_bind.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file functionality required to bind threads to processes
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Bind_Task
*       PROC_Bind_HISR
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*       thread_control.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "proc_core.h"
#include "os/kernel/plus/core/inc/thread_control.h"

/*************************************************************************
*
* FUNCTION
*
*       PROC_Bind_Task
*
* DESCRIPTION
*
*       Binds the task passed to the currently scheduled process.  In the
*       case the current task is the root kernel image the task being
*       bound should have a valid process in the task control block via
*       argv.
*
* INPUTS
*
*       task_ptr                        Task to be bound
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_PROCESS              Process pointer in argv is invalid
*       NU_INVALID_POINTER              Process pointer in argv is null
*
*************************************************************************/
STATUS PROC_Bind_Task(NU_TASK *task_ptr)
{
    PROC_CB *process;
    STATUS   status = NU_SUCCESS;

    /* Error check the task control block */
    NU_ERROR_CHECK((task_ptr == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Determine if this is the root thread of a new process,
           The root thread can only be created by the kernel process
           thread, if so the process to bind to will be placed in argv */
        if (TCD_Current_Thread == (VOID*)&(PROC_Kernel_CB -> root_task))
        {
            /* argv will point to the process to use */
            process = (PROC_CB *)task_ptr -> tc_argv;

            /* Verify the process */
            NU_ERROR_CHECK((process == NU_NULL), status, NU_INVALID_POINTER);
            NU_ERROR_CHECK((process -> valid != PROC_CB_ID), status, NU_INVALID_PROCESS);
        }
        else
        {
            /* Use current process */
            process = PROC_Scheduled_CB;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Lock the process */
        TCCT_Schedule_Lock();

        /* Add the task to the process list */
        NU_Place_On_List(&(process -> tasks),
                         &(task_ptr -> tc_proc_node));

        /* Increment the total number of tasks */
        process -> total_tasks++;

        /* Update the pointer in the task control block */
        task_ptr -> tc_process = process;

        /* Release the lock */
        TCCT_Schedule_Unlock();
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Bind_HISR
*
* DESCRIPTION
*
*       Binds the HISR passed to the kernel process.
*
* INPUTS
*
*       hisr_ptr                        HISR to be bound
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_POINTER              hisr_ptr is invalid
*
*************************************************************************/
STATUS PROC_Bind_HISR(NU_HISR *hisr_ptr)
{
    STATUS status = NU_SUCCESS;

    /* Error check the HISR control block */
    NU_ERROR_CHECK((hisr_ptr == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Update the pointer in the task control block */
        hisr_ptr -> tc_process = (VOID *)PROC_Kernel_CB;
    }

    return(status);
}
