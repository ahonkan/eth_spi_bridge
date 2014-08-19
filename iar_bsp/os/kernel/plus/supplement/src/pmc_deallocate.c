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
*       pmc_deallocate.c
*
*   COMPONENT
*
*       PM - Partition Memory Management
*
*   DESCRIPTION
*
*       This file contains the Deallocate core routine for the
*       Partition Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Deallocate_Partition             Deallocate a partition from
*                                           a pool
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       partition_memory.h                  Partition functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/supplement/inc/partition_memory.h"
#include        "services/nu_trace_os_mark.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Deallocate_Partition
*
*   DESCRIPTION
*
*       This function deallocates a previously allocated partition.  If
*       there is a task waiting for a partition, the partition is simply
*       given to the waiting task and the waiting task is resumed.
*       Otherwise, the partition is returned to the partition pool.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove from suspend list
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect partition pool
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       partition                           Pointer to partition memory
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           NU_INVALID_POINTER              Indicates the supplied
*                                           partition pointer is NULL,
*                                           or otherwise invalid.
*
***********************************************************************/
STATUS NU_Deallocate_Partition(VOID *partition)
{
    R1 PM_PCB       *pool;                  /* Pool pointer              */
    R3 PM_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    R2 PM_HEADER    *header_ptr;            /* Pointer to partition header  */
    STATUS          preempt;                /* Preemption flag           */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    NU_ERROR_CHECK((partition == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Pickup the associated pool's pointer.  It is inside the header of
           each partition.  */
        header_ptr =  (PM_HEADER *) (((BYTE_PTR) partition) - PM_OVERHEAD);
    }

    /* Determine if the pointer are NULL */
    NU_ERROR_CHECK(((header_ptr == NU_NULL) || (partition == NU_NULL)), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
    	pool =  header_ptr -> pm_partition_pool;

    	/* Determine if partition pool pointer is valid */
    	NU_ERROR_CHECK((pool == NU_NULL), status, NU_INVALID_POINTER);
    }

    /* Verify the partition pool pointer is valid */
    NU_ERROR_CHECK((pool -> pm_id != PM_PARTITION_ID), status, NU_INVALID_POINTER);

    /* Verify the partition is allocated */
    NU_ERROR_CHECK((header_ptr -> pm_next_available != (PM_HEADER *)PM_PARTITION_ALLOCATED), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Pickup the associated pool's pointer.  It is inside the header of
           each partition.  */
        pool =        header_ptr -> pm_partition_pool;

        /* Protect against simultaneous access to the partition pool.  */
        TCCT_Schedule_Lock();

        /* Determine if another task is waiting for a partition from the pool.  */
        if (pool -> pm_tasks_waiting)
        {
            /* Yes, another task is waiting for a partition from the pool.  */

            /* Decrement the number of tasks waiting counter.  */
            pool -> pm_tasks_waiting--;

            /* Remove the first suspended block from the list.  */
            suspend_ptr =  pool -> pm_suspension_list;
            NU_Remove_From_List((CS_NODE **) &(pool -> pm_suspension_list),
                                &(suspend_ptr -> pm_suspend_link));

            /* Setup the appropriate return value.  */
            suspend_ptr -> pm_return_status =   NU_SUCCESS;
            suspend_ptr -> pm_return_pointer =  partition;

            /* Trace log */
            T_PMEM_DEALLOCATE((VOID*)pool, partition, ESAL_GET_RETURN_ADDRESS(0),
                              (pool->pm_pool_size), (pool->pm_partition_size),
                              (pool->pm_available), (pool->pm_allocated), OBJ_UNBLKD_CTXT);

            /* Resume the suspended task.  */
            preempt =
               TCC_Resume_Task((NU_TASK *) suspend_ptr -> pm_suspended_task,
                                                           NU_PARTITION_SUSPEND);

            /* Determine if a preempt condition is present.  */
            if (preempt)
            {
                /* Trace log */
                T_TASK_READY((VOID*)TCCT_Current_Thread());

                /* Transfer control to the system if the resumed task function
                   detects a preemption condition.  */
                TCCT_Control_To_System();
            }
        }
        else
        {

            /* Increment the available partitions counter.  */
            pool -> pm_available++;

            /* Decrement the allocated partitions counter.  */
            pool -> pm_allocated--;

            /* Place the partition back on the available list.  */
            header_ptr -> pm_next_available =  pool -> pm_available_list;
            pool -> pm_available_list =        header_ptr;

        }

        /* Trace log */
        T_PMEM_DEALLOCATE((VOID*)pool, partition, ESAL_GET_RETURN_ADDRESS(0),
                          (pool->pm_pool_size), (pool->pm_partition_size), (pool->pm_available),
                          (pool->pm_allocated), OBJ_ACTION_SUCCESS);

        /* Release protection of the partition pool.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log
         * NOTE: Return 0 for total size and 0 for available memory in case pool_ptr is null */
        T_PMEM_DEALLOCATE((VOID*)pool, partition, ESAL_GET_RETURN_ADDRESS(0), 0, 0, 0, 0, status);
    }

    /* Return the completion status.  */
    return(status);
}
