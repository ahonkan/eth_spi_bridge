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
*       sck_udr.c
*
*   DESCRIPTION
*
*       This file contains the routines to manage the DNS database of
*       records from the application layer.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Update_DNS_Record
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
*       NU_Update_DNS_Record
*
*   DESCRIPTION
*
*       This routine is used to create, delete or update a DNS record.
*
*   INPUTS
*
*       handle                  A handle for the record to update. If
*                               the record is being added, this value
*                               will be filled in by the routine.
*       dns_record              The updates to make to the record:
*                                   dns_name    Name of the record.
*                                   dns_type    Type of record; DNS_TYPE_A,
*                                               DNS_TYPE_AAAA, DNS_TYPE_PTR,
*                                               DNS_TYPE_SRV or DNS_TYPE_TXT
*                                               are currently supported.
*                                   dns_data    Data associated with the record.
*                                               If the record is A or AAAA; list
*                                               of IP addresses.  Otherwise,
*                                               data specific to the record.
*                                   dns_ttl     TTL of the record in seconds.
*                                   dns_family  Family of the IP addresses or
*                                               NU_FAMILY_UNSPEC if the record
*                                               does not reference an IP address.
*                                   dns_data_len If the record is A or AAAA;
*                                               number of IP addresses. Otherwise,
*                                               length of data in bytes.
*                                   dns_prio    If the type is DNS_TYPE_SRV,
*                                               the priority level of this service.
*                                   dns_weight  If the type is DNS_TYPE_SRV, the
*                                               weight value of this service.
*                                   dns_port    If the type is DNS_TYPE_SRV, the
*                                               port number over which this service
*                                               can be reached.
*                                   dns_flags   DNS_PERMANENT_ENTRY - the
*                                               entry will not be deleted.
*                                               DNS_UNIQUE_RECORD - the record
*                                               is unique - probing and
*                                               announcing will be performed.
*       action                  Add, delete or modify the record; NU_DNS_ADD,
*                               NU_DNS_DELETE, NU_DNS_UPDATE.  The TTL of any
*                               record type can be updated by this routine.
*                               Otherwise, only the data portion of the SRV
*                               and TXT records can be updated.
*
*   OUTPUTS
*
*       NU_SUCCESS              The action was successfully completed.
*       NU_INALID_PARM          One of the input parameters is invalid.
*       NU_NO_ACTION            The caller is requesting either to add a
*                               record and a matching record already
*                               exists, to delete a record that does
*                               not exist, or to update a record that
*                               it learned dynamically from the network
*                               or that does not exist.
*
*       Otherwise, an operating-system specific error code is returned.
*
*************************************************************************/
STATUS NU_Update_DNS_Record(UNSIGNED *handle, NU_DNS_HOST *dns_record,
                            UINT8 action)
{
    STATUS              status;
    DNS_HOST            *dns_ptr,
                        *new_dns_host;
    DNS_SRV_DATA        srv_data;
    VOID                *extra_data = NU_NULL;
#if (INCLUDE_MDNS == NU_TRUE)
    DV_DEVICE_ENTRY     *dev_ptr;
#endif

    NU_SUPERV_USER_VARIABLES

    /* handle must always be non-null.  dns_record can only be non-null if
     * this is a deletion.  All members of dns_record must be valid if this
     * is an addition.
     */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (!handle) || ((!dns_record) && (action != NU_DNS_DELETE)) ||
         ((action == NU_DNS_ADD) && ((!dns_record->dns_name) || (!dns_record->dns_data) ||
          (dns_record->dns_data_len <= 0) || (dns_record->dns_ttl <= 0) ||
          (
#if (INCLUDE_IPV4 == NU_TRUE)
          (dns_record->dns_family != NU_FAMILY_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
          &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
          (dns_record->dns_family != NU_FAMILY_IP6)
#endif
          && ((dns_record->dns_family != NU_FAMILY_UNSPEC) ||
              ((dns_record->dns_type != DNS_TYPE_PTR) &&
               (dns_record->dns_type != DNS_TYPE_SRV) &&
               (dns_record->dns_type != DNS_TYPE_TXT))))) ) )
    {
        return (NU_INVALID_PARM);
    }
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore. */
    status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* If this is a call to add a new record. */
        if (action == NU_DNS_ADD)
        {
            dns_ptr = NU_NULL;

            /* Ensure the record does not already exist. */
            if ( (dns_record->dns_type == DNS_TYPE_PTR) &&
                 (dns_record->dns_family != NU_FAMILY_UNSPEC) )
            {
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
                /* A PTR record can have only one IP address. */
                if (dns_record->dns_data_len != 1)
                {
                    status = NU_INVALID_PARM;
                }

                else
#endif
                {
                    /* Look for an already existing record. */
                    dns_ptr = DNS_Find_Matching_Host_By_Addr((UINT8*)&dns_record->dns_data[0],
                                                             dns_record->dns_family);
                }
            }

            else
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
                if (
#if (INCLUDE_IPV4 == NU_TRUE)
                        (dns_record->dns_type == DNS_TYPE_A)
#if (INCLUDE_IPV6 == NU_TRUE)
                        ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                        (dns_record->dns_type == DNS_TYPE_AAAA)
#endif
                        || ( ((dns_record->dns_type == DNS_TYPE_PTR) ||
                              (dns_record->dns_type == DNS_TYPE_SRV) ||
                              (dns_record->dns_type == DNS_TYPE_TXT))&&
                             (dns_record->dns_family == NU_FAMILY_UNSPEC) )
                    )
#endif
            {
                /* Look for an already existing record. */
                dns_ptr = DNS_Find_Matching_Host_By_Name(dns_record->dns_name,
                                                         dns_record->dns_family == NU_FAMILY_UNSPEC ?
                                                         dns_record->dns_data : NU_NULL,
                                                         dns_record->dns_type);
            }

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
            else
            {
                status = NU_INVALID_PARM;
            }
#endif

            if (status == NU_SUCCESS)
            {
                /* If a record already exists, return an error and the handle. */
                if (dns_ptr)
                {
                    *handle = dns_ptr->dns_id;
                    status = NU_NO_ACTION;
                }

                else
                {
                    /* If this is a SRV record, copy the SRV specific data. */
                    if (dns_record->dns_type == DNS_TYPE_SRV)
                    {
                        srv_data.dns_srv_weight = dns_record->dns_weight;
                        srv_data.dns_srv_prio = dns_record->dns_prio;
                        srv_data.dns_srv_port = dns_record->dns_port;

                        extra_data = (VOID*)&srv_data;
                    }

                    /* Add the record. */
                    dns_ptr = DNS_Add_Host(dns_record->dns_name, (CHAR*)dns_record->dns_data,
                                           dns_record->dns_ttl, dns_record->dns_family,
                                           dns_record->dns_type, dns_record->dns_data_len,
                                           dns_record->dns_flags | DNS_LOCAL_RECORD,
                                           extra_data);

                    /* If the record was successfully added. */
                    if (dns_ptr)
                    {
                        /* Return this handle to the user. */
                        *handle = dns_ptr->dns_id;

#if (INCLUDE_MDNS == NU_TRUE)
#if (INCLUDE_IPV4 == NU_TRUE)
                        if (dns_record->dns_family == NU_FAMILY_IP)
                        {
                            /* Get a pointer to the device assigned the address in
                             * the record.
                             */
                            dev_ptr = DEV_Get_Dev_By_Addr((UINT8*)&dns_record->dns_data[0]);
                        }
#if (INCLUDE_IPV6 == NU_TRUE)
                        else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                        if (dns_record->dns_family == NU_FAMILY_IP6)
                        {
                            /* Get a pointer to the device assigned the address in
                             * the record.
                             */
                            dev_ptr = DEV6_Get_Dev_By_Addr((UINT8*)&dns_record->dns_data[0]);
                        }
#endif

                        else
                        {
                            dev_ptr = NU_NULL;
                        }

                        if (dev_ptr)
                        {
                            /* Store the device index of the interface associated with this entry. */
                            dns_ptr->mdns_dev_index = dev_ptr->dev_index;

                            /* If the caller wants this record to be unique on the link,
                             * and the record is not a PTR record, kick off probing and
                             * announcing.
                             */
                            if ( (dns_ptr->dns_flags & DNS_UNIQUE_RECORD) &&
                                 (dns_ptr->dns_type != DNS_TYPE_PTR) )
                            {
                                /* Allocate memory for the timer. */
                                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&dns_ptr->mdns_timer,
                                                            sizeof(NU_TIMER), NU_NO_SUSPEND);

                                if (status == NU_SUCCESS)
                                {
                                    /* Zero out the memory. */
                                    memset(dns_ptr->mdns_timer, 0, sizeof(NU_TIMER));

                                    /* Create the probing timer.  Zero is an invalid initial timeout, so
                                     * ensure the timer can never be set to zero.
                                     */
                                    status = NU_Create_Timer(dns_ptr->mdns_timer, "MDNS",
                                                             MDNS_Event_Handler, dns_ptr->dns_id,
                                                             (UTL_Rand() % MDNS_PROBE_DELAY) + 1, 0,
                                                             NU_ENABLE_TIMER);
                                }

                                /* Set the state to probing. */
                                if (status == NU_SUCCESS)
                                {
                                    dns_ptr->mdns_state = MDNS_PROBING;
                                }

                                else
                                {
                                    status = NU_INVALID_PARM;
                                }
                            }
                        }

                        /* If the IP address should exist on the node, return an
                         * error.
                         */
                        else if (dns_record->dns_family != NU_FAMILY_UNSPEC)
                        {
                            status = NU_INVALID_PARM;
                        }

                        if (status != NU_SUCCESS)
                        {
                            /* Delete the record that was just added since an error
                             * occurred trying to start up probing and announcing.
                             */
                            DNS_Delete_Host(dns_ptr);
                        }
#endif
                    }

                    else
                    {
                        status = NU_NO_MEMORY;
                    }
                }
            }
        }

        /* If the request is to delete the record. */
        else if (action == NU_DNS_DELETE)
        {
            /* Get a pointer to the record. */
            dns_ptr = DNS_Find_Host_By_ID(*handle);

            /* If the record does not exist or has been flagged for deletion. */
            if ( (!dns_ptr) ||
                 ((dns_ptr->dns_ttl == 0) && (dns_ptr->dns_flags & DNS_DELAY_RESPONSE)) )
            {
                status = NU_NO_ACTION;
            }

            /* Delete the entry and return success. */
            else
            {
                DNS_Delete_Host(dns_ptr);
            }
        }

        /* If the request is to update an existing entry. */
        else if (action == NU_DNS_UPDATE)
        {
            status = NU_NO_ACTION;

            /* Get a pointer to the record. */
            dns_ptr = DNS_Find_Host_By_ID(*handle);

            if ( (dns_ptr) && (dns_ptr->dns_ttl != 0) )
            {
                /* The application can only update records that it owns.  It cannot
                 * update records that it has learned on the network.
                 */
                if (dns_ptr->dns_flags & DNS_LOCAL_RECORD)
                {
                    status = NU_SUCCESS;

                    /* The application can change the TTL of any type of record. */
                    dns_ptr->dns_ttl = dns_record->dns_ttl * SCK_Ticks_Per_Second;

                    /* If the entry is not permanent, add the current clock to the value
                     * so we can tell when the entry times out.
                     */
                    if (!(dns_ptr->dns_flags & DNS_PERMANENT_ENTRY))
                    {
                        dns_ptr->dns_ttl += NU_Retrieve_Clock();
                    }

                    /* The application can change the weight, port and priority of a
                     * SRV record.
                     */
                    if (dns_ptr->dns_type == DNS_TYPE_SRV)
                    {
                        dns_ptr->dns_weight = dns_record->dns_weight;
                        dns_ptr->dns_prio = dns_record->dns_prio;
                        dns_ptr->dns_port = dns_record->dns_port;
                    }

                    /* If it's a TXT record, the application can update the
                     * key/value pairs.
                     */
                    else if (dns_ptr->dns_type == DNS_TYPE_TXT)
                    {
                        /* Create a new record for the new data.  The TTL could also have been
                         * updated.
                         */
                        new_dns_host = DNS_Add_Host(dns_ptr->dns_name, dns_record->dns_data,
                                                    dns_record->dns_ttl, dns_ptr->dns_family,
                                                    DNS_TYPE_TXT, dns_record->dns_data_len,
                                                    dns_ptr->dns_flags | DNS_LOCAL_RECORD,
                                                    NU_NULL);

                        if (new_dns_host)
                        {
                            /* Return a pointer to the new handle. */
                            *handle = new_dns_host->dns_id;

#if (INCLUDE_MDNS == NU_TRUE)
                            new_dns_host->mdns_dev_index = dns_ptr->mdns_dev_index;
#endif

                            /* Flag the host as unique so a Goodbye packet is not
                             * sent when the record is deleted.
                             */
                            dns_ptr->dns_flags |= DNS_UNIQUE_RECORD;

                            /* Delete the existing host. */
                            DNS_Delete_Host(dns_ptr);
                        }

                        else
                        {
                            status = NU_NO_MEMORY;
                        }
                    }
                }

                /* The application cannot update foreign records. */
                else
                {
                    status = NU_NO_ACTION;
                }
            }
        }

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Update_DNS_Record */
