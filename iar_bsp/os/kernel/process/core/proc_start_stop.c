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
*       proc_start_stop.c
*
* COMPONENT
*
*       Nucleus Processes - Core
*
* DESCRIPTION
*
*       This file contains implementations of NU_Start and NU_Stop APIs
*
* FUNCTIONS
*
*       NU_Start
*       NU_Stop
*       NU_Kill
*       PROC_Start
*       PROC_Stop
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
*   FUNCTION
*
*       NU_Start
*
*   DESCRIPTION
*
*       Starts the specified process
*
*   INPUTS
*
*       pid                 ID of process to start
*       args                Additional arguments (unused at this time)
*       suspend             Suspend on send/receive kernel thread messages
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully started
*       NU_INVALID_PROCESS  ID does not point to valid process
*
*************************************************************************/
STATUS NU_Start(INT pid, VOID * args, UNSIGNED suspend)
{
    STATUS status = NU_SUCCESS;


    /* Reference unused parameters to avoid warnings */
    NU_UNUSED_PARAM(args);

    /* Kernel process cannot be started with this API */
    NU_ERROR_CHECK((pid == PROC_KERNEL_ID), status, NU_INVALID_PROCESS);

    if (status == NU_SUCCESS)
    {
        /* Transition the process to start it */
        status = PROC_Transition(pid, PROC_STARTING_STATE, 0, suspend);
    }

    /* Return status to caller */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_Stop
*
*   DESCRIPTION
*
*       Stops the specified process
*
*   INPUTS
*
*       pid                 ID of process to stop
*       exit_code           Exit code for process
*       suspend             Suspend on send/receive kernel thread messages
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully stopped
*       NU_INVALID_PROCESS  ID does not point to valid process
*
*************************************************************************/
STATUS NU_Stop(INT pid, INT exit_code, UNSIGNED suspend)
{
    STATUS      status = NU_SUCCESS;


    /* Kernel process cannot be stopped */
    NU_ERROR_CHECK((pid == PROC_KERNEL_ID), status, NU_INVALID_PROCESS);

    if (status == NU_SUCCESS)
    {
        /* Transition the process to stop it */
        status = PROC_Transition(pid, PROC_DEINITIALIZING_STATE, exit_code, suspend);
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Kill
*
*   DESCRIPTION
*
*       Stops and unloads the specified process
*
*   INPUTS
*
*       pid                 ID of process to kill
*       suspend             Suspend on send/receive kernel thread messages
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully killed
*
*************************************************************************/
STATUS  NU_Kill(INT pid, UNSIGNED suspend)
{
    STATUS status = NU_SUCCESS;

    /* Kernel process cannot be killed */
    NU_ERROR_CHECK((pid == PROC_KERNEL_ID), status, NU_INVALID_PROCESS);

    if (status == NU_SUCCESS)
    {
        /* Transition the process to kill it */
        status = PROC_Transition(pid, PROC_KILLING_STATE, EXIT_KILL, suspend);
    }

    /* Return status to caller */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_Start
*
*   DESCRIPTION
*
*       Starts the specified process
*
*   INPUTS
*
*       process             Pointer to process to start
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully started
*
*************************************************************************/
STATUS PROC_Start(PROC_CB *process)
{
    STATUS   status;

    /* If the pool pointer is setup create the pool */
    if (process -> pool != NU_NULL)
    {
        /* Create the memory pool */
        status = NU_Create_Memory_Pool(process -> pool, "heap",
                                       (VOID *)((UNSIGNED)process -> pool + sizeof(NU_MEMORY_POOL)),
                                       process -> heap_size - sizeof(NU_MEMORY_POOL), PROC_HEAP_MIN_ALLOC, NU_FIFO);
    }

    if (status == NU_SUCCESS)
    {
        /* Terminate the task, regardless of current state */
        (VOID)NU_Terminate_Task(&(process -> root_task));

        /* Reset task with proper parameters */
        status = NU_Reset_Task(&(process -> root_task), PROC_CMD_START, NU_NULL);
    }

    if (status == NU_SUCCESS)
    {
        /* Clear the BSS. */
        memset(process -> bss_start, 0, process -> bss_size);

        /* Initialize the data section using the initdata recorded at load time. */
        memcpy(process -> data_start, process -> initdata_start, process -> data_size);

        /* Reset exit code */
        process -> exit_code = 0;

        /* Resume the task */
        status = NU_Resume_Task(&(process -> root_task));
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PROC_Stop
*
*   DESCRIPTION
*
*       Stops the specified process
*
*   INPUTS
*
*       process             Pointer to process to stop
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully stopped
*
*************************************************************************/
STATUS PROC_Stop(PROC_CB *process)
{
    STATUS      status = NU_SUCCESS;
    CS_NODE    *node;
    NU_TASK    *task;

    /* Get first node of the task list */
    node = process -> tasks;

    /* Loop through all of the tasks in the process and remove
       all but the root thread */
    while (node != NU_NULL)
    {
        /* Get the address of the task */
        task = NU_STRUCT_BASE(node, tc_proc_node, NU_TASK);

        /* Get the next node now before the list is updated again
           by the call to delete task */
        node = node -> cs_next;

        /* Check if the task is the root task */
        if (task != &(process -> root_task))
        {
            /* Terminate the task, regardless of current state.
               The root task isn't terminated here as it could
               be encountered more than once before leaving the
               loop */
            status = NU_Terminate_Task(task);
            if (status == NU_SUCCESS)
            {
                /* Delete the task, tasks will be recreated by the
                   process's main function */
                status = NU_Delete_Task(task);
            }
        }

        /* Determine if the end of the list has been found */
        if ((process -> total_tasks == 1) || (status != NU_SUCCESS))
        {
            /* Only root task is left, exit the loop */
            node = NU_NULL;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Terminate the task, regardless of current state */
        (VOID)NU_Terminate_Task(&(process -> root_task));

        /* Call the stop thread */
        status = NU_Reset_Task(&(process -> root_task), PROC_CMD_STOP, (VOID *)(process -> abort_flag));
        if (status == NU_SUCCESS)
        {
            /* Mark the process as protected during exit */
            process -> exit_protect = NU_TRUE;

            /* Resume the task */
            status = NU_Resume_Task(&(process -> root_task));
        }
    }

    /* Return status to caller */
    return (status);
}
