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
*       sck_dnssd_r.c
*
*   DESCRIPTION
*
*       This file contains the routines to register and unregister a
*       service on the specified domain.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_DNS_SD_Register_Service
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern DNS_HOST_LIST    DNS_Hosts;

/*************************************************************************
*
*   FUNCTION
*
*       NU_DNS_SD_Register_Service
*
*   DESCRIPTION
*
*       This routine is used to register and unregister a service on
*       the specified domain in the network using a specified network
*       interface. The same service can be registered across multiple
*       interfaces
*
*   INPUTS
*
*       action                  NU_DNS_START to start the service and
*                               NU_DNS_STOP to stop the service
*       name                    Any arbitrary string of length less than 64
*                               bytes containing legal unicode characters.
*       type                    The name of the service to be registered
*       domain                  The domain in which to register the
*                               service.  Only the domain ".local" is
*                               currently supported.
*       port                    The TCP or UDP port number upon which
*                               the service is listening
*       keys                    Additional (optional) key=value pairs
*                               that must be stored in the advertised
*                               service's TXT record. Each pair must be
*                               NULL terminated.
*       if_index                Index of the interface on which the
*                               service can be reached.
*
*   OUTPUTS
*
*       NU_SUCCESS              Service was successfully [un]registered.
*       NU_INVALID_PARM         An input parameter is invalid.
*       NU_NO_MEMORY            There is not enough memory to register
*                               the service.
*       An operating system specific error is returned otherwise.
*
*************************************************************************/
STATUS NU_DNS_SD_Register_Service(INT action, CHAR *name, CHAR *type,
                                  CHAR *domain, UINT16 port, CHAR *keys,
                                  INT32 if_index)
{
    STATUS              status;

#if (INCLUDE_MDNS == NU_TRUE)
    DNS_HOST            *ptr_ptr = NU_NULL,
                        *srv_ptr = NU_NULL,
                        *txt_ptr = NU_NULL;
    CHAR                *data;
    DV_DEVICE_ENTRY     *dev;
    BOOLEAN             ptr_added = NU_FALSE,
                        srv_added = NU_FALSE,
                        txt_added = NU_FALSE;
    DNS_SRV_DATA        srv_data;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If no name was specified use a default name */
    if (name == NU_NULL)
        name = DNS_DEFAULT_INSTANCE_NAME;

    /* Validate the input. */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (strlen(name) > 63) || (type == NU_NULL) ||
         (domain == NU_NULL) || (strcmp(domain, "local") != 0) ||
         (if_index < 0) || (action < NU_DNS_START) ||
         (action > NU_DNS_STOP) )
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }
#endif

    /* Allocate memory for the concatenated name. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&data,
                               strlen(name) + strlen(type) + strlen(domain) + 3,
                               NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Obtain the semaphore. */
        status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Concatenate the name, type and domain. */
            strcpy(data, name);
            strcat(data, ".");
            strcat(data, type);
            strcat(data, ".");
            strcat(data, domain);

            /* Check to see if the records for the service being
             * added already exists.
             */
            /* Look for an already existing PTR record. */
            ptr_ptr = DNS_Find_Matching_Host_By_Name((data + strlen(name) + 1),
                                                     data, DNS_TYPE_PTR);

            /* Look for an already existing SRV record. */
            srv_ptr = DNS_Find_Matching_Host_By_Name(data, NU_NULL,
                                                     DNS_TYPE_SRV);

            /* Look for an already existing TXT record. */
            txt_ptr = DNS_Find_Matching_Host_By_Name(data, NU_NULL,
                                                     DNS_TYPE_TXT);

            if (action == NU_DNS_STOP)
            {
                /* If the call is to un-register the service, delete the
                 * respective records.
                 */
                if (ptr_ptr)
                {
                    /* Delete the PTR record */
                    DNS_Delete_Host(ptr_ptr);
                }

                if (srv_ptr)
                {
                    /* Delete the SRV record */
                    DNS_Delete_Host(srv_ptr);
                }

                if (txt_ptr)
                {
                    /* Delete the TXT record */
                    DNS_Delete_Host(txt_ptr);
                }
            }


            else if (action == NU_DNS_START)
            {
                /* If the call is to register the service, check and if the
                 * required records already exist. If not, add them.
                 */

                /* Check to see if a PTR record matching the service already
                 * exists.
                 */
                if (ptr_ptr == NU_NULL)
                {
                    /* Add a new PTR record */
                    ptr_ptr = DNS_Add_Host((data + strlen(name) + 1),
                                           data, DNS_SD_DEFAULT_TTL,
                                           NU_FAMILY_UNSPEC, DNS_TYPE_PTR,
                                           strlen(data),
                                           DNS_LOCAL_RECORD | DNS_PERMANENT_ENTRY,
                                           NU_NULL);

                    if (ptr_ptr)
                    {
                        /* Successfully added a PTR record for the
                         * service
                         */
                        ptr_added = NU_TRUE;
                    }
                    else
                    {
                        /* Failed to add a PTR record */
                        status = NU_NO_MEMORY;
                    }
                }

                /* Check to see if a SRV record matching the service already
                 * exists.
                 */
                if ( (srv_ptr == NU_NULL) && (status == NU_SUCCESS) )
                {
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Retrieve the device pointer for the interface on
                         * which the service is being requested.
                         */
                        dev = DEV_Get_Dev_By_Index(if_index);

                        if (dev)
                        {
                            /* Set up the SRV specific parameters. */
                            srv_data.dns_srv_weight = DNS_SD_DEFAULT_WEIGHT;
                            srv_data.dns_srv_prio = DNS_SD_DEFAULT_PRIO;
                            srv_data.dns_srv_port = port;

                            /* Add a new SRV record */
                            srv_ptr = DNS_Add_Host(data, (dev->dev_host_name),
                                                   DNS_SD_DEFAULT_TTL, NU_FAMILY_UNSPEC,
                                                   DNS_TYPE_SRV, strlen(dev->dev_host_name),
                                                   DNS_LOCAL_RECORD | DNS_PERMANENT_ENTRY,
                                                   &srv_data);

                            if (srv_ptr)
                            {
                                /* Successfully added a new SRV record */
                                srv_added = NU_TRUE;
                            }
                            else
                            {
                                status = NU_NO_MEMORY;
                            }
                        }
                        else
                        {
                            /* Device was not found, maybe the interface index is invalid? */
                            status = NU_INVALID_PARM;
                        }

                        /* Release the TCP semaphore */
                        NU_Release_Semaphore(&TCP_Resource);
                    }
                }

                /* Check to see if a TXT record matching the service already
                 * exists.
                 */
                if ( (txt_ptr == NU_NULL) && (status == NU_SUCCESS) )
                {
                    /* Add the new TXT record */
                    txt_ptr = DNS_Add_Host(data, keys, DNS_SD_DEFAULT_TTL,
                                           NU_FAMILY_UNSPEC, DNS_TYPE_TXT,
                                           keys != NU_NULL ? strlen(keys) : 0,
                                           DNS_LOCAL_RECORD | DNS_PERMANENT_ENTRY,
                                           NU_NULL);

                    if (txt_ptr)
                    {
                        /* Successfully added a new TXT record */
                        txt_added = NU_TRUE;
                    }

                    else
                    {
                        status = NU_NO_MEMORY;
                    }
                }

                if (status !=  NU_SUCCESS)
                {
                    /* If something bad happened, delete all the records
                     * that were just added.
                     */
                    if ( (ptr_ptr) && (ptr_added) )
                        DNS_Delete_Host(ptr_ptr);

                    if ( (srv_ptr) && (srv_added) )
                        DNS_Delete_Host(srv_ptr);

                    if ( (txt_ptr) && (txt_added) )
                        DNS_Delete_Host(txt_ptr);
                }
            }

            /* Release the DNS semaphore */
            NU_Release_Semaphore(&DNS_Resource);
        }

        /* Deallocate the memory for the data. */
        NU_Deallocate_Memory(data);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

#else

    status = NU_INVALID_PARM;

#endif

    return (status);

} /* NU_DNS_SD_Register_Service */

