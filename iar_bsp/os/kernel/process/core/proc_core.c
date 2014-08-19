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
*       proc_core.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains the basic common functionality of the core
*       component.  This functionality includes process creation and
*       obtaining common information.
*
* DATA STRUCTURES
*
*       PROC_Scheduled_CB
*       PROC_Kernel_CB
*       PROC_Created_List
*       PROC_Total_Created
*       PROC_CB_Array
*
* FUNCTIONS
*
*       proc_info_get
*       PROC_Get_Pointer
*       PROC_Initialize
*       PROC_Release_CB
*       PROC_Get_Name
*       PROC_Get_Memory_Pool
*       NU_Getpid
*       NU_Get_Exit_Code
*       NU_Established_Processes
*       NU_Processes_Information
*       NU_Process_Information
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       thread_control
*       proc_core.h
*       string.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "proc_core.h"
#include <string.h>

/* Pointer the currently executing process. */
PROC_CB            *PROC_Scheduled_CB;

/* This is the kernel process control block */
PROC_CB            *PROC_Kernel_CB;

/* PROC_Created_List is the head pointer of the linked list of
   created processes.  If the list is NU_NULL, there are
   no processes created. */
CS_NODE            *PROC_Created_List;

/* PROC_Total_Created contains the number of currently created
   processes. */
UNSIGNED            PROC_Total_Created;

/* PROC_CB_Array is the physical location for all processes in the
   system */
static PROC_CB PROC_CB_Array[CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES];

