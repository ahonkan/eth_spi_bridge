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
*       sck_dnssd_l.c
*
*   DESCRIPTION
*
*       This file contains the routines to look up the information
*       necessary to contact and use a specific service.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_DNS_SD_Look_Up
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
*       NU_DNS_SD_Look_Up
*
*   DESCRIPTION
*
*       This routine is used to look up the information necessary to
*       contact and use the named service;  the hostname of the machine
*       where that service is available, the port number on which the
*       service is listening, and (if present) TXT record attributes
*       describing properties of the service.
*
*   INPUTS
*
*       *name                   The specific instance of the service as
*                               returned by NU_DNS_SD_Refresh().
*       *type                   The service name.
*       *domain                 The domain of the service.  The domain
*                               "local." is the only domain supported at
*                               this time.
*       *status                 The status of the call; NU_SUCCESS upon
*                               successful completion, NU_INVALID_PARM
*                               if the input is invalid, NU_NO_ACTION if
*                               no records were found; otherwise, an
*                               operating-system specific error.
*
*   OUTPUTS
*
*       A pointer to the NU_DNS_SD_SERVICE structure filled in with the
*       data necessary to connect to that service.  The application must
*       free the memory returned by this routine when finished with the
*       data.
*
*************************************************************************/
NU_DNS_SD_SERVICE *NU_DNS_SD_Look_Up(CHAR *name, CHAR *type, CHAR *domain,
                                     STATUS *status)
{
#if (INCLUDE_MDNS == NU_TRUE)
    CHAR                *data;
    DNS_HOST            *srv_ptr, *txt_ptr;
    UINT16              record_len;
    NU_DNS_SD_SERVICE   *service_ptr = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input. */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (name == NU_NULL) || (type == NU_NULL) || (domain == NU_NULL) ||
         (status == NU_NULL) || (strcmp(domain, "local") != 0) )
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
                                 strlen(name) + strlen(type) + strlen(domain) + 3,
                                 NU_SUSPEND);

    if (*status == NU_SUCCESS)
    {
        /* Concatenate the type and domain. */
        strcpy(data, name);
        strcat(data, ".");
        strcat(data, type);
        strcat(data, ".");
        strcat(data, domain);

        /* Obtain the semaphore. */
        *status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

        if (*status == NU_SUCCESS)
        {
            /* Find the SRV record for this service.*/
            srv_ptr = DNS_Find_Matching_Host_By_Name(data, NU_NULL, DNS_TYPE_SRV);

            /* A SRV record is mandatory. */
            if (srv_ptr)
            {
                /* Initialize the length of the return pointer. */
                record_len = sizeof(NU_DNS_SD_SERVICE);

                /* Add memory for the length of the hostname and the null-terminator. */
                record_len += (strlen(srv_ptr->dns_record_data) + 1);

                /* Find the TXT record for this service.*/
                txt_ptr = DNS_Find_Matching_Host_By_Name(data, NU_NULL, DNS_TYPE_TXT);

                /* If a TXT record exists. */
                if (txt_ptr)
                {
                    /* Add memory for the length of the keys and the null-terminator. */
                    record_len += (txt_ptr->dns_h_length + 1);
                }

                /* Allocate memory for the return pointer. */
                *status = NU_Allocate_Memory(MEM_Cached, (VOID **)&service_ptr,
                                             record_len, NU_SUSPEND);

                if (*status == NU_SUCCESS)
                {
                    /* Set the pointer to the memory for the hostname. */
                    service_ptr->dns_hostname = (CHAR*)(service_ptr + 1);

                    /* Copy the hostname from the SRV record. */
                    strcpy(service_ptr->dns_hostname, srv_ptr->dns_record_data);

                    /* Copy the remaining fields from the SRV record. */
                    service_ptr->dns_prio = srv_ptr->dns_prio;
                    service_ptr->dns_weight = srv_ptr->dns_weight;
                    service_ptr->dns_port = srv_ptr->dns_port;

                    /* If a TXT record was found. */
                    if (txt_ptr)
                    {
                        /* Set the pointer to the memory for the keys. */
                        service_ptr->dns_keys =
                            (CHAR*)(&service_ptr->dns_hostname[strlen(service_ptr->dns_hostname)] + 1);

                        /* Copy the keys. */
                        strcpy(service_ptr->dns_keys, txt_ptr->dns_record_data);

                        /* Store the length of the keys. */
                        service_ptr->dns_key_len = txt_ptr->dns_h_length;
                    }

                    else
                    {
                        service_ptr->dns_key_len = 0;
                        service_ptr->dns_keys = NU_NULL;
                    }
                }
            }

            else
            {
                *status = NU_NO_ACTION;
            }

            NU_Release_Semaphore(&DNS_Resource);
        }

        /* Deallocate the memory for the data. */
        NU_Deallocate_Memory(data);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (service_ptr);

#else

    if (status)
    {
        *status = NU_INVALID_PARM;
    }

    return (NU_NULL);

#endif

} /* NU_DNS_SD_Look_Up */
