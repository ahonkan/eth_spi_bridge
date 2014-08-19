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
*       dmc_reallocate.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains the core Reallocate routine for the
*       Dynamic Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Reallocate_Memory                Reallocate a memory block
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


/***********************************************************************
*
*   FUNCTION
*
*       NU_Reallocate_Aligned_Memory
*
*   DESCRIPTION
*
*       This function reallocates memory from the specified dynamic memory
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
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       NU_Deallocate_Memory                Deallocate memory
*       NU_Allocate_Aligned_Memory          Allocate memory
*       memcpy
*
*   INPUTS
*
*       pool_ptr                            Memory pool pointer
*       memory_ptr                          Pointer to the destination
*                                           memory pointer
*       alignment                           Desired alignment of
*                                           memory pointer
*       size                                Number of bytes requested
*       suspend                             Suspension option if full
*
*   OUTPUTS
*
*       status
*       NU_SUCCESS                          If service is successful
*       NU_NO_MEMORY                        Memory not available
*       NU_TIMEOUT                          If timeout on service
*       NU_POOL_DELETED                     If memory pool deleted
*                                           during suspension
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid
*       NU_INVALID_POINTER                  Indicates the return pointer
*                                           is NULL
*       NU_INVALID_SIZE                     Indicates the size is 0 or
*                                           larger than the pool
*       NU_INVALID_SUSPEND                  Invalid suspension requested
*
***********************************************************************/
STATUS NU_Reallocate_Aligned_Memory(NU_MEMORY_POOL *pool_ptr,
                                    VOID **memory_ptr,
                                    UNSIGNED size, UNSIGNED alignment,
                                    UNSIGNED suspend)
{
    STATUS      status = NU_SUCCESS;
    VOID *      orig_ptr;
    DM_HEADER * header_ptr;
    UNSIGNED    block_size;
    NU_SUPERV_USER_VARIABLES

    /* Determine if dynamic memory pool pointer is invalid.  */
    NU_ERROR_CHECK((pool_ptr == NU_NULL), status, NU_INVALID_POOL);

    /* Ensure the dynamic memory pool pointer is valid */
    NU_ERROR_CHECK((pool_ptr -> dm_id != DM_DYNAMIC_ID), status, NU_INVALID_POOL);

    /* Ensure the memory return pointer is valid */
    NU_ERROR_CHECK((memory_ptr == NU_NULL), status, NU_INVALID_POINTER);

    /* Ensure a valid size in the pool for allocation size */
    NU_ERROR_CHECK((size > (pool_ptr -> dm_pool_size - (2 * DM_OVERHEAD))), status, NU_INVALID_SIZE);

    /* Check for suspension from an non-task thread.  */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Determine if block just needs to be deallocated */
        if ((size == 0) && (*memory_ptr != NU_NULL))
        {
            /* deallocate the memory */
            status = NU_Deallocate_Memory(*memory_ptr);

            /* Set the return pointer to NULL */
            *memory_ptr = NU_NULL;
        }
        else
        {
            /* Get the original memory pointer passed in */
            orig_ptr = *memory_ptr;

            /* Allocate the requested memory */
            status = NU_Allocate_Aligned_Memory(pool_ptr, memory_ptr, size, alignment, suspend);

            /* Check to see if allocation was successful */
            if (status == NU_SUCCESS)
            {
                /* Check to see if a copy is required */
                if (orig_ptr != NU_NULL)
                {
                    /* Pickup the header for the original block of memory */
                    header_ptr = (DM_HEADER *) (((BYTE_PTR) orig_ptr) - DM_OVERHEAD);
                    
                    if ((header_ptr -> dm_memory_pool -> dm_id == DM_DYNAMIC_ID) && 
                        (header_ptr -> dm_memory_free == 0))
                    {
                        /* Get the size of the original memory block */
                        block_size = (((BYTE_PTR) (header_ptr -> dm_next_memory)) -
                                      ((BYTE_PTR) header_ptr)) - DM_OVERHEAD;
    
                        /* Expanding or shrinking? */
                        if (block_size > size)
                        {
                            /* Shrinking - only copy what is required */
                            block_size = size;
                        }
    
                        /* Copy the contents of the original block to the new block */
                        memcpy(*memory_ptr, orig_ptr, block_size);
    
                        /* Free the original block */
                        status = NU_Deallocate_Memory(orig_ptr);
                    }
                }
            }
            else
            {
                /* Restore the original pointer since the allocation failed */
                *memory_ptr = orig_ptr;
            }
        }

        /* Trace log */
        T_DMEM_REALLOCATE((VOID*)pool_ptr, (VOID*)(*memory_ptr), ESAL_GET_RETURN_ADDRESS(0),
                          (pool_ptr->dm_pool_size), (pool_ptr->dm_available), size, suspend, status);

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log
         * NOTE: Return 0 for total size and 0 for available memory in case pool_ptr is null */
        T_DMEM_REALLOCATE((VOID*)pool_ptr, (VOID*)(*memory_ptr),
                          ESAL_GET_RETURN_ADDRESS(0), 0, 0, size, suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}
