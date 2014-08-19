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
* FILE NAME
*
*       dns_fhe.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Free_Host_Entry
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Free_Host_Entry
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Arrays declared in DNS.C */
extern CHAR  DNS_New_Host_Memory[];
extern UINT8 DNS_New_Host_Memory_Flags[];
#endif


/*************************************************************************
*
*   FUNCTION
*
*       NU_Free_Host_Entry
*
*   DESCRIPTION
*
*       This function frees the memory previously allocated for a host
*       entry.
*
*   INPUTS
*
*       *host_entry             A pointer to a structure to be free.
*
*   OUTPUTS
*
*       NU_SUCCESS              The memory was successfully deallocated.
*       NU_INVALID_POINTER      The supplied pointer is invalid.
*
*************************************************************************/
STATUS NU_Free_Host_Entry(NU_HOSTENT *host_entry)
{
    STATUS  status;
    NU_SUPERV_USER_VARIABLES

    /* Deallocate the memory */
    if (!host_entry)
        return (NU_INVALID_POINTER);

    NU_SUPERVISOR_MODE();

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Deallocate_Memory((VOID*)host_entry);

#else
    /* Turn the memory flag off to indicate an unused memory */
    DNS_New_Host_Memory_Flags[(UINT8)(((CHAR *)host_entry -
                               (CHAR *)DNS_New_Host_Memory) /
                               NET_HOSTS_MEMORY)] = NU_FALSE;
    status = NU_SUCCESS;
#endif

    NU_USER_MODE();

    return (status);

} /* NU_Free_Host_Entry */

