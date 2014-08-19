/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       proc_sym_use.c
*
*   COMPONENT
*
*       PROC - Nucleus Processes
*
*   DESCRIPTION
*
*       This file contains the symbol usage tracking system.  This system
*       tracks the use of symbols exported by a process by other
*       processes.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PROC_Symbols_Use
*       PROC_Symbols_Unuse
*       PROC_Symbols_Unuse_All
*       PROC_Symbols_In_Use
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*       [proc_mem_mgmt.h]
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/process/core/proc_core.h"

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
#include "os/kernel/process/mem_mgmt/proc_mem_mgmt.h"
#endif

/* Bit Field (UINT32) support macros */

/* Bit Field UINT32 Get Pointer - This macro equates to a pointer to the
   UINT32 containing the specified bit in the bit field. */
#define PROC_BIT_FIELD_UINT32_GET_PTR(pBitField,index)  ((UINT32*)(((UNSIGNED)(pBitField)) + ((index) / 32)))

/* Bit Field UINT32 Get Index - This macro equates to the index within
   the UINT32 containing the specified bit in the bit field. */
#define PROC_BIT_FIELD_UINT32_GET_IDX(index)            (31 - (((UINT32)(index)) & 0x0000001F))

/* Bit Field UINT32 Set Bit - This macro sets a bit in a UINT32. */
#define PROC_BIT_FIELD_UINT32_SET_BIT(value,index)      ((value) = ((value) | (ESAL_GE_MEM_32BIT_SET(index))))

/* Bit Field UINT32 Clear Bit - This macro clears a bit in a UINT32. */
#define PROC_BIT_FIELD_UINT32_CLEAR_BIT(value,index)    ((value) = ((value) & (ESAL_GE_MEM_32BIT_CLEAR(index))))


/*************************************************************************
*
*   FUNCTION
*
*      PROC_Symbols_Use
*
*   DESCRIPTION
*
*      Called to track use of symbols provided by one process (the owner)
*      to another process (the user).
*
*   INPUTS
*
*      sym_owner - The process that is providing the symbols.
*
*      sym_user - The process that is using the symbols.
*
*   OUTPUTS
*
*      NU_SUCCESS - Indicates successful operation.
*
*************************************************************************/
STATUS PROC_Symbols_Use(PROC_CB * sym_owner, PROC_CB * sym_user)
{
    UINT32 *    temp32_ptr;
    STATUS      status = NU_SUCCESS;

    /* Determine if user process is already using owners symbols.  Only
       update if this is a new use relationship. */
    temp32_ptr = PROC_BIT_FIELD_UINT32_GET_PTR(sym_user -> sym_using, sym_owner -> id);
    if (ESAL_GE_MEM_32BIT_TEST(*temp32_ptr, sym_owner -> id) == NU_FALSE)
    {
        /* Update owner process to indicate a new user of its symbols */
        sym_owner -> sym_used_count++;

        /* Update user process to indicate it is using symbols of owner */
        PROC_BIT_FIELD_UINT32_SET_BIT(*temp32_ptr, sym_owner -> id);
        sym_user -> sym_using_count++;

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE

        /* Share the process memory */
        status = PROC_Share_Memory(sym_owner, sym_user);

#endif

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*      PROC_Symbols_Unuse
*
*   DESCRIPTION
*
*      Called to track cessation of use of symbols provided by one process
*      (the owner) to another process (the user).
*
*   INPUTS
*
*      sym_owner - The process that is providing the symbols.
*
*      sym_user - The process that is using the symbols.
*
*   OUTPUTS
*
*      NU_SUCCESS - Indicates successful operation.
*
*************************************************************************/
STATUS PROC_Symbols_Unuse(PROC_CB * sym_owner, PROC_CB * sym_user)
{
    UINT32 *    temp32_ptr;
    STATUS      status = NU_SUCCESS;

    /* Update owner process to indicate a one less user of its resources */
    sym_owner -> sym_used_count--;

    /* Update user process to indicate it is no longer using resources of
       owner */
    temp32_ptr = PROC_BIT_FIELD_UINT32_GET_PTR(sym_user -> sym_using, sym_owner -> id);
    PROC_BIT_FIELD_UINT32_CLEAR_BIT(*temp32_ptr, sym_owner -> id);
    sym_user -> sym_using_count--;

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE

    /* Remove the shared memory regions */
    status = PROC_Unshare_Memory(sym_owner, sym_user);

#endif

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*      PROC_Symbols_Unuse_All
*
*   DESCRIPTION
*
*      Called to track cessation of use of symbols provided by all
*      processes to another process (the user).
*
*   INPUTS
*
*      sym_user - The process that is using the symbols.
*
*   OUTPUTS
*
*      NU_SUCCESS - Indicates successful operation.
*
*      <other> - Indicates (other) unexpected error occurred.
*
*************************************************************************/
STATUS PROC_Symbols_Unuse_All(PROC_CB * sym_user)
{
    STATUS          status = NU_SUCCESS;
    PROC_CB *       process;
    UNSIGNED        i;
    UINT32 *        temp32_ptr;

    /* Cease using any symbols this process is currently using. */
    i = 0;
    while ((status == NU_SUCCESS) &&
           (i < CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES) &&
           (sym_user -> sym_using_count > 0))
    {
        /* Consult the using array to determine if this process is using
           symbols of the process at the array index specified. */
        temp32_ptr = PROC_BIT_FIELD_UINT32_GET_PTR(sym_user -> sym_using, i);
        if (ESAL_GE_MEM_32BIT_TEST(*temp32_ptr, i) == NU_TRUE)
        {
            /* Attempt to get a pointer to the process providing the
               symbols to this process. */
            process = PROC_Get_Pointer(i);
            if (process != NU_NULL)
            {
                /* Cease using symbols from the providing process. */
                status = PROC_Symbols_Unuse(process, sym_user);
            }
        }

        i++;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*      PROC_Symbols_In_Use
*
*   DESCRIPTION
*
*      Called to indicate if a process has any symbols currently in use by
*      other processes.
*
*   INPUTS
*
*      sym_owner - The process that is providing the symbols.
*
*   OUTPUTS
*
*      NU_TRUE - Indicates that the process has symbols in use.
*
*      NU_FALSE - Indicates that the process has no symbols in use.
*
*************************************************************************/
BOOLEAN PROC_Symbols_In_Use(PROC_CB * sym_owner)
{
    return ((BOOLEAN)(sym_owner -> sym_used_count > 0));
}