#if (CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       PROC_Load_Notify
*
* DESCRIPTION
*
*       Function call that notifies a debugger that a process has been
*       loaded.
*
* INPUTS
*
*       name - The name of the loaded process.
*
*       id - The ID of the process that has been loaded.
*
*       load_addr - The address that the process has been loaded at.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PROC_Load_Notify(CHAR * name, UINT id, UINT load_addr)
{
    return;
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Unload_Notify
*
* DESCRIPTION
*
*       Function call that notifies a debugger that a process has been
*       unloaded.
*
* INPUTS
*
*       id - The ID of the process that has been unloaded.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PROC_Unload_Notify(UINT id)
{
    return;
}

/* Development support notification function pointers */
VOID (*PROC_Load_Notify_Ptr)(CHAR *, UINT, UINT) = PROC_Load_Notify;
VOID (*PROC_Unload_Notify_Ptr)(UINT) = PROC_Unload_Notify;

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT */

/*************************************************************************
*
* FUNCTION
*
*       proc_info_get
*
* DESCRIPTION
*
*       Fills in a process information structure from a given process
*
* INPUTS
*
*       process                             Pointer to process structure
*       info                                Pointer to information structure
*
* OUTPUTS
*
*       None
*
*************************************************************************/
static VOID proc_info_get(PROC_CB * process, NU_PROCESS_INFO * info)
{
    /* Save data for this process into the info array */
    info->pid = process->id;
    strcpy(info->name, process->name);
    info->entry_addr = process->entry_addr;
    info->load_addr = process->load_addr;
    info->state = process->state;
    info->exit_code = process->exit_code;
    info->kernel_mode = process->kernel_mode;
}


/*************************************************************************
*
* FUNCTION
*
*       PROC_Get_Pointer
*
* DESCRIPTION
*
*       Uses passed in value to index the process array and return pointer
*       to the process control block
*
* INPUTS
*
*       id                              ID for process control block
*
* OUTPUTS
*
*       Pointer to process control block
*
*************************************************************************/
PROC_CB *PROC_Get_Pointer(INT id)
{
    PROC_CB *proc_ptr;

    /* Initialize pointer to null */
    proc_ptr = NU_NULL;

    /* Verify the ID is in range */
    if (id < CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES)
    {
        /* Verify the process is valid */
        if (PROC_CB_Array[id].valid == PROC_CB_ID)
        {
            proc_ptr = &PROC_CB_Array[id];
        }
    }

    return(proc_ptr);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Initialize
*
* DESCRIPTION
*
*       Creates a process control block.  Allocates from array, updates
*       the total number of processes and returns a pointer to the newly
*       created control block.
*
* INPUTS
*
*       process                         Return pointer to the newly created
*                                       process control block
*       name                            Name to help identify process
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_POINTER              Unusable pointer to return process
*       NU_UNAVAILABLE                  Indicates no processes are available
*
*************************************************************************/
STATUS PROC_Initialize(PROC_CB **process, CHAR *name)
{
    STATUS status = NU_SUCCESS;
    INT    proc_index = PROC_KERNEL_ID;
    INT    saved_index = 0;

    /* Verify the return process pointer */
    NU_ERROR_CHECK((process == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK((name == NU_NULL), status, NU_INVALID_POINTER);

    /* Obtain a unique process ID, and pointer to the control block */
    if (status == NU_SUCCESS)
    {
        /* Lock the process creation */
        TCCT_Schedule_Lock();

        /* If we haven't exceeded the max number of processes we need to
           find an available control block */
        if ((PROC_Total_Created < CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES) && (status == NU_SUCCESS))
        {
            /* Reset status to unavailable until a process ID is found */
            status = NU_UNAVAILABLE;

            /* Loop through the index array until an available index is found */
            for (proc_index = 0; ((proc_index < CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES) && (status == NU_UNAVAILABLE)); proc_index++)
            {
                /* Is this index available? */
                if (PROC_CB_Array[proc_index].valid != PROC_CB_ID)
                {
                    /* Obtain an actual control block from the array */
                    *process = &PROC_CB_Array[proc_index];

                    /* Update status to exit the loop */
                    status = NU_SUCCESS;

                    /* Save the index for process ID usage */
                    saved_index = proc_index;
                }
            }

            if (status == NU_SUCCESS)
            {
                /* Clear the memory */
                memset((*process), 0, sizeof(PROC_CB));

                /* Update global list and count to include new control block
                   process will not be valid until internal ID is set */
                NU_Place_On_List(&PROC_Created_List, &((*process) -> created));
                PROC_Total_Created += 1;

                /* Finalize the control block elements and return
                   the new process ID */
                (*process) -> id = saved_index;

                /* Copy name to control block */
                strncpy((*process) -> name, name, (PROC_NAME_LENGTH - 1));

                /* Make this a valid process, this will also mark the
                   process in the array as used. */
                (*process) -> valid = PROC_CB_ID;
            }
        }
        else
        {
            /* No more processes are available */
            status = NU_UNAVAILABLE;
        }

        /* Unlock the process creation critical section */
        TCCT_Schedule_Unlock();
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Release_CB
*
* DESCRIPTION
*
*       Removes a process control block, updates the
*       total number of processes, and frees the ID.
*
* INPUTS
*
*       process
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_PROCESS              Indicates bad process pointer
*       NU_INVALID_POINTER              Passed null for pointer
*
*************************************************************************/
STATUS PROC_Release_CB(PROC_CB *process)
{
    STATUS status = NU_SUCCESS;

    /* Check for proper process */
    NU_ERROR_CHECK((process == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK((process -> valid != PROC_CB_ID), status, NU_INVALID_PROCESS);
    NU_ERROR_CHECK((process == PROC_Kernel_CB), status, NU_INVALID_PROCESS);

    if (status == NU_SUCCESS)
    {
        /* Additional error checking */
        NU_ERROR_CHECK((process -> id >= CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES), status, NU_INVALID_PROCESS);
        NU_ERROR_CHECK((&PROC_CB_Array[process -> id] != process), status, NU_INVALID_PROCESS);
    }

    if (status == NU_SUCCESS)
    {
        /* Lock the process deletion */
        TCCT_Schedule_Lock();

        /* Mark as invalid, this will also
           mark the process as available in the array */
        process -> valid = 0;

        /* Remove from the list */
        NU_Remove_From_List(&PROC_Created_List, &(process -> created));
        PROC_Total_Created -= 1;

        /* Unlock the process deletion critical section */
        TCCT_Schedule_Unlock();
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Get_Name
*
* DESCRIPTION
*
*       Gets the name of the process
*
* INPUTS
*
*       id                              ID to get name from
*       name                            Pointer to return name
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_PROCESS              ID is not valid, or process has
*                                       already started
*       NU_INVALID_POINTER              Entry or Exit point is not within
*                                       executable memory of the process
*
*************************************************************************/
STATUS PROC_Get_Name(INT id, CHAR ** name)
{
    STATUS   status = NU_SUCCESS;
    PROC_CB *process;

    /* Get the process control block for given id */
    process = PROC_Get_Pointer(id);

    if (process != NU_NULL)
    {
        /* Get pointer to name */
        *name = process -> name;
    }
    else
    {
        status = NU_INVALID_PROCESS;
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*      PROC_Get_Memory_Pool
*
* DESCRIPTION
*
*      This service reads the memory pool specific to the current process.
*
* INPUTS
*
*      memory_pool_ptr     Return location of the pool
*
* OUTPUTS
*
*      NU_SUCCESS          Indicates successful completion of the service
*      NU_INVALID_POINTER  Memory pool return pointer is null
*
*************************************************************************/
STATUS PROC_Get_Memory_Pool(NU_MEMORY_POOL **memory_pool_ptr)
{
    STATUS  status = NU_SUCCESS;

    /* Check the incoming pointer for null */
    if (memory_pool_ptr != NU_NULL)
    {
        /* Return the pool pointer */
        *memory_pool_ptr = PROC_Scheduled_CB -> pool;
    }
    else
    {
        /* Return error for null pointer */
        status = NU_INVALID_POINTER;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_Getpid
*
* DESCRIPTION
*
*       Gets the process ID of the caller
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       INT                                 Process ID
*
*************************************************************************/
INT NU_Getpid(VOID)
{
    /* Return current process ID to caller */
    return (PROC_Scheduled_CB->id);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Get_Exit_Code
*
* DESCRIPTION
*
*       Gets the exit code of the specified process
*
* INPUTS
*
*       pid                                 Process ID
*       exit_code                           Pointer to exit code
*
* OUTPUTS
*
*       NU_SUCCESS                          Exit code returned
*       other                               Exit code not returned
*
*************************************************************************/
STATUS  NU_Get_Exit_Code(INT pid, INT * exit_code)
{
    STATUS      status = NU_SUCCESS;
    PROC_CB *   process;


    /* Get the process control block for given id */
    process = PROC_Get_Pointer(pid);

    /* Ensure the process pointer was returned */
    if (process != NU_NULL)
    {
        /* Check to see if the state of the process is stopped and no abort() was
           requested - this is the only state where the exit code is valid */
        if ((process->state == PROC_STOPPED_STATE) && (process->abort_flag != NU_TRUE))
        {
            /* Get exit code from process */
            *exit_code = process->exit_code;
        }
        else
        {
            /* Set error value */
            status = NU_INVALID_STATE;
        }
    }
    else
    {
        /* Set invalid process error */
        status = NU_INVALID_PROCESS;
    }

    /* Return status to the caller */
    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Established_Processes
*
* DESCRIPTION
*
*       Returns the number of processes loaded in the system
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       UNSIGNED                            Number of processes (0-max unsigned)
*
*************************************************************************/
UNSIGNED    NU_Established_Processes(VOID)
{
    /* Return the number of created processes to the caller */
    return (PROC_Total_Created);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Processes_Information
*
* DESCRIPTION
*
*       Returns information on up to the max number of processes specified
*
* INPUTS
*
*       max_processes                       Maximum number of processes to get info for
*       info_array                          Array of information structures
*
* OUTPUTS
*
*       NU_SUCCESS                          Info on processes returned
*       NU_INVALID_POINTER                  Invalid pointers passed to function
*
*************************************************************************/
STATUS   NU_Processes_Information(UNSIGNED * max_processes, NU_PROCESS_INFO info_array[])
{
    STATUS              status = NU_SUCCESS;
    PROC_CB *           current_process;
    UNSIGNED            proc_count = 0;


    /* Check for errors in parameters */
    NU_ERROR_CHECK(((info_array == NU_NULL) || (max_processes == NU_NULL)), status, NU_INVALID_POINTER);

    /* Continue if max processes is not 0 and no param errors */
    if ((*max_processes != 0) && (status == NU_SUCCESS))
    {
        /* Lock the process list traversal */
        TCCT_Schedule_Lock();

        /* Get the first process */
        current_process = PROC_GET_FIRST();

        /* Loop through all processes */
        do
        {
            /* Save data for this process into the info array */
            proc_info_get(current_process, &info_array[proc_count]);

            /* Increment the process count */
            proc_count++;

            /* Decrement max number of processes available in array */
            (*max_processes)--;

            /* Move to the next process. */
            current_process = PROC_GET_NEXT(current_process);

        /* Loop until back to start of list or max processes reached */
        } while ((current_process != PROC_GET_FIRST()) && (*max_processes != 0));

        /* Set number of processes actually handled */
        *max_processes = proc_count;

        /* Critical section over - unlock scheduler */
        TCCT_Schedule_Unlock();
    }

    /* Return status */
    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Process_Information
*
* DESCRIPTION
*
*       Returns information single process specified
*
* INPUTS
*
*       pid                                 Process ID
*       info                                Pointer to information structures
*
* OUTPUTS
*
*       NU_SUCCESS                          Info on process returned
*       NU_INVALID_POINTER                  Invalid pointer passed to function
*       NU_INVALID_PROCESS                  Invalid process ID passed to function
*
*************************************************************************/
STATUS   NU_Process_Information(INT pid, NU_PROCESS_INFO * info)
{
    STATUS      status = NU_SUCCESS;
    PROC_CB *   process;


    /* Check for errors in parameters */
    NU_ERROR_CHECK((info == NU_NULL), status, NU_INVALID_POINTER);

    /* Ensure success */
    if (status == NU_SUCCESS)
    {
        /* Lock the process list traversal */
        TCCT_Schedule_Lock();

        /* Get the process control block for given id */
        process = PROC_Get_Pointer(pid);

        /* Ensure the process pointer was returned */
        if (process != NU_NULL)
        {
            /* Save data for this process into the info structure */
            proc_info_get(process, info);
        }
        else
        {
            /* Return status showing invalid process */
            status = NU_INVALID_PROCESS;
        }

        /* Critical section over - unlock scheduler */
        TCCT_Schedule_Unlock();
    }

    /* Return status */
    return (status);
}


/* Export process interfaces */
NU_EXPORT_SYMBOL(NU_Getpid);
NU_EXPORT_SYMBOL(NU_Get_Exit_Code);
NU_EXPORT_SYMBOL(NU_Established_Processes);
NU_EXPORT_SYMBOL(NU_Processes_Information);
NU_EXPORT_SYMBOL(NU_Process_Information);
