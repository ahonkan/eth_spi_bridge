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
*       dmc_deallocate.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains the core Deallocate routine for the
*       Dynamic Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Deallocate_Memory                Deallocate a memory block
*                                           from a dynamic memory pool
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       dynamic_memory.h                    Dynamic memory functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#include        "services/nu_trace_os_mark.h"

/* Define internal component function prototypes.  */
DM_HEADER *DMC_Allocate(NU_MEMORY_POOL *pool_ptr, UNSIGNED size, UNSIGNED alignment);
static VOID DMC_Deallocate(DM_PCB *pool, DM_HEADER *header_ptr);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Deallocate_Memory
*
*   DESCRIPTION
*
*       This function deallocates a previously allocated dynamic memory
*       block.  The deallocated dynamic memory block is merged with any
*       adjacent neighbors.  This insures that there are no consecutive
*       blocks of free memory in the pool (makes the search easier!).
*       If there is a task waiting for dynamic memory, a determination
*       of whether or not the request can now be satisfied is made after
*       the deallocation is complete.
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
*       TCCT_Schedule_Lock                  Protect dynamic memory pool
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       memory                              Pointer to dynamic memory
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_INVALID_POINTER              Indicates the supplied
*                                           pointer is invalid
*
***********************************************************************/
STATUS NU_Deallocate_Memory(VOID *memory)
{
    R1 DM_PCB       *pool;                  /* Pool pointer              */
    R2 DM_SUSPEND   *suspend_ptr;           /* Pointer to suspend block  */
    R3 DM_HEADER    *header_ptr;            /* Pointer to memory header  */
    UNSIGNED        size;                   /* Suspended task request    */
    UNSIGNED        align;                  /* Suspended task alignment  */
    STATUS          preempt;                /* Preemption flag           */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Determine if the memory pointer is NULL.  */
    NU_ERROR_CHECK((memory == NU_NULL), status, NU_INVALID_POINTER);

    /* Check that it is safe to dereference the memory pointer */
    if (status == NU_SUCCESS)
    {
        /* Pickup the header pointer */
        header_ptr = (DM_HEADER *) (((BYTE_PTR) memory) - DM_OVERHEAD);
    }

    /* Determine if the header pointer is invalid.  */
    NU_ERROR_CHECK((header_ptr == NU_NULL), status, NU_INVALID_POINTER);

    /* Check that it is safe to dereference the pool pointer */
    if (status == NU_SUCCESS)
    {
        /* Pickup the associated pool's pointer.  It is inside the header of
           each memory.  */
        pool = header_ptr -> dm_memory_pool;
    }

    /* Determine if dynamic memory pool pointer is invalid.  */
    NU_ERROR_CHECK((pool == NU_NULL), status, NU_INVALID_POINTER);

    /* Ensure the dynamic memory pool pointer is valid */
    NU_ERROR_CHECK((pool -> dm_id != DM_DYNAMIC_ID), status, NU_INVALID_POINTER);

    /* Ensure the dynamic memory is free - must not be allocated.  */
    NU_ERROR_CHECK((header_ptr -> dm_memory_free), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the memory pool.  */
        TCCT_Schedule_Lock();

        DMC_Deallocate(pool, header_ptr);

        /* Determine if another task is waiting for memory from the pool.  */
        suspend_ptr =  pool -> dm_suspension_list;
        preempt =      0;
        while (suspend_ptr)
        {
            /* Yes, another task is waiting for memory from the pool.  Search
               the pool in the same manner as the memory allocation function.  */
            size =        suspend_ptr -> dm_request_size;
            align =       suspend_ptr -> dm_alignment;
            header_ptr = DMC_Allocate(pool, size, align);
            if(header_ptr != NU_NULL)
            {
                /* Decrement the number of tasks waiting counter.  */
                pool -> dm_tasks_waiting--;

                /* Remove the first suspended block from the list.  */
                NU_Remove_From_List((CS_NODE **) &(pool -> dm_suspension_list),
                                    &(suspend_ptr -> dm_suspend_link));

                /* Setup the appropriate return value.  */
                suspend_ptr -> dm_return_status =   NU_SUCCESS;
                suspend_ptr -> dm_return_pointer =  (VOID *)
                                    (((BYTE_PTR) header_ptr) + DM_OVERHEAD);

                /* Trace log */
                T_DMEM_DEALLOCATE((VOID*)pool, memory, ESAL_GET_RETURN_ADDRESS(0), (pool->dm_pool_size),
                                  (pool->dm_available), OBJ_UNBLKD_CTXT);

                /* Resume the suspended task.  */
                preempt =  preempt |
                  TCC_Resume_Task((NU_TASK *) suspend_ptr -> dm_suspended_task,
                                                            NU_MEMORY_SUSPEND);

                /* Pickup the next suspension pointer.  */
                suspend_ptr =  pool -> dm_suspension_list;
            }
            else
            {
                /* Not enough memory for suspended task. */
                suspend_ptr =  NU_NULL;
            }

        }

        /* Trace log */
        T_DMEM_DEALLOCATE((VOID*)pool, memory, ESAL_GET_RETURN_ADDRESS(0), (pool->dm_pool_size),
                         (pool->dm_available), OBJ_ACTION_SUCCESS);

        /* Determine if a preempt condition is present.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to the system if the resumed task function
               detects a preemption condition.  */
            TCCT_Control_To_System();
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
        T_DMEM_DEALLOCATE((VOID*)pool, memory, ESAL_GET_RETURN_ADDRESS(0), 0, 0, status);
    }

    /* Return the completion status.  */
    return(status);
}

