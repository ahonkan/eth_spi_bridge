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
*       pmf_info.c
*
*   COMPONENT
*
*       PM - Partition Memory Management
*
*   DESCRIPTION
*
*       This file contains the Information routine to obtain facts about
*       the Partition Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Partition_Pool_Information       Retrieve partition pool info
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       partition_memory.h                  Partition functions
*
************************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/supplement/inc/partition_memory.h"
#include        <string.h>

/***********************************************************************
*
*   FUNCTION
*
*       NU_Partition_Pool_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified partition
*       pool.  However, if the supplied partition pool pointer is
*       invalid, the function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect partition pool
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pool_ptr                            Pointer to the partition
*                                           pool
*       name                                Destination for the name
*       start_address                       Destination for the starting
*                                           memory address of the pool
*       pool_size                           Destination for the pool's
*                                           total size
*       partition_size                      Destination for the size of
*                                           each partition
*       available                           Destination for the
*                                           available number of
*                                           partitions
*       allocated                           Destination for the number
*                                           of allocated partitions
*       suspend_type                        Destination for the type of
*                                           suspension
*       tasks_waiting                       Destination for the tasks
*                                           waiting count
*       first_task                          Destination for the pointer
*                                           to the first task waiting
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid pool pointer
*                                           is supplied
*           NU_INVALID_POOL                 If pool pointer invalid
*
***********************************************************************/
STATUS NU_Partition_Pool_Information(NU_PARTITION_POOL *pool_ptr, CHAR *name,
                                     VOID **start_address, UNSIGNED *pool_size,
                                     UNSIGNED *partition_size, UNSIGNED *available,
                                     UNSIGNED *allocated, OPTION *suspend_type,
                                     UNSIGNED *tasks_waiting, NU_TASK **first_task)
{
    PM_PCB          *pool;                  /* Pool control block ptr    */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input pool pointer into internal pointer.  */
    pool =  (PM_PCB *) pool_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this partition pool id is valid.  */
    if ((pool != NU_NULL) && (pool -> pm_id == PM_PARTITION_ID))
    {
        /* Setup protection of the partition pool.  */
        TCCT_Schedule_Lock();

        /* The partition pool pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the partition pool's name.  */
        strncpy(name, pool -> pm_name, NU_MAX_NAME);

        /* Determine the suspension type.  */
        if (pool -> pm_fifo_suspend)
        {
            *suspend_type =          NU_FIFO;
        }
        else
        {
            *suspend_type =          NU_PRIORITY;
        }

        /* Retrieve information directly out of the control structure.  */
        *start_address =        pool -> pm_start_address;
        *pool_size =            pool -> pm_pool_size;
        *partition_size =       pool -> pm_partition_size;
        *available =            pool -> pm_available;
        *allocated =            pool -> pm_allocated;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  pool -> pm_tasks_waiting;
        if (pool -> pm_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task =  (NU_TASK *)
                (pool -> pm_suspension_list) -> pm_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection of the partition pool.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the partition pool pointer is invalid.   */
        completion =  NU_INVALID_POOL;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}
