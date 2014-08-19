/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
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
*       dmc_add_memory.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains the core add memory routines for the Dynamic Memory
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Add_Memory                       Add memory to a memory pool
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

/***********************************************************************
*
*   FUNCTION
*
*       NU_Add_Memory
*
*   DESCRIPTION
*
*       This function adds a new section of memory to an existing memory
*       pool.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Data structure protect
*       TCCT_Schedule_Unlock                Un-protect data structure
*
*   INPUTS
*
*       pool_ptr                            Memory pool control block
*                                           pointer
*       memory_start_address                Starting address of the memory
*       memory_pool_size                    Number of bytes of memory
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_POOL                     Indicates the pool control
*                                           block pointer is invalid
*       NU_INVALID_MEMORY                   Indicates the starting
*                                           memory address is NULL
*       NU_INVALID_SIZE                     Indicates that either the
*                                           memory size is invalid
*       NU_NOT_ALIGNED                      Start address is not aligned
*
***********************************************************************/
STATUS NU_Add_Memory(NU_MEMORY_POOL *pool_ptr, VOID *memory_start_address, UNSIGNED memory_size)
{
    STATUS           status = NU_SUCCESS;
    DM_HEADER       *old_header_ptr;        /* Dynamic mem block header ptr */
    DM_HEADER       *new_header_ptr;        /* Dynamic mem block header ptr */
    NU_SUPERV_USER_VARIABLES

    /* Convert the new memory size into something that is evenly divisible by
       the sizeof an UNSIGNED data element.  */
    memory_size = DM_ADJUSTED_SIZE(memory_size);

    /* Check for a NULL dynamic memory pool control block pointer or a control
       block that is not valid.  */
    NU_ERROR_CHECK(((pool_ptr == NU_NULL) || (pool_ptr -> dm_id != DM_DYNAMIC_ID)), status, NU_INVALID_POOL);

    /* Invalid memory pointer.  */
    NU_ERROR_CHECK((memory_start_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Start address is not aligned */
    NU_ERROR_CHECK((ESAL_GE_MEM_ALIGNED_CHECK(memory_start_address, sizeof(UNSIGNED)) == NU_FALSE), status, NU_NOT_ALIGNED);

    /* Pool could not even accommodate one allocation.  */
    NU_ERROR_CHECK((((pool_ptr -> dm_min_allocation) + (2 * DM_OVERHEAD)) > memory_size), status, NU_INVALID_SIZE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Build a single block that has all of the memory.  */
        new_header_ptr = (DM_HEADER *) memory_start_address;

        /* Protect against simultaneous access to the memory pool.  */
        TCCT_Schedule_Lock();

        /* Save the starting address and size parameters in the dynamic memory
           control block.  */
        pool_ptr -> dm_pool_size += memory_size;

        /* Initialize the memory parameters.  */
        pool_ptr -> dm_available += (memory_size - (2 * DM_OVERHEAD));

        /* Build the block header.  */
        new_header_ptr -> dm_memory_pool = pool_ptr;
        new_header_ptr -> dm_next_memory = (DM_HEADER *)
               (((BYTE_PTR) new_header_ptr) + memory_size - DM_OVERHEAD);
        new_header_ptr -> dm_previous_memory = new_header_ptr -> dm_next_memory;
        new_header_ptr -> dm_memory_free = NU_TRUE;

        /* Link this free block into the original free block list */
        if (pool_ptr -> dm_memory_list == NU_NULL)
        {
            new_header_ptr -> dm_next_free = new_header_ptr;
            new_header_ptr -> dm_previous_free = new_header_ptr;
            pool_ptr -> dm_memory_list = new_header_ptr;
        }
        else
        {
            old_header_ptr = (DM_HEADER *) pool_ptr -> dm_memory_list;
            new_header_ptr -> dm_next_free = old_header_ptr;
            new_header_ptr -> dm_previous_free = old_header_ptr -> dm_previous_free;
            old_header_ptr -> dm_previous_free -> dm_next_free = new_header_ptr;
            old_header_ptr -> dm_previous_free = new_header_ptr;
        }

        /* Build the small trailer block that prevents block merging when the
           pool wraps around.  Note that the list is circular so searching can
           wrap across the physical end of the memory pool.  */
        new_header_ptr =  new_header_ptr -> dm_next_memory;
        new_header_ptr -> dm_next_memory = (DM_HEADER *) memory_start_address;
        new_header_ptr -> dm_previous_memory = (DM_HEADER *) memory_start_address;
        new_header_ptr -> dm_memory_free = NU_FALSE;
        new_header_ptr -> dm_memory_pool = pool_ptr;

        /* Trace log */
        T_MEM_ADD((VOID*)pool_ptr, memory_start_address, ESAL_GET_RETURN_ADDRESS(0),
                  (pool_ptr->dm_pool_size), (pool_ptr->dm_available), memory_size, OBJ_ACTION_SUCCESS);

        /* Release protection of the memory pool.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log
         * NOTE: Return 0 for total size and 0 for available memory in case pool_ptr is null */
    	T_MEM_ADD((VOID*)pool_ptr, memory_start_address, ESAL_GET_RETURN_ADDRESS(0), 0, 0, memory_size, status);
    }

    /* Return successful completion.  */
    return(status);
}

