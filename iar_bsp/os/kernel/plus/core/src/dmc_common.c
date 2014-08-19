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
*       dmc_common.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains the core common routines for the Dynamic Memory
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Memory_Pool               Create a dynamic memory pool
*       NU_Allocate_Memory                  Allocate a memory block from
*                                           a dynamic memory pool
*       DMC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       common_services.h                   Common service constants
*       thread_control.h                    Thread Control functions
*       dynamic_memory.h                    Dynamic memory functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#include        <string.h>
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *DMD_Created_Pools_List;
extern UNSIGNED         DMD_Total_Pools;

/* Define internal component function prototypes.  */
VOID    DMC_Cleanup(VOID *information);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Memory_Pool
*
*   DESCRIPTION
*
*       This function creates a dynamic memory pool and then places it
*       on the list of created dynamic memory pools.
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
*       pool_ptr                            Memory pool control block
*                                           pointer
*       name                                Memory pool name
*       start_address                       Starting address of the pool
*       pool_size                           Number of bytes in the pool
*       min_allocation                      Minimum allocation size
*       suspend_type                        Suspension type
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_POOL                     Indicates the pool control
*                                           block pointer is invalid
*       NU_INVALID_MEMORY                   Indicates the starting
*                                           memory address is NULL
*       NU_INVALID_SIZE                     Indicates that either the
*                                           pool size and/or the
*                                           minimum allocation size is
*                                           invalid
*       NU_INVALID_SUSPEND                  Indicate the suspension type
*                                           is invalid
*       NU_NOT_ALIGNED                      Start address is not aligned
*
***********************************************************************/
STATUS NU_Create_Memory_Pool(NU_MEMORY_POOL *pool_ptr, CHAR *name,
                             VOID *start_address, UNSIGNED pool_size,
                             UNSIGNED min_allocation, OPTION suspend_type)
{
    R1 DM_PCB       *pool;                  /* Pool control block ptr    */
    DM_HEADER       *header_ptr;            /* Dynamic mem block header ptr */
    STATUS           status = NU_SUCCESS;   /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pool pointer into internal pointer.  */
    pool =  (DM_PCB *) pool_ptr;

    /* Adjust the minimum allocation size to something that is evenly
       divisible by the number of bytes in an UNSIGNED data type.  */
    min_allocation = DM_ADJUSTED_SIZE(min_allocation);

    /* Guarantee the minimum allocation size, if a 0 was passed
       an error will be returned, do not adjust for 0 */
    if ((min_allocation < DM_MIN_ALLOCATION) && (min_allocation != 0))
    {
        /* Selected minimum allocation is too small */
        min_allocation = DM_MIN_ALLOCATION;
    }

    /* Adjust the pool size to something that is evenly divisible by the
       number of bytes in an UNSIGNED data type.  */
    pool_size = DM_ADJUSTED_SIZE(pool_size);

    /* Check for a NULL dynamic memory pool control block pointer or a control
       block that is already created.  */
    NU_ERROR_CHECK(((pool == NU_NULL) || (pool -> dm_id == DM_DYNAMIC_ID)), status, NU_INVALID_POOL);

    /* Check for invalid memory pointer. */
    NU_ERROR_CHECK((start_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Ensure the start address is aligned */
    NU_ERROR_CHECK((ESAL_GE_MEM_ALIGNED_CHECK(start_address, sizeof(UNSIGNED)) == NU_FALSE), status, NU_NOT_ALIGNED);

    /* Ensure the pool could accommodate at least one allocation. */
    NU_ERROR_CHECK(((min_allocation == 0) || ((min_allocation + (2 * DM_OVERHEAD)) > pool_size)), status, NU_INVALID_SIZE);

    /* Check for invalid suspension type.  */
    NU_ERROR_CHECK(((suspend_type != NU_FIFO) && (suspend_type != NU_PRIORITY)), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(pool, DM_PCB);

        /* Fill in the dynamic memory pool name. */
        strncpy(pool -> dm_name, name, (NU_MAX_NAME - 1));

        /* Save the starting address and size parameters in the dynamic memory
           control block.  */
        pool -> dm_start_address =   start_address;
        pool -> dm_pool_size =       pool_size;
        pool -> dm_min_allocation =  min_allocation;

        /* Setup the dynamic memory pool suspension type.  */
        if (suspend_type == NU_FIFO)
        {
            /* FIFO suspension is selected, setup the flag accordingly.  */
            pool -> dm_fifo_suspend =  NU_TRUE;
        }

        /* Build a single block that has all of the memory.  */
        header_ptr =  (DM_HEADER *) start_address;

        /* Initialize the memory parameters.  */
        pool -> dm_available =       pool_size - (2 * DM_OVERHEAD);
        pool -> dm_memory_list =     header_ptr;

        /* Build the block header.  */
        header_ptr -> dm_memory_pool =  pool;
        header_ptr -> dm_next_memory =  (DM_HEADER *)
               (((BYTE_PTR) header_ptr) + pool -> dm_available + DM_OVERHEAD);
        header_ptr -> dm_previous_memory =  header_ptr -> dm_next_memory;
        header_ptr -> dm_memory_free =  NU_TRUE;
        header_ptr -> dm_next_free = header_ptr;
        header_ptr -> dm_previous_free = header_ptr;

        /* Build the small trailer block that prevents block merging when the
           pool wraps around.  Note that the list is circular so searching can
           wrap across the physical end of the memory pool.  */
        header_ptr =  header_ptr -> dm_next_memory;
        header_ptr -> dm_next_memory =  (DM_HEADER *) start_address;
        header_ptr -> dm_previous_memory =  (DM_HEADER *) start_address;
        header_ptr -> dm_memory_free =  NU_FALSE;
        header_ptr -> dm_memory_pool =  pool;

        /* Protect against access to the list of created memory pools.  */
        TCCT_Schedule_Lock();

        /* At this point the dynamic memory pool is completely built.  The ID can
           now be set and it can be linked into the created dynamic memory
           pool list. */
        pool -> dm_id =  DM_DYNAMIC_ID;

        /* Link the memory pool into the list of created memory pools and
           increment the total number of pools in the system.  */
        NU_Place_On_List(&DMD_Created_Pools_List, &(pool -> dm_created));
        DMD_Total_Pools++;

        /* Trace log */
        T_MEM_POOL_CREATE((VOID*)pool_ptr, (VOID*)start_address, name,
                          pool_size, (pool -> dm_available), min_allocation, suspend_type, NU_SUCCESS);

        /* Release protection against access to the list of created memory
           pools.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_MEM_POOL_CREATE((VOID*)pool_ptr, (VOID*)start_address, name,
                          pool_size, 0, min_allocation, suspend_type, status);
    }

    /* Return successful completion.  */
    return(status);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Allocate_Aligned_Memory
*
*   DESCRIPTION
*
*       This function allocates memory from the specified dynamic memory
*       pool.  If dynamic memory is currently available, this function
*       is completed immediately.  Otherwise, if there is not enough
*       memory currently available, task suspension is possible.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       NU_Priority_Place_On_List           Place node on the list
*                                           of equal or greater priority
*       TCC_Suspend_Task                    Suspend calling task
*       TCC_Task_Priority                   Pickup task priority
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Current_Thread                 Pickup current thread pointer
*       TCCT_Schedule_Lock                  Protect memory pool
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pool_ptr                            Memory pool pointer
*       return_pointer                      Pointer to the destination
*                                           memory pointer
*       size                                Number of bytes requested
*       alignment                           Required alignment of
*                                           destination memory pointer
*       suspend                             Suspension option if full
*
*  OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_NO_MEMORY                    Memory not available
*           NU_TIMEOUT                      If timeout on service
*           NU_POOL_DELETED                 If memory pool deleted
*                                           during suspension
*           NU_INVALID_POOL                 Indicates the supplied pool
*                                           pointer is invalid
*           NU_INVALID_POINTER              Indicates the return pointer
*                                           is NULL
*           NU_INVALID_SIZE                 Indicates the size is 0 or
*                                           larger than the pool
*           NU_INVALID_SUSPEND              Invalid suspension requested
*
***********************************************************************/
STATUS NU_Allocate_Aligned_Memory(NU_MEMORY_POOL *pool_ptr,
                                  VOID **return_pointer,
                                  UNSIGNED size, UNSIGNED alignment,
                                  UNSIGNED suspend)
{
    R1 DM_PCB       *pool;                  /* Pool control block ptr    */
    R2 DM_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    R3 DM_HEADER    *memory_ptr;            /* Pointer to memory         */
    DM_SUSPEND      suspend_block;          /* Allocate suspension block */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pool pointer into internal pointer.  */
    pool = (DM_PCB *) pool_ptr;

    /* Determine if dynamic memory pool pointer is invalid.  */
    NU_ERROR_CHECK((pool == NU_NULL), status, NU_INVALID_POOL);

    /* Ensure dynamic memory pool pointer is valid */
    NU_ERROR_CHECK((pool -> dm_id != DM_DYNAMIC_ID), status, NU_INVALID_POOL);

    /* Ensure return pointer is valid. */
    NU_ERROR_CHECK((return_pointer == NU_NULL), status, NU_INVALID_POINTER);

    /* Check for valid allocation size */
    NU_ERROR_CHECK(((size == 0) || (size > (pool -> dm_pool_size - (2 * DM_OVERHEAD)))), status, NU_INVALID_SIZE);

    /* Check for suspension from an non-task thread.  */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Adjust the request to a size evenly divisible by the number of bytes
           in an UNSIGNED data element.  Also, check to make sure it is of the
           minimum size.  */
        if (size < pool -> dm_min_allocation)
        {
            /* Change size to the minimum allocation.  */
            size =  pool -> dm_min_allocation;
        }
        else
        {
            /* Insure that size is a multiple of the UNSIGNED size.  */
            size = DM_ADJUSTED_SIZE(size);
        }

        /* Adjust the requested alignment to one evenly divisible by the number of
           bytes in an UNSIGNED data element. */
        if (alignment != 0)
        {
            alignment = DM_ADJUSTED_ALIGNMENT(alignment);
        }

        /* Protect against simultaneous access to the memory pool.  */
        TCCT_Schedule_Lock();

        /* Allocate memory if available */
        memory_ptr = DMC_Allocate(pool_ptr, size, alignment);

        if (memory_ptr != NU_NULL)
        {
            /* Return a memory address to the caller.  */
            *return_pointer =  (VOID *) (((BYTE_PTR) memory_ptr) + DM_OVERHEAD);

            /* Trace log */
            T_DMEM_ALLOCATE((VOID*)pool_ptr, (VOID*)(*return_pointer),
                            ESAL_GET_RETURN_ADDRESS(0), (pool_ptr->dm_pool_size),
                            (pool_ptr->dm_available), size, suspend, OBJ_ACTION_SUCCESS);
        }
        else
        {
            /* Enough dynamic memory is not available.  Determine if suspension is
               required. */
            if (suspend)
            {
                /* Suspension is selected.  */

                /* Increment the number of tasks waiting.  */
                pool -> dm_tasks_waiting++;

                /* Setup the suspend block and suspend the calling task.  */
                suspend_ptr =  &suspend_block;
                suspend_ptr -> dm_memory_pool =              pool;
                suspend_ptr -> dm_request_size =             size;
                suspend_ptr -> dm_suspend_link.cs_next =     NU_NULL;
                suspend_ptr -> dm_suspend_link.cs_previous = NU_NULL;
                task =                            (TC_TCB *) TCCT_Current_Thread();
                suspend_ptr -> dm_suspended_task =           task;
                suspend_ptr -> dm_alignment = alignment;

                /* Determine if priority or FIFO suspension is associated with the
                   memory pool.  */
                if (pool -> dm_fifo_suspend)
                {
                    /* FIFO suspension is required.  Link the suspend block into
                       the list of suspended tasks on this memory pool.  */
                    NU_Place_On_List((CS_NODE **)
                                     &(pool -> dm_suspension_list),
                                     &(suspend_ptr -> dm_suspend_link));
                }
                else
                {
                    /* Get the priority of the current thread so the suspend block
                       can be placed in the appropriate place.  */
                    suspend_ptr -> dm_suspend_link.cs_priority =
                                                         TCC_Task_Priority(task);

                    NU_Priority_Place_On_List((CS_NODE **)
                                              &(pool -> dm_suspension_list),
                                              &(suspend_ptr -> dm_suspend_link));
                }

                /* Trace log */
                T_DMEM_ALLOCATE((VOID*)pool_ptr, (VOID*)(*return_pointer),
                                ESAL_GET_RETURN_ADDRESS(0), (pool_ptr->dm_pool_size),
                                (pool_ptr->dm_available), size, suspend, OBJ_BLKD_CTXT);

                /* Finally, suspend the calling task. Note that the suspension call
                   automatically clears the system protection.  */
                TCC_Suspend_Task((NU_TASK *) task, NU_MEMORY_SUSPEND,
                                            DMC_Cleanup, suspend_ptr, suspend);

                /* Pickup the return status.  */
                status =            suspend_ptr -> dm_return_status;
                *return_pointer =   suspend_ptr -> dm_return_pointer;

            }
            else
            {
                /* No suspension requested.  Simply return an error status.  */
                status =            NU_NO_MEMORY;
                *return_pointer =   NU_NULL;

                /* Trace log */
                T_DMEM_ALLOCATE((VOID*)pool_ptr, (VOID*)(*return_pointer),
                                ESAL_GET_RETURN_ADDRESS(0), (pool_ptr->dm_pool_size),
                                (pool_ptr->dm_available), size, suspend, status);
            }
        }

        /* Release protection of the memory pool.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log
         * NOTE: Return 0 for total size and 0 for available memory in case pool_ptr is null */
        T_DMEM_ALLOCATE((VOID*)pool_ptr, (VOID*)(*return_pointer),
                        ESAL_GET_RETURN_ADDRESS(0), 0, 0, size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}

/***********************************************************************
*
*   FUNCTION
*
*       DMC_Allocate
*
*   DESCRIPTION
*
*       This function allocates memory by searching a list of free
*       memory blocks.  Blocks will be front split to gain proper
*       alignment if that memory is big enough to be used by another
*       allocation.  Blocks will be rear split if the block size
*       remaining is large enough for future allocations.  If memory
*       can't be saved through splits it will be marked with the
*       allocated block.
*
*   CALLED BY
*
*       NU_Allocate_Memory
*       NU_Deallocate_Memory
*       NU_Reallocate_Memory
*
*   CALLS
*
*       DMC_Split_Block
*
*   INPUTS
*
*       pool_ptr                            Memory pool pointer
*       size                                Number of bytes requested
*       alignment                           The alignment the start of
*                                           allocated memory should use
*
*  OUTPUTS
*
*       memory_ptr                          Pointer to free block or
*                                           NU_NULL if no memory found
*
***********************************************************************/
DM_HEADER *DMC_Allocate(NU_MEMORY_POOL *pool_ptr, UNSIGNED size, UNSIGNED alignment)
{
    DM_HEADER  *memory_ptr;                 /* Pointer to memory */
    UNSIGNED    address = 0;                /* Address of start of block */
    UNSIGNED    split_size = 0;             /* Bytes for front split     */
    UNSIGNED    next_aligned;               /* Next aligned block addr   */
    UNSIGNED    block_size = 0;

    /* Search the memory list for the first available block of memory that
       satisfies the request.  Note that blocks are merged and sorted during the
       deallocation function. */
    memory_ptr = pool_ptr -> dm_memory_list;

    if (memory_ptr != NU_NULL)
    {
        do
        {
            /* Calculate the free block size.  */
            block_size = (((BYTE_PTR) (memory_ptr -> dm_next_memory)) -
                          ((BYTE_PTR) memory_ptr)) - DM_OVERHEAD;

            /* Test the free block to see if the request can be satisfied */
            if (block_size < size)
            {
                /* Large enough block has not been found.  Move the search
                   pointer to the next block. */
                memory_ptr = memory_ptr -> dm_next_free;
            }
            else
            {
                /* Check to see if allocated memory has a specific alignment requirement */
                if (alignment)
                {
                    /* Get address of memory block */
                    address = ((UNSIGNED)(memory_ptr)) + DM_OVERHEAD;

                    /* Is this free block, minus the overhead, already aligned? */
                    if (address % alignment != 0)
                    {
                        /* Not aligned, can the free block be split in front? */
                        next_aligned = address + (alignment - 1);
                        next_aligned /= alignment;
                        next_aligned *= alignment;
                        split_size = next_aligned - address;

                        /* Is space from start of block to aligned location large enough
                           to contain 2 DM_OVERHEAD plus pool -> dm_min_allocation? */
                        if (split_size < ((2 * DM_OVERHEAD) + pool_ptr -> dm_min_allocation))
                        {
                            /* No, so try to make space for overhead and dm_min_allocation */
                            next_aligned = address + (2 * DM_OVERHEAD) +
                                          (pool_ptr -> dm_min_allocation) + (alignment - 1);
                            next_aligned /= alignment;
                            next_aligned *= alignment;
                            split_size = next_aligned - address;

                            /* If the new split size is too big search for a
                               new free block */
                            if ((split_size + size) > block_size)
                            {
                                /* Reset block size to force move */
                                block_size = 0;
                            }
                        }

                        /* Determine if the next aligned address is within
                           the current memory block */
                        if ((block_size > 0) && (((UNSIGNED)next_aligned) > ((UNSIGNED)address + block_size)))
                        {
                            /* Reset block size to force move */
                            block_size = 0;
                        }

                        /* Adjust free_size for result of front split */
                        if (block_size > split_size)
                        {
                            block_size -= split_size;
                        }

                        /* Check block size again after adjustments to see if
                           the pointer should be advanced in the list */
                        if (block_size < size)
                        {
                            /* Can't adjust block beginning, so keep searching */
                            memory_ptr = memory_ptr -> dm_next_free;
                        }
                    }
                }
            }

        /* Determine if the block can satisfy the request and
           if the search should continue.  */
        } while ((memory_ptr != pool_ptr -> dm_memory_list) && (block_size < size));

        /* Determine if the memory is available.  */
        if (block_size >= size)
        {
            /* A block that satisfies the request has been found.  */

            /* Check to see if allocated memory has a specific alignment requirement */
            if (alignment)
            {
                /* Is a front split required? The front split will represent the chunk
                   of memory that goes from the last pointer to the aligned address. */
                if(address % alignment != 0)
                {
                    /* Not aligned, front split the block, leaving an allocated block.
                       Update the memory pointer to the new aligned block */
                    memory_ptr = DMC_Split_Block(pool_ptr, memory_ptr, split_size, NU_TRUE);
                }
            }

            /* Determine if the block needs to be split.  This rear split will be
               the memory following the size needed from the location of the memory
               pointer.  */
            if (block_size >= (size + DM_OVERHEAD + pool_ptr -> dm_min_allocation))
            {
                /* Yes, split the block.  */
                (VOID)DMC_Split_Block(pool_ptr, memory_ptr, size + DM_OVERHEAD, NU_FALSE);
            }
            else
            {
                /* Decrement the entire free size from the available bytes
                   count.  */
                pool_ptr -> dm_available =  pool_ptr -> dm_available - block_size;
            }

            /* Mark this memory as now allocated */
            memory_ptr -> dm_memory_free = NU_FALSE;

            /* If the allocation list is now empty clear the free list pointer */
            if (pool_ptr -> dm_available == 0)
            {
                pool_ptr -> dm_memory_list = NU_NULL;
            }
            else
            {
                /* update the list to remove the allocated block */
                (memory_ptr -> dm_previous_free) -> dm_next_free = memory_ptr -> dm_next_free;
                (memory_ptr -> dm_next_free) -> dm_previous_free = memory_ptr -> dm_previous_free;

                /* If the first element is now being used update the pool control block
                   search list */
                if (pool_ptr -> dm_memory_list == memory_ptr)
                {
                    pool_ptr -> dm_memory_list = memory_ptr -> dm_next_free;
                }
            }
        }
        else
        {
            /* No memory available clear the memory pointer */
            memory_ptr = NU_NULL;
        }
    }

    return (memory_ptr);
}

/***********************************************************************
*
*   FUNCTION
*
*       DMC_Split_Block
*
*   DESCRIPTION
*
*       This function sets up a new header and splits the block on the
*       memory lists.  Protection must be in place before using this
*       internal function.
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       pool_ptr                            Memory pool pointer
*       memory_ptr                          Pointer to free block
*       size                                Number of bytes requested
*       front_split                         Front and rear splits
*                                           calculate remaining memory
*                                           differently.
*
*  OUTPUTS
*
*       new_ptr                             Pointer to split block header
*
***********************************************************************/
DM_HEADER *DMC_Split_Block(NU_MEMORY_POOL *pool_ptr, DM_HEADER *memory_ptr,
                           UNSIGNED size, BOOLEAN front_split)
{
    DM_HEADER *new_ptr;

     /* Yes, split the block.  */
    new_ptr =  (DM_HEADER *) (((BYTE_PTR) memory_ptr) + size);

    /* Mark the new block as free.  */
    new_ptr -> dm_memory_free = NU_TRUE;

    /* Put the pool pointer into the new block.  */
    new_ptr -> dm_memory_pool = pool_ptr;

    /* Build the necessary pointers.  */
    new_ptr -> dm_previous_memory = memory_ptr;
    new_ptr -> dm_next_memory = memory_ptr -> dm_next_memory;
    (new_ptr -> dm_next_memory) -> dm_previous_memory = new_ptr;
    memory_ptr -> dm_next_memory = new_ptr;

    /* Add to the free list */
    new_ptr -> dm_previous_free = memory_ptr;
    new_ptr -> dm_next_free = memory_ptr -> dm_next_free;
    (new_ptr -> dm_next_free) -> dm_previous_free = new_ptr;
    memory_ptr -> dm_next_free = new_ptr;

    /* If the size being passed in is for a rear split the overhead will be
       the only size lost. */
    if (front_split == NU_TRUE)
    {
        /* Decrement the available byte count.  */
        pool_ptr -> dm_available = pool_ptr -> dm_available - DM_OVERHEAD;
    }
    else
    {
        /* This is a rear split update the byte count for the full size lost */
        pool_ptr -> dm_available = pool_ptr -> dm_available - size;
    }

    return (new_ptr);
}

/***********************************************************************
*
*   FUNCTION
*
*       DMC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a memory pool.  It is not called unless a timeout or
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
VOID  DMC_Cleanup(VOID *information)
{
    DM_SUSPEND      *suspend_ptr;           /* Suspension block pointer  */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (DM_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> dm_return_status =   NU_TIMEOUT;
    suspend_ptr -> dm_return_pointer =  NU_NULL;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> dm_memory_pool) -> dm_tasks_waiting--;

    /* Unlink the suspend block from the suspension list.  */
    NU_Remove_From_List((CS_NODE **)
                        &((suspend_ptr -> dm_memory_pool) -> dm_suspension_list),
                        &(suspend_ptr -> dm_suspend_link));

    /* Return to user mode */
    NU_USER_MODE();
}
