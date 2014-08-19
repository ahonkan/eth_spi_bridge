/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       sck_if_fni.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_IF_FreeNameIndex.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_IF_FreeNameIndex
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

extern UINT8   NET_IF_NameIndex_Memory_Flag;

#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_IF_FreeNameIndex
*
*   DESCRIPTION
*
*       This function frees the dynamic memory that was allocated by
*       NU_IF_NameIndex().
*
*   INPUTS
*
*       *ptr                    A pointer to the memory to free.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_IF_FreeNameIndex(struct if_nameindex *ptr)
{
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (ptr == NU_NULL)
        return;

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    if (NU_Deallocate_Memory(ptr) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory",
                       NERR_SEVERE, __FILE__, __LINE__);

#else

    /* Indicate that the memory is free to be used */
    NET_IF_NameIndex_Memory_Flag = NU_FALSE;

#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

} /* NU_IF_FreeNameIndex */
