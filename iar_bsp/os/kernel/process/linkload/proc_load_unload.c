/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       proc_load_unload.c
*
*   COMPONENT
*
*       Nucleus Processes - Linker / Loader
*
*   DESCRIPTION
*
*       Implements the NU_Load and NU_Unload APIs
*
*   FUNCTIONS
*
*       proc_load
*       NU_Load
*       NU_Unload
*       PROC_Load
*       PROC_Unload
*
*************************************************************************/

#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "storage/nu_storage.h"
#include "os/kernel/process/core/proc_core.h"
#include "kernel/proc_extern.h"
#include "proc_reloclink.h"
#include "os/kernel/process/mem_mgmt/proc_mem_mgmt.h"
#include "proc_elf.h"


/*************************************************************************
*
*   FUNCTION
*
*       NU_Load
*
*   DESCRIPTION
*
*       Loads the specified process ELF image as user mode process to memory and
*       performs relocation of the ELF image
*
*   INPUTS
*
*       name                Name of ELF image to load
*       pid                 ID of process loaded and linked
*       load_addr           Address where to load image and where image loaded
*       extension           Other parameters for ELF image loading
*       suspend             Suspend on send/receive kernel thread messages
*
*
*   OUTPUTS
*
*       NU_SUCCESS              If process successfully loaded and linked
*       NU_UNAVAILABLE          Invalid load base
*       NU_INVALID_POINTER      Invalid name (file name) or return pointer
*                               for ID
*       NU_INVALID_OPTIONS      Invalid name length (larger than maximum
*                               name length for a process).
*       NU_INVALID_OPERATION    Indicates an attempt to load a kernel-mode
*                               process from user-mode.
*
*************************************************************************/
STATUS  NU_Load(CHAR * name, INT * pid, VOID * load_addr, VOID * extension, UNSIGNED suspend)
{
    STATUS              status = NU_SUCCESS;
    PROC_CB *           process;
    NU_LOAD_EXTENSION * ext_ptr;
    PROC_CB *           current_process;
    UNSIGNED            heap_size;
    UNSIGNED            stack_size;
    BOOLEAN             kernel_mode;

    /* Optional error checking */
    NU_ERROR_CHECK((load_addr != NU_LOAD_DYNAMIC), status, NU_UNAVAILABLE);
    NU_ERROR_CHECK((name == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK((strlen(name) > (PROC_NAME_LENGTH - 1)), status, NU_INVALID_OPTIONS);
    NU_ERROR_CHECK((pid == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Check for load extensions. */
        if (extension != NU_NULL)
        {
            /* Get pointer to extension structure */
            ext_ptr = (NU_LOAD_EXTENSION *)extension;

            /* Ensure loading kernel-mode from kernel-mode only. */
            current_process = PROC_Get_Pointer(NU_Getpid());
            if ((ext_ptr -> kernel_mode == NU_TRUE) &&
                (current_process -> kernel_mode == NU_FALSE))
            {
                /* ERROR: Cannot load kernel-mode from user-mode. */
                status = NU_INVALID_OPERATION;
            }

            if (status == NU_SUCCESS)
            {
                /* Set heap size to extension setting if non-zero
                   otherwise set to default value. */
                heap_size = ext_ptr -> heap_size;
                if (heap_size == 0)
                {
                    heap_size = PROC_HEAP_SIZE;
                }

                /* Set stack size to extension setting if non-zero
                   otherwise set to default value. */
                stack_size = ext_ptr -> stack_size;
                if (stack_size == 0)
                {
                    stack_size = PROC_ROOT_STACK_SIZE;
                }

                /* Set kernel-mode to extension setting. */
                kernel_mode = ext_ptr -> kernel_mode;
            }
        }
        else
        {
            /* Set default values. */
            heap_size = PROC_HEAP_SIZE;
            stack_size = PROC_ROOT_STACK_SIZE;
            kernel_mode = NU_FALSE;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Create a process */
        status = PROC_Initialize(&process, name);

        /* Verify process was created correctly */
        if (status == NU_SUCCESS)
        {
            /* Setup return value for ID */
            *pid = process -> id;

            /* Set process attributes. */
            process -> kernel_mode = kernel_mode;

#if	(PROC_PAGE_SIZE > 0)

            /* Page align the heap size. */
            process -> heap_size = (UINT32)ESAL_GE_MEM_PTR_ALIGN(heap_size, PROC_PAGE_SIZE);

            /* Page align the root task stack size. */
            process -> stack_size = (UINT32)ESAL_GE_MEM_PTR_ALIGN(stack_size, PROC_PAGE_SIZE);

#else

            process -> heap_size = heap_size;
            process -> stack_size = stack_size;

#endif	/* (PROC_PAGE_SIZE > 0) */

            /* Create the semaphore */
            status = NU_Create_Semaphore(&(process -> semaphore), "process", 1, NU_PRIORITY_INHERIT);
        }

        /* Create the queue */
        if (status == NU_SUCCESS)
        {
            status = NU_Create_Queue(&(process -> queue), "process", &(process -> buffer),
                                     PROC_QUEUE_SIZE, NU_FIXED_SIZE, PROC_MSG_SIZE, NU_FIFO);
        }

        /* Ensure minimal system for transitions is complete */
        if (status == NU_SUCCESS)
        {
            /* Transition to load the process */
            status = PROC_Transition(*pid, PROC_LOADING_STATE, 0, suspend);
        }
    }

    /* Return status to caller */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_Unload
*
*   DESCRIPTION
*
*       Unloads the specified process from memory
*
*   INPUTS
*
*       pid                 ID of process to unload
*       suspend             Suspend on send/receive kernel thread messages
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully loaded and linked
*       NU_INVALID_PROCESS  ID does not point to valid process
*
*************************************************************************/
STATUS  NU_Unload(INT pid, UNSIGNED suspend)
{
    STATUS   status = NU_SUCCESS;

    /* Kernel process cannot be unloaded */
    NU_ERROR_CHECK((pid == PROC_KERNEL_ID), status, NU_INVALID_PROCESS);

    if (status == NU_SUCCESS)
    {
        /* Transition the state and allow the process to be unloaded */
        status = PROC_Transition(pid, PROC_UNLOADING_STATE, 0, suspend);
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_Load
*
*   DESCRIPTION
*
*       Loads the specified process ELF image to memory and
*       performs relocation of the ELF image
*
*   INPUTS
*
*       name                Name of ELF image to load
*       id                  ID of process loaded and linked
*       load_addr           Address where to load image and where image loaded
*       extension           Other parameters for ELF image loading
*
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully loaded and linked
*
*************************************************************************/
STATUS PROC_Load(PROC_CB *process)
{
    STATUS              status = NU_SUCCESS;
    INT                 fd;
    UINT8 *             buffer;
    UINT32              entry_addr;
    NU_SYMBOL_ENTRY    *symbol_table;
    NU_SYMBOL_ENTRY    *ksymbol_table;
    VOID               *root_stack;

    /* Try to open the specified file */
    fd = NU_Open(process -> name, PO_RDONLY, PS_IREAD);

    /* Ensure file was found */
    if (fd >= 0)
    {
        status = PROC_ELF_File_Load(process, fd, (VOID *)&buffer, (VOID *)&entry_addr,
                                    &root_stack, process -> stack_size, process -> heap_size,
                                    PROC_PAGE_SIZE, &symbol_table, &ksymbol_table);

        /* Ensure successful */
        if (status == NU_SUCCESS)
        {
            /* Set the heap control block to the base of the heap */
            process -> pool = (NU_MEMORY_POOL *)((UNSIGNED)root_stack + process -> stack_size);

            /* clear the memory for the pool control block */
            memset(process -> pool, 0, sizeof(NU_MEMORY_POOL));

            /* Set Nucleus symbols table address */
            process -> symbols = symbol_table;
            process -> ksymbols = ksymbol_table;

            /* Set module entry and exit points */
            process -> entry_addr = (NU_PROC_ENTRY)entry_addr;

            /* Set module load address */
            process -> load_addr = (VOID *)buffer;

            /* Create task to serve as the root task, pass the process to
               argv so that the binding occurs properly */
            status = NU_Create_Task(&(process -> root_task), "root",
                                    (VOID (*)(UNSIGNED, VOID *))(process -> entry_addr),
                                    1, (VOID *)process, root_stack, process -> stack_size,
                                    PROC_ROOT_PRIORITY, PROC_ROOT_TIMESLICE, NU_PREEMPT,
                                    NU_NO_START);

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
            if (status == NU_SUCCESS)
            {
                /* Setup the memory regions */
                status = PROC_Create_Process_Memory(process);
            }
#endif
        }
    }
    else
    {
        /* Clean up the already created elements */

        /* Remove the queue, ignore errors */
        (VOID)NU_Delete_Queue(&(process -> queue));

        /* Remove the semaphore, ignore errors */
        (VOID)NU_Delete_Semaphore(&(process -> semaphore));

        /* Delete the process */
        (VOID)PROC_Release_CB(process);

        /* Return file not found */
        status = fd;
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PROC_Unload
*
*   DESCRIPTION
*
*       Unloads the specified process from memory
*
*   INPUTS
*
*       id                  ID of process to unload
*
*   OUTPUTS
*
*       NU_SUCCESS          If process successfully loaded and linked
*
*************************************************************************/
STATUS PROC_Unload(PROC_CB *process)
{
    STATUS  status;

    /* Remove the root thread, ignore errors */
    (VOID)NU_Terminate_Task(&(process -> root_task));
    (VOID)NU_Delete_Task(&(process -> root_task));

    /* Remove the queue, ignore errors */
    (VOID)NU_Delete_Queue(&(process -> queue));

    /* Remove the semaphore, ignore errors */
    (VOID)NU_Release_Semaphore(&(process -> semaphore));
    (VOID)NU_Delete_Semaphore(&(process -> semaphore));

    /* Deallocate memory for process, ignore errors */
    (VOID)PROC_Free(process -> load_addr);

    /* Delete the process */
    status = PROC_Delete(process);

    /* Returning to the kernel thread which will suspend
       and wait on another transition, this process will not
       return */

    return (status);
}
