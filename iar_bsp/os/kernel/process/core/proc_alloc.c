/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       proc_alloc.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains functionality for obtaining and freeing
*       memory for use within processes.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Alloc
*       PROC_Free
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "proc_core.h"

/*************************************************************************
*
* FUNCTION
*
*       PROC_Alloc
*
* DESCRIPTION
*
*       TBD
*
* INPUTS
*
*       pointer                         Return pointer for allocated memory
*       size                            Size of allocation
*       alignment                       Requested memory alignment
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       Return value from allocate aligned memory call
*
*************************************************************************/
STATUS PROC_Alloc(VOID **pointer, UINT32 size, UINT32 alignment)
{
    static NU_MEMORY_POOL *sys_mem;

    /* Determine if system memory pool ptr needs to be obtained */
    if (sys_mem == NU_NULL)
    {
        /* Get system memory pool */
        NU_System_Memory_Get(&sys_mem, NU_NULL);
    }

    /* Return result of memory allocation */
    return (NU_Allocate_Aligned_Memory(sys_mem, pointer, size, alignment, NU_NO_SUSPEND));
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Free
*
* DESCRIPTION
*
*       TBD
*
* INPUTS
*
*       pointer                         Pointer to deallocate
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       Return value from allocate aligned memory call
*
*************************************************************************/
STATUS PROC_Free(VOID *pointer)
{
    /* Return result of memory deallocation */
    return (NU_Deallocate_Memory(pointer));
}
