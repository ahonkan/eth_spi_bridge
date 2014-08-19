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
*       inct_pools.c
*
*   COMPONENT
*
*       IN - Initialization
*
*   DESCRIPTION
*
*       This file contains the memory pool initialization and setup
*       routine with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       INCT_Sys_Mem_Pools_Initialize       Initialize the system memory
*                                           pools
*       NU_System_Memory_Get                Get pointer to system memory
*                                           pools
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       dynamic_memory.h                    Dynamic Memory constants
*       esal.h                              ESAL functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/esal.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#include        "os/kernel/plus/supplement/inc/error_management.h"

/* System memory pools used by Middleware */
extern          NU_MEMORY_POOL  System_Memory;

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))

extern          NU_MEMORY_POOL  Uncached_System_Memory;
static          BOOLEAN         Uncached_Memory_Flag = NU_FALSE;

#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */


/***********************************************************************
*
*   FUNCTION
*
*       INCT_Sys_Mem_Pools_Initialize
*
*   DESCRIPTION
*
*       This function creates two system memory pools.  The first
*       memory pool is either cached or uncached memory.  The
*       second memory pool will always be uncached memory.
*
*   CALLED BY
*
*       INC_Initialize
*
*   CALLS
*
*       ESAL_GE_MEM_Remaining_Size_Get      Gets remaining size of
*                                           memory block
*       ESAL_GE_MEM_Next_Match_Find         Finds address of memory
*                                           matching criteria
*       NU_Create_Memory_Pool               Creates a memory pool
*       ERC_System_Error                    System error handler
*
*   INPUTS
*
*       avail_mem_ptr                       pointer to start of first
*                                           available memory
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    INCT_Sys_Mem_Pools_Initialize(VOID *avail_mem_ptr)
{
    R1 UINT32   sys_mem_size;
    STATUS      status = NU_NO_MEMORY;

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))

    R2 UINT32   uncached_mem_size;
    VOID        *uncached_mem_ptr;

#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

    /* Get size of remaining memory after start of available memory */
    sys_mem_size = ESAL_GE_MEM_Remaining_Size_Get(avail_mem_ptr);

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))

    /* Find address of next block of uncached RAM memory starting from the
       available memory address */
    uncached_mem_ptr = ESAL_GE_MEM_Next_Match_Find(avail_mem_ptr, ESAL_NOCACHE,
                                                   ESAL_RAM, ESAL_DATA);

    /* Check if uncached memory found */
    if ((uncached_mem_ptr != (VOID *)ESAL_GE_MEM_ERROR) &&
        (uncached_mem_ptr == avail_mem_ptr))
    {
        Uncached_Memory_Flag = NU_TRUE;
    }

#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

    /* Create first system memory pool
       NOTE:  This pool can be either cached or uncached.  This depends on the
              type of memory available on the target platform. */
    status = NU_Create_Memory_Pool(&System_Memory,
                                   "SYSMEM",
                                   avail_mem_ptr,
                                   (sys_mem_size - DM_OVERHEAD),
                                   32,
                                   NU_FIFO);

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))

    /* Ensure first memory pool creation successful */
    if ((status == NU_SUCCESS) && ((uncached_mem_ptr != avail_mem_ptr) && (uncached_mem_ptr != (VOID *)ESAL_GE_MEM_ERROR)))
    {
        /* Get size of uncached memory block */
        uncached_mem_size = ESAL_GE_MEM_Remaining_Size_Get(uncached_mem_ptr);

        /* Create second system memory pool
           NOTE:  This pool will always be uncached */
        status = NU_Create_Memory_Pool(&Uncached_System_Memory,
                                       "USYSMEM",
                                       uncached_mem_ptr,
                                       (uncached_mem_size - DM_OVERHEAD),
                                       32,
                                       NU_FIFO);
    }

#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

    /* Ensure all initialization completed successfully */
    if (status != NU_SUCCESS)
    {
        /* Call system error handler with status */
        ERC_System_Error(status);
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_System_Memory_Get
*
*   DESCRIPTION
*
*       This function returns the pointers to the system memory pools.
*
*   CALLED BY
*
*       Application / Middleware
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       sys_pool_ptr                        Pointer to memory pool pointer
*       usys_pool_ptr                       Pointer to uncached memory pool pointer
*
*   OUTPUTS
*
*       NU_SUCCESS                          Memory pool pointer(s) returned
*       NU_INVALID_POINTER                  Both pointers passed are invalid
*
***********************************************************************/
STATUS NU_System_Memory_Get(NU_MEMORY_POOL ** sys_pool_ptr,
                            NU_MEMORY_POOL ** usys_pool_ptr)
{
    STATUS  status = NU_INVALID_POINTER;


    /* Check if system memory pointer is valid */
    if (sys_pool_ptr != NU_NULL)
    {
#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
        /* Return the process heap pointer */
        status = PROC_Get_Memory_Pool(sys_pool_ptr);
#else
        /* Return system memory pool pointer */
        *sys_pool_ptr = &System_Memory;

        /* Set return status to success */
        status = NU_SUCCESS;
#endif
    }

    /* Check if uncached system memory pointer is valid */
    if (usys_pool_ptr != NU_NULL)
    {
#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))

        /* Return uncached system memory pool pointer */
        if (Uncached_Memory_Flag == NU_TRUE)
        {
            *usys_pool_ptr = &System_Memory;
        }
        else
        {
            *usys_pool_ptr = &Uncached_System_Memory;
        }

#else
        *usys_pool_ptr = &System_Memory;

#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

        /* Set return status to success */
        status = NU_SUCCESS;
    }

    /* Return status to caller */
    return (status);
}
