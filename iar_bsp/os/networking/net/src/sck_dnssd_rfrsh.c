/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       sck_dnssd_rfrsh.c
*
*   DESCRIPTION
*
*       This file contains the routines to retrieve instances of a service
*       on the network that were discovered via NU_DNS_SD_Browse().
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_DNS_SD_Refresh
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

extern DNS_HOST_LIST    DNS_Hosts;

/*************************************************************************
*
*   FUNCTION
*
*       NU_DNS_SD_Refresh
*
*   DESCRIPTION
*
*       This routine is used to retrieve a list of instances on the
*       network that offer the specified service.  The routine
*       NU_DNS_SD_Browse() must have been invoked prior to calling
*       this routine, and the calling thread must have received a
*       signal that records have been found corresponding to the
*       prior NU_DNS_SD_Browse() request.
*
*   INPUTS
*
*       *type                   The name of the service for which to
*                               retrieve the found instances.
*       *domain                 The domain of scope for the respective
*                               instances.  Only the domain local. is
*                               currently supported.
*       *status                 The status of the call; NU_SUCCESS upon
*                               successful completion, NU_INVALID_PARM
*                               if the input is invalid, NU_NO_ACTION if
*                               no records were found; otherwise, an
*                               operating-system specific error.  Note
*                               that entries could still be returned in
*                               the case of an error, which indicates
*                               that memory could not be allocated for
*                               all available entries.
*
*   OUTPUTS
*
*       A list of instances that provide the service requested or NU_NULL
*       if no instances have been found.  The application must free the
*       memory returned by this routine when finished with the data.
*
*************************************************************************/
NU_DNS_SD_INSTANCE *NU_DNS_SD_Refresh(CHAR *type, CHAR *domain, STATUS *status)
{
#if (INCLUDE_MDNS == NU_TRUE)
    CHAR                *data;
    DNS_HOST            *l_host;
    NU_DNS_SD_INSTANCE  *return_ptr = NU_NULL, *instance_ptr, *previous_ptr = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input. */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (type == NU_NULL) || (domain == NU_NULL) || (status == NU_NULL) ||
         (strcmp(domain, "local") != 0) )
    {
        if (status)
        {
            *status = NU_INVALID_PARM;
        }

        return (NU_NULL);
    }
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory for the concatenated name. */
    *status = NU_Allocate_Memory(MEM_Cached, (VOID **)&data,
                                 strlen(type) + strlen(domain) + 2, NU_SUSPEND);

    if (*status == NU_SUCCESS)
    {
        /* Concatenate the type and domain. */
        strcpy(data, type);
        strcat(data, ".");
        strcat(data, domain);

        /* Obtain the semaphore. */
        *status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

        if (*status == NU_SUCCESS)
        {
            /* Find all records that match the service. */
            for (l_host = DNS_Hosts.dns_head; l_host; l_host = l_host->dns_next)
            {
                /* Find a PTR record matching the service requested. */
                if ( (l_host->dns_type == DNS_TYPE_PTR) &&
                     (NU_STRICMP((const char *)l_host->dns_name, data) == 0) )
                {
                    /* We found a match.  If this is a permanent entry, or if the ttl
                     * has not expired then return it.  Do not return entries that
                     * are currently flagged for deletion.
                     */
                    if ( ((l_host->dns_flags & DNS_PERMANENT_ENTRY) ||
                          (INT32_CMP(l_host->dns_ttl, NU_Retrieve_Clock()) > 0)) &&
                          (!(l_host->dns_flags & DNS_DELAY_RESPONSE)) )
                    {
                        /* Allocate memory for the new instance. */
                        *status = NU_Allocate_Memory(MEM_Cached, (VOID **)&instance_ptr,
                                                     sizeof(NU_DNS_SD_INSTANCE) +
                                                     strlen(l_host->dns_record_data) + 1, NU_SUSPEND);

                        if (*status == NU_SUCCESS)
                        {
                            /* Set the pointer to the memory for the name. */
                            instance_ptr->dns_sd_name = (CHAR *)(instance_ptr + 1);

                            /* Set up the data structure. */
                            strcpy(instance_ptr->dns_sd_name, l_host->dns_record_data);
                            instance_ptr->dns_sd_next = NU_NULL;

                            /* Set up the next pointer for the previous entry. */
                            if (previous_ptr)
                            {
                                previous_ptr->dns_sd_next =
                                    (NU_DNS_SD_INSTANCE*)instance_ptr;
                            }

                            /* This is the first instance - return a pointer to
                             * this entry.
                             */
                            else
                            {
                                return_ptr = instance_ptr;
                            }

                            /* Save this entry as the previous. */
                            previous_ptr = instance_ptr;
                        }

                        /* If memory is not available for this entry, return
                         * an error indicating that there are more entries,
                         * but not enough memory for the allocation.
                         */
                        else
                        {
                            break;
                        }
                    }
                }
            }

            NU_Release_Semaphore(&DNS_Resource);

            /* If no records were found and an error has not occurred. */
            if ( (*status == NU_SUCCESS) && (!return_ptr) )
            {
                *status = NU_NO_ACTION;
            }
        }

        /* Deallocate the memory for the data. */
        NU_Deallocate_Memory(data);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (return_ptr);

#else

    if (status)
    {
        *status = NU_INVALID_PARM;
    }

    return (NU_NULL);

#endif

} /* NU_DNS_SD_Refresh */
