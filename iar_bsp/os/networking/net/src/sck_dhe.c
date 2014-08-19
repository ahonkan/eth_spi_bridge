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
*       sck_dhe.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Delete_Host_Entry.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Delete_Host_Entry
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Delete_Host_Entry
*
*   DESCRIPTION
*
*       Given a pointer to a host entry, this function deletes that
*       host entry from the list of cached entries.
*
*   INPUTS
*
*       *host_entry             A pointer to the host entry to be
*                               deleted.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         A passed in parameter is invalid.
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_Delete_Host_Entry(NU_HOSTENT *host_entry)
{
    STATUS      status;
    UINT32      target_id = 0; /* ID of a record will never be zero. */
    DNS_HOST    *target_host;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the pointer that was passed into the routine */
    if (!host_entry)
        return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore. */
    status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        /* If the family argument is NU_FAMILY_IP6, then search for an
         * IPv6 record.
         */
        if (host_entry->h_addrtype == NU_FAMILY_IP6)
            target_host = DNS_Find_Matching_Host_By_Name(host_entry->h_name, NU_NULL,
                                                         DNS_TYPE_AAAA);

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        /* Search for a matching IPv4 host in the existing database. */
        target_host = DNS_Find_Matching_Host_By_Name(host_entry->h_name, NU_NULL,
                                                     DNS_TYPE_A);
#endif

        /* If a matching entry was found, remove the host from the host list */
        if (target_host)
        {
            /* Save a copy of the ID. */
            target_id = target_host->dns_id;
        }

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Ensure an entry was found.  NU_SUCCESS is returned even if no entry
     * is found to delete.
     */
    if (target_id != 0)
    {
        /* Call the API to delete the record. */
        NU_Update_DNS_Record(&target_id, NU_NULL, NU_DNS_DELETE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Delete_Host_Entry */