/***********************************************************************
*
*   FUNCTION
*
*       DMC_Deallocate
*
*   DESCRIPTION
*
*       This function frees memory and places it on the free memory list.
*
*   CALLED BY
*
*       NU_Deallocate_Memory
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       pool_ptr                            Memory pool pointer
*       header_ptr                          Header to memory being
*                                           deallocated
*
***********************************************************************/
static VOID DMC_Deallocate(DM_PCB *pool, DM_HEADER *header_ptr)
{
    DM_HEADER *new_ptr;                     /* New memory block pointer  */
    BOOLEAN    in_free_list = NU_FALSE;

    /* Mark the memory as available.  */
    header_ptr -> dm_memory_free =  NU_TRUE;

    /* Adjust the available number of bytes.  */
    pool -> dm_available =  pool -> dm_available +
                        (((BYTE_PTR) (header_ptr -> dm_next_memory)) -
                           ((BYTE_PTR) header_ptr)) - DM_OVERHEAD;

    /* Determine if the block can be merged with the previous neighbor.  */
    if ((header_ptr -> dm_previous_memory) -> dm_memory_free)
    {
        /* Adjust the available number of bytes.  */
        pool -> dm_available =  pool -> dm_available + DM_OVERHEAD;

        /* Yes, merge block with previous neighbor.  */
        (header_ptr -> dm_previous_memory) -> dm_next_memory = header_ptr -> dm_next_memory;
        (header_ptr -> dm_next_memory) -> dm_previous_memory = header_ptr -> dm_previous_memory;

        /* Move header pointer to previous.  */
        header_ptr =  header_ptr -> dm_previous_memory;

        /* Since we are joining with an existing block it is already in the free list,
           there is also no need to modify the free list pointers in this case */
        in_free_list = NU_TRUE;
    }

    /* Determine if the block can be merged with the next neighbor.  */
    if ((header_ptr -> dm_next_memory) -> dm_memory_free)
    {
        /* Adjust the available number of bytes.  */
        pool -> dm_available =  pool -> dm_available + DM_OVERHEAD;

        /* Yes, merge block with next neighbor.  */
        new_ptr =  header_ptr -> dm_next_memory;
        (new_ptr -> dm_next_memory) -> dm_previous_memory = header_ptr;
        header_ptr -> dm_next_memory = new_ptr -> dm_next_memory;

        /* Remove the old block from the free list and
           place the new one in the list */
        if (new_ptr -> dm_next_free == new_ptr)
        {
            /* If this is the only element on the list just update
               the header to point back to itself */
            header_ptr -> dm_previous_free = header_ptr;
            header_ptr -> dm_next_free = header_ptr;
        }
        else
        {
            /* At this point check to see if the the header pointer
               is on the list */
            if (in_free_list == NU_FALSE)
            {
                /* Add to the free list where the adjacent pointer
                   was located */
                header_ptr -> dm_previous_free = new_ptr -> dm_previous_free;
                header_ptr -> dm_next_free = new_ptr -> dm_next_free;
                (new_ptr -> dm_next_free) -> dm_previous_free = header_ptr;
                (new_ptr -> dm_previous_free) -> dm_next_free = header_ptr;

            }
            else
            {
                /* Update the previous and next adjacent pointers to point
                   to the header block pointer and remove the "new" pointer
                   from the list */
                (new_ptr -> dm_previous_free) -> dm_next_free = new_ptr -> dm_next_free;
                (new_ptr -> dm_next_free) -> dm_previous_free = new_ptr -> dm_previous_free;
            }
        }

        /* Since we have updated the free list pointers indicate
           it is already in the free list */
        in_free_list = NU_TRUE;
    }

    /* Determine if this memory needs to be added to the free list */
    if (in_free_list == NU_FALSE)
    {
        /* Add to the free list */
        if (pool -> dm_memory_list == NU_NULL)
        {
            /* If list is empty set to the header pointer */
            header_ptr -> dm_next_free = header_ptr;
            header_ptr -> dm_previous_free = header_ptr;
        }
        else
        {
            /* Place in the list */
            header_ptr -> dm_previous_free = (pool -> dm_memory_list) -> dm_previous_free;
            (header_ptr -> dm_previous_free) -> dm_next_free = header_ptr;
            header_ptr -> dm_next_free = (pool -> dm_memory_list);
            (header_ptr -> dm_next_free) -> dm_previous_free = header_ptr;
        }
    }

    /* Update the memory list pointer */
    pool -> dm_memory_list = header_ptr;
}


