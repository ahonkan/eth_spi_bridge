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
*       dmf_info.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains routines to obtain Information about
*       the Dynamic Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Memory_Pool_Information          Retrieve memory pool info
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       dynamic_memory.h                    Dynamic memory functions
*
************************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Memory_Pool_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified memory
*       pool.  However, if the supplied memory pool pointer is
*       invalid, the function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*       TCCT_Schedule_Lock                  Protect memory pool
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pool_ptr                            Pointer to the memory pool
*       name                                Destination for the name
*       start_address                       Destination for the starting
*                                           memory address of the pool
*       pool_size                           Destination for the pool's
*                                           total size
*       min_allocation                      Destination for the minimum
*                                           block allocation size
*       available                           Destination for the available
*                                           number of bytes in pool
*       suspend_type                        Destination for the type of
*                                           suspension
*       tasks_waiting                       Destination for the tasks
*                                           waiting count
*       first_task                          Destination for the pointer
*                                           to the first task waiting
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If a valid pool pointer
*                                           is supplied
*           NU_INVALID_POOL                 If pool pointer invalid
*
***********************************************************************/
STATUS NU_Memory_Pool_Information(NU_MEMORY_POOL *pool_ptr, CHAR *name,
                                  VOID **start_address, UNSIGNED *pool_size,
                                  UNSIGNED *min_allocation, UNSIGNED *available,
                                  OPTION *suspend_type, UNSIGNED *tasks_waiting,
                                  NU_TASK **first_task)
{
    DM_PCB          *pool;                  /* Pool control block ptr    */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input pool pointer into internal pointer.  */
    pool =  (DM_PCB *) pool_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this memory pool id is valid.  */
    if ((pool != NU_NULL) && (pool -> dm_id == DM_DYNAMIC_ID))
    {
        /* Setup protection of the memory pool.  */
        TCCT_Schedule_Lock();

        /* The memory pool pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the memory pool's name.  */
        strncpy(name, pool -> dm_name, NU_MAX_NAME);

        /* Determine the suspension type.  */
        if (pool -> dm_fifo_suspend)
        {
            *suspend_type =          NU_FIFO;
        }
        else
        {
            *suspend_type =          NU_PRIORITY;
        }

        /* Retrieve information directly out of the control structure.  */
        *start_address =        pool -> dm_start_address;
        *pool_size =            pool -> dm_pool_size;
        *min_allocation =       pool -> dm_min_allocation;
        *available =            pool -> dm_available;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  pool -> dm_tasks_waiting;
        if (pool -> dm_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task =  (NU_TASK *)
                (pool -> dm_suspension_list) -> dm_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection of the memory pool.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the memory pool pointer is invalid.   */
        completion =  NU_INVALID_POOL;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
