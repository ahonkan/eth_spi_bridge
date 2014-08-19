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
*       pmc_common.c
*
*   COMPONENT
*
*       PM - Partition Memory Management
*
*   DESCRIPTION
*
*       This file contains the core common routines for the
*       Partition Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Partition_Pool            Create a Partition Pool
*       NU_Allocate_Partition               Allocate a partition from a
*                                           pool
*       PMC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       common_services.h                   Common service constants
*       partition_memory.h                  Partition functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/supplement/inc/partition_memory.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define external inner-component global data references.  */

extern CS_NODE         *PMD_Created_Pools_List;
extern UNSIGNED         PMD_Total_Pools;

/* Define internal component function prototypes.  */

VOID    PMC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Partition_Pool
*
*   DESCRIPTION
*
*       This function creates a memory partition pool and then places it
*       on the list of created partition pools.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Add node to linked-list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Data structure protect
*       TCCT_Schedule_Unlock                Un-protect data structure
*
*   INPUTS
*
*       pool_ptr                            Partition pool control block
*                                           pointer
*       name                                Partition pool name
*       start_address                       Starting address of the pool
*       pool_size                           Number of bytes in the pool
*       partition_size                      Number of bytes in each
*                                           partition of the pool
*       suspend_type                        Suspension type
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_POOL                     Pool control block pointer
*                                           is NULL
*       NU_INVALID_MEMORY                   Pool starting address is
*                                           NULL
*       NU_INVALID_SIZE                     Partition size is 0 or it is
*                                           larger than the pool area
*       NU_INVALID_SUSPEND                  Suspension selection is not
*                                           valid
*       NU_NOT_ALIGNED                      Start address is not aligned
*
***********************************************************************/
STATUS NU_Create_Partition_Pool(NU_PARTITION_POOL *pool_ptr, CHAR *name,
                                VOID *start_address, UNSIGNED pool_size,
                                UNSIGNED partition_size, OPTION suspend_type)
{
    R1 PM_PCB       *pool;                  /* Pool control block ptr    */
    BYTE_PTR        pointer;                /* Working byte pointer      */
    PM_HEADER       *header_ptr;            /* Partition block header ptr*/
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pool pointer into internal pointer.  */
    pool =  (PM_PCB *) pool_ptr;

    /* Adjust the partition size to something that is evenly divisible by
       the number of bytes in an UNSIGNED data type.  */
    partition_size = ((partition_size + sizeof(UNSIGNED) - 1)/sizeof(UNSIGNED)) *
                      sizeof(UNSIGNED);

    /* Check for a NULL partition pool control block pointer or a control
       block that is already created */
    NU_ERROR_CHECK(((pool == NU_NULL) || (pool -> pm_id == PM_PARTITION_ID)), status, NU_INVALID_POOL);

    /* Check for invalid memory pointer */
    NU_ERROR_CHECK((start_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Verify the start address is aligned */
    NU_ERROR_CHECK((ESAL_GE_MEM_ALIGNED_CHECK(start_address, sizeof(UNSIGNED)) == NU_FALSE), status, NU_NOT_ALIGNED);

    /* Verify the pool could accommodate at least one partition */
    NU_ERROR_CHECK(((partition_size == 0) || ((partition_size + PM_OVERHEAD) > pool_size)), status, NU_INVALID_SIZE);

    /* Veriyf valid suspension type */
    NU_ERROR_CHECK(((suspend_type != NU_FIFO) && (suspend_type != NU_PRIORITY)), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(pool, PM_PCB);

        /* Fill in the partition pool name. */
        strncpy(pool -> pm_name, name, (NU_MAX_NAME - 1));

        /* Save the starting address and size parameters in the partition control
           block.  */
        pool -> pm_start_address =   start_address;
        pool -> pm_pool_size =       pool_size;
        pool -> pm_partition_size =  partition_size;

        /* Setup the partition pool suspension type.  */
        if (suspend_type == NU_FIFO)
        {
            /* FIFO suspension is selected, setup the flag accordingly.  */
            pool -> pm_fifo_suspend =  NU_TRUE;
        }

        /* Loop to build and link as many partitions as possible from within the
           specified memory area.  */
        pointer =  (BYTE_PTR)  start_address;
        while (pool_size >= (PM_OVERHEAD + partition_size))
        {

            /* There is room for another partition.  */

            /* Cast the current pointer variable to a header pointer.  */
            header_ptr =  (PM_HEADER *) pointer;

            /* Now, build a header and link it into the partition pool
               available list- at the front.  */
            header_ptr -> pm_partition_pool =  pool;
            header_ptr -> pm_next_available =  pool -> pm_available_list;
            pool -> pm_available_list =        header_ptr;

            /* Increment the number of partitions available in the pool.  */
            pool -> pm_available++;

            /* Decrement the number of bytes remaining in the pool.  */
            pool_size =  pool_size - (PM_OVERHEAD + partition_size);

            /* Increment the working pointer to the next partition position.  */
            pointer =  pointer + (PM_OVERHEAD + partition_size);
        }

        /* Protect against access to the list of created partition pools.  */
        TCCT_Schedule_Lock();

        /* At this point the partition pool is completely built.  The ID can
           now be set and it can be linked into the created partition pool list. */
        pool -> pm_id =  PM_PARTITION_ID;

        /* Link the partition pool into the list of created partition pools and
           increment the total number of pools in the system.  */
        NU_Place_On_List(&PMD_Created_Pools_List, &(pool -> pm_created));
        PMD_Total_Pools++;

        /* Trace log */
        T_PMEM_CREATE((VOID*)pool, start_address, name,  (pool -> pm_pool_size),
                      (pool->pm_partition_size), suspend_type, OBJ_ACTION_SUCCESS);

        /* Release protection against access to the list of created partition
           pools.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_PMEM_CREATE((VOID*)pool, start_address, name, 0, partition_size,
                      suspend_type, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Allocate_Partition
*
*   DESCRIPTION
*
*       This function allocates a memory partition from the specified
*       memory partition pool.  If a memory partition is currently
*       available, this function is completed immediately.  Otherwise,
*       if there are no partitions currently available, suspension is
*       possible.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       NU_Priority_Place_On_List           Place on priority list
*       TCC_Suspend_Task                    Suspend calling task
*       TCC_Task_Priority                   Pickup task's priority
*       [NU_Check_Stack]                    Stack checking function
*       TCCT_Current_Thread                 Pickup current thread
*                                           pointer
*       TCCT_Schedule_Lock                  Protect partition pool
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pool_ptr                            Memory partition pool
*                                           pointer
*       return_pointer                      Pointer to the destination
*                                           memory pointer
*       suspend                             Suspension option if full
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_NO_PARTITION                 No partitions are available
*           NU_TIMEOUT                      If timeout on service
*           NU_POOL_DELETED                 If partition pool deleted
*                                           during suspension
*           NU_INVALID_POOL                 Indicates the pool pointer
*                                           is invalid
*           NU_INVALID_POINTER              Indicates the return pointer
*                                           is NULL
*           NU_INVALID_SUSPEND              Indicates the suspension is
*                                           invalid
*
***********************************************************************/
STATUS NU_Allocate_Partition(NU_PARTITION_POOL *pool_ptr,
                             VOID **return_pointer, UNSIGNED suspend)
{
    R1 PM_PCB       *pool;                  /* Pool control block ptr    */
    R2 PM_SUSPEND   *suspend_ptr;           /* Suspend block pointer     */
    PM_SUSPEND      suspend_block;          /* Allocate suspension block */
    R3 PM_HEADER    *partition_ptr;         /* Pointer to partition      */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pool pointer into internal pointer.  */
    pool =  (PM_PCB *) pool_ptr;

    /* Determine if partition pool pointer is valid */
    NU_ERROR_CHECK((pool == NU_NULL), status, NU_INVALID_POOL);

    /* Determine if partition pool pointer is valid */
    NU_ERROR_CHECK((pool -> pm_id != PM_PARTITION_ID), status, NU_INVALID_POOL);

    /* Determine if the return pointer is valid */
    NU_ERROR_CHECK((return_pointer == NU_NULL), status, NU_INVALID_POINTER);

    /* Verify suspension from a task thread */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the partition pool.  */
        TCCT_Schedule_Lock();

        /* Determine if there is an available memory partition.  */
        if (pool -> pm_available)
        {
            /* Partition available.  */

            /* Decrement the available count.  */
            pool -> pm_available--;

            /* Increment the allocated count.  */
            pool -> pm_allocated++;

            /* Unlink the first memory partition and return the pointer to the
               caller.  */
            partition_ptr =              pool -> pm_available_list;
            pool -> pm_available_list =  partition_ptr -> pm_next_available;

            /* Set next available to show partition is allocated */
            partition_ptr -> pm_next_available = (PM_HEADER *)PM_PARTITION_ALLOCATED;

            /* Return a memory address to the caller.  */
            *return_pointer =  (VOID *) (((BYTE_PTR) partition_ptr) + PM_OVERHEAD);

            /* Trace log */
            T_PMEM_ALLOCATE((VOID*)pool, (VOID*)(*return_pointer), ESAL_GET_RETURN_ADDRESS(0),
                            (pool_ptr->pm_pool_size), (pool_ptr->pm_partition_size),
                            (pool_ptr->pm_available), (pool_ptr->pm_allocated),
                            suspend, OBJ_ACTION_SUCCESS);

        }
        else
        {

            /* A partition is not available.  Determine if suspension is
               required. */
            if (suspend)
            {
                /* Suspension is selected.  */

                /* Increment the number of tasks waiting.  */
                pool -> pm_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
                suspend_ptr -> pm_partition_pool =           pool;
                suspend_ptr -> pm_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> pm_suspend_link.cs_previous = NU_NULL;
                task =                            (TC_TCB *) TCCT_Current_Thread();
                suspend_ptr -> pm_suspended_task =           task;

                /* Determine if priority or FIFO suspension is associated with the
                   partition pool.  */
                if (pool -> pm_fifo_suspend)
                {

                    /* FIFO suspension is required.  Link the suspend block into
                       the list of suspended tasks on this partition pool.  */
                    NU_Place_On_List((CS_NODE **)
                                     &(pool -> pm_suspension_list),
                                     &(suspend_ptr -> pm_suspend_link));
                }
                else
                {

                    /* Get the priority of the current thread so the suspend block
                       can be placed in the appropriate place.  */
                    suspend_ptr -> pm_suspend_link.cs_priority =
                                                         TCC_Task_Priority(task);

                    NU_Priority_Place_On_List((CS_NODE **)
                                              &(pool -> pm_suspension_list),
                                              &(suspend_ptr -> pm_suspend_link));
                }

                /* Trace log */
                T_PMEM_ALLOCATE((VOID*)pool, (VOID*)(*return_pointer), ESAL_GET_RETURN_ADDRESS(0),
                                (pool_ptr->pm_pool_size), (pool_ptr->pm_partition_size),
                                (pool_ptr->pm_available), (pool_ptr->pm_allocated),
                                suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the protection on the partition pool.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_PARTITION_SUSPEND,
                                            PMC_Cleanup, suspend_ptr, suspend);

                /* Pickup the return status.  */
                status =            suspend_ptr -> pm_return_status;
                *return_pointer =   suspend_ptr -> pm_return_pointer;

            }
            else
            {
                /* No suspension requested.  Simply return an error status.  */
                status =  NU_NO_PARTITION;

                /* Trace log */
                T_PMEM_ALLOCATE((VOID*)pool, (VOID*)(*return_pointer), ESAL_GET_RETURN_ADDRESS(0),
                                (pool_ptr->pm_pool_size), (pool_ptr->pm_partition_size),
                                (pool_ptr->pm_available), (pool_ptr->pm_allocated), suspend, status);
            }
        }

        /* Release protection of the partition pool.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log
         * NOTE: Return 0 for total size and 0 for available memory in case pool_ptr is null */
        T_PMEM_ALLOCATE((VOID*)pool, (VOID*)(*return_pointer), ESAL_GET_RETURN_ADDRESS(0),
                        0, 0, 0, 0, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       PMC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a partition pool.  It is not called unless a timeout or
*       a task terminate is in progress.  Note that protection is
*       already in effect - the same protection at suspension time.
*
*   CALLED BY
*
*       TCC_Task_Timeout                    Task timeout
*       NU_Terminate_Task                   Task terminate
*
*   CALLS
*
*       NU_Remove_From_List                 Remove suspend block from
*                                           the suspension list
*
*   INPUTS
*
*       information                         Pointer to suspend block
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  PMC_Cleanup(VOID *information)
{
    PM_SUSPEND      *suspend_ptr;          /* Suspension block pointer  */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (PM_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> pm_return_status =   NU_TIMEOUT;
    suspend_ptr -> pm_return_pointer =  NU_NULL;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> pm_partition_pool) -> pm_tasks_waiting--;

    /* Unlink the suspend block from the suspension list.  */
    NU_Remove_From_List((CS_NODE **)
                        &((suspend_ptr -> pm_partition_pool) -> pm_suspension_list),
                        &(suspend_ptr -> pm_suspend_link));

    /* Return to user mode */
    NU_USER_MODE();
}
