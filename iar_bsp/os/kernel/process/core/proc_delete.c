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
*       proc_delete.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains functionality required to delete a process
*       from the system.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Delete
*       PROC_Unbind_Task
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*       thread_control.h
*       [proc_mem_mgmt.h]
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "proc_core.h"
#include "os/kernel/plus/core/inc/thread_control.h"

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
#include "os/kernel/process/mem_mgmt/proc_mem_mgmt.h"
#endif

/*************************************************************************
*
* FUNCTION
*
*       PROC_Delete
*
* DESCRIPTION
*
*       Removes the process from the system and frees the ID
*
* INPUTS
*
*       process
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_POINTER              Process pointer is null
*       NU_INVALID_PROCESS              Passed in value is not valid process
*
*************************************************************************/
STATUS PROC_Delete(PROC_CB *process)
{
    STATUS   status = NU_SUCCESS;

    /* Check for proper process */
    NU_ERROR_CHECK((process == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK((process -> valid != PROC_CB_ID), status, NU_INVALID_PROCESS);
    NU_ERROR_CHECK((process -> state != PROC_UNLOADING_STATE), status, NU_INVALID_DELETE);

    if (status == NU_SUCCESS)
    {
        /* Release all symbols in use by this process. */
        status = PROC_Symbols_Unuse_All(process);
    }

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
    if (status == NU_SUCCESS)
    {
        /* Delete all of the regions in this process */
        status = PROC_Delete_Process_Memory(process);
    }
#endif

    if (status == NU_SUCCESS)
    {
        /* Free the process */
        status = PROC_Release_CB(process);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Unbind_Task
*
* DESCRIPTION
*
*       Remove the task from the process it is currently bound
*
* INPUTS
*
*       task_ptr                        Pointer to task control block
*
* OUTPUTS
*
*       NU_SUCCESS                      Routine executed successfully
*       NU_INVALID_TASK                 Bad task control block passed in
*
*************************************************************************/
STATUS PROC_Unbind_Task(NU_TASK *task_ptr)
{
    STATUS      status = NU_SUCCESS;
    PROC_CB    *process;

    /* Error check the task control block */
    NU_ERROR_CHECK((task_ptr == NU_NULL), status, NU_INVALID_TASK);

    if (status == NU_SUCCESS)
    {
        /* Obtain the process control block from the task */
        process = (PROC_CB *)task_ptr -> tc_process;

        if ((process != NU_NULL) && (process -> valid == PROC_CB_ID))
        {
            /* Lock the process */
            TCCT_Schedule_Lock();

            /* Remove the old task from the process control block */
            NU_Remove_From_List(&(process -> tasks),
                                &(task_ptr -> tc_proc_node));

            /* Decrement the total number of tasks */
            process -> total_tasks--;

            /* Clear process pointer in the task. */
            task_ptr -> tc_process = NU_NULL;

            /* Release the lock */
            TCCT_Schedule_Unlock();
        }
    }

    return(status);
}
