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
*       sck_gnbn.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Get_IP_Node_By_Name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Get_IP_Node_By_Name
*       DNS_Find_Host_By_Name
*       DNS_Merge_Records
*       DNS_Sort_Records
*       DNS_Compare_Addrs
*       DNS_AddrScope
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/dns6.h"
#endif

STATIC VOID DNS_Merge_Records(CHAR *addr_buff, DNS_HOST *ipv6_host,
                              DNS_HOST *ipv4_host, UINT16 resolved_addrs,
                              INT16 family);

extern NU_HOSTENT   *DNS_Create_Host_Entry(CHAR *, INT16, const CHAR *, STATUS *);

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_IP_Node_By_Name
*
*   DESCRIPTION
*
*       Given a host name and family, this function searches for the
*       host in  the "hosts file".  A NU_HOSTENT data structure is
*       allocated, filled in with the appropriate information and a
*       pointer to the data structure returned to the application.
*
*       After the application has finished using the data structure
*       returned by this function, a call to NU_Free_Host_Entry must
*       be made to free the memory allocated by the DNS lookup.
*
*   INPUTS
*
*       *name                   Pointer to the name of the host to look
*                               for.
*       family                  The family of the target host.  Either
*                               NU_FAMILY_IP or NU_FAMILY_IP6
*       flags                   Flag to determine which type of records
*                               are searched.
*       *error_num              Status returned to the caller:
*
*                               NU_SUCCESS          Success
*                               NU_NO_MEMORY        No memory so host
*                                                   was not added
*                               NU_DNS_ERROR        DNS could not resolve
*                               NU_NO_DNS_SERVER    No DNS server exists
*                               NU_INVALID_PARM     A passed in parameter
*                                                   is invalid.
*                               NU_INVALID_LABEL    One of the parameters
*                                                   was a NULL pointer.
*
*   OUTPUTS
*
*       NU_HOSTENT*             Pointer to data structure containing
*                               results.
*       NU_NULL                 Failure.
*
*************************************************************************/
NU_HOSTENT *NU_Get_IP_Node_By_Name(CHAR *name, INT16 family, INT16 flags,
                                   STATUS *error_num)
{
    NU_HOSTENT  *host_entry = NU_NULL;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Check for a NULL pointer. */
    if ( (!name) || (!error_num) || ((family != SK_FAM_IP)

#if (INCLUDE_IPV6 == NU_TRUE)
         && (family != SK_FAM_IP6)
#endif
        ) )
    {
        if (error_num)
            *error_num = NU_INVALID_PARM;

        return (host_entry);
    }

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_DNS != NU_TRUE)
    UNUSED_PARAMETER(flags);
#endif

#if (INCLUDE_DNS == NU_TRUE)

    /* Obtain the semaphore. */
    if (NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        host_entry = DNS_Find_Host_By_Name(name, family, flags, error_num);

        NU_Release_Semaphore(&DNS_Resource);
    }
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (host_entry);

} /* NU_Get_IP_Node_By_Name */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Find_Host_By_Name
*
*   DESCRIPTION
*
*       This function searches the "hosts" file for a host with a matching
*       name and of the corresponding family.  If the host is not found,
*       DNS is used to resolve the host name if possible.
*
*   INPUTS
*
*       *name                   The name of the host to find.
*       family                  The family type of the address to return.
*       flags                   Flags used in conjunction with a family type
*                               of NU_FAMILY_IP6.
*       *error_num              A pointer to the status of the resolve.
*
*                               NU_SUCCESS          Success
*                               NU_NO_MEMORY        No memory
*                               NU_DNS_ERROR        DNS could not resolve
*                               NU_NO_DNS_SERVER    No DNS server exists
*                               NU_INVALID_PARM     Type was not valid
*                               NU_INVALID_LABEL    Label was not correct
*
*   OUTPUTS
*
*       NU_HOSTENT*             A pointer to the host entry.
*       NU_NULL                 The name could not be resolved.
*
******************************************************************************/
NU_HOSTENT *DNS_Find_Host_By_Name(CHAR *name, INT16 family, INT16 flags,
                                  STATUS *error_num)
{
    CHAR        ip_addr[DNS_MAX_IP_ADDRS][MAX_ADDRESS_SIZE];
#if (INCLUDE_DNS == NU_TRUE)
    UNSIGNED    ttl = 0;
#endif
    INT16       resolved_addrs = 0;
    NU_HOSTENT  *host_entry = NU_NULL;
    DNS_HOST    *ipv4_host = NU_NULL;

#if (INCLUDE_IPV6 == NU_TRUE)
    DNS_HOST    *ipv6_host = NU_NULL;
#if (INCLUDE_DNS == NU_TRUE)
#if (INCLUDE_IPV4 == NU_TRUE)
    STATUS      ipv4_status;
    INT16       ipv6_resolved_addrs = 0;
#endif
#endif
#endif

    /* Zero out the memory being used for the resolve */
    UTL_Zero(ip_addr, DNS_MAX_IP_ADDRS * MAX_ADDRESS_SIZE);

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* If the family argument is NU_FAMILY_IP6, then a query is made for
     * AAAA records.
     */
    if (family == NU_FAMILY_IP6)
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* Only search for IPv6 addresses if the DNS_ADDRCONFIG flag is
         * set and there is a local IPv6 address on the device or if the
         * DNS_ADDRCONFIG flag is not set.
         */
        if ( (!(flags & DNS_ADDRCONFIG)) || (DEV6_Find_Configured_Device()) )
        {
            /* Search for a matching IPv6 host in the existing database. */
            ipv6_host = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_AAAA);

            /* If a match does not exist in the database, perform address
             * resolution.
             */
#if (INCLUDE_DNS == NU_TRUE)
            if (!(ipv6_host))
            {
                /* Attempt to find an AAAA record */
                *error_num = DNS_Resolve(name, (CHAR*)ip_addr, &ttl, DNS_TYPE_AAAA,
                                         family, &resolved_addrs);

                /* If an AAAA record was found, store it in the database */
                if (*error_num == NU_SUCCESS)
                {
                    if (DNS_Add_Host(name, (CHAR*)ip_addr, ttl, NU_FAMILY_IP6,
                                     DNS_TYPE_AAAA, resolved_addrs, 0, NU_NULL) == NU_NULL)
                        NLOG_Error_Log("Failed to add DNS host", NERR_SEVERE,
                                       __FILE__, __LINE__);
                }

#if (INCLUDE_IPV4 == NU_TRUE)
                /* Store off the number of IPv6 addresses resolved */
                ipv6_resolved_addrs = resolved_addrs;
#endif
            }
            else
                *error_num = NU_SUCCESS;
#endif
        }

        /* The DNS_ADDRCONFIG flag was set, but there is no device configured
         * in the system with an IPv6 address.
         */
        else
            *error_num = NU_DNS_ERROR;

#if (INCLUDE_IPV4 == NU_TRUE)

        /* RFC 2553 section 6.1 - If the DNS_V4MAPPED flag is specified
         * along with NU_FAMILY_IP6, then the caller will accept IPv4-Mapped
         * IPv6 addresses if no IPv6 addresses are found.  When DNS_ALL is
         * logically OR'd with DNS_V4MAPPED, then the caller wants IPv4 and
         * IPv6 addresses.
         */
        if ( (((*error_num != NU_SUCCESS) && (flags & DNS_V4MAPPED)) ||
              ((flags & (DNS_ALL | DNS_V4MAPPED)) == (DNS_ALL | DNS_V4MAPPED))) &&
             ((!(flags & DNS_ADDRCONFIG)) || (IP_Find_Configured_Device())) )
        {
            /* Search for a matching IPv4 host in the existing database. */
            ipv4_host = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_A);

            /* If a match does not exist in the database, perform address
             * resolution.
             */
#if (INCLUDE_DNS == NU_TRUE)
            if (!(ipv4_host))
            {
                /* If the user will accept IPv4-Mapped IPv6 addresses,
                 * look for A records for the target.
                 */
                ipv4_status = DNS_Resolve(name, (CHAR*)ip_addr, &ttl, DNS_TYPE_A,
                                          family, &resolved_addrs);

                /* If an A record was found, add it to the database as an IPv4
                 * record.
                 */
                if (ipv4_status == NU_SUCCESS)
                {
                    if (DNS_Add_Host(name, (CHAR*)&(ip_addr[ipv6_resolved_addrs]),
                                     ttl, NU_FAMILY_IP, DNS_TYPE_A,
                                     (INT16)(resolved_addrs - ipv6_resolved_addrs), 0,
                                     NU_NULL) == NU_NULL)
                        NLOG_Error_Log("Failed to add DNS host", NERR_SEVERE,
                                       __FILE__, __LINE__);
                }

                /* If the IPv6 resolve was not successful, but the IPv4 resolve
                 * was successful, set the error code to the status of the IPv4
                 * resolve.
                 */
                if (*error_num != NU_SUCCESS)
                    *error_num = ipv4_status;
            }
            else
                *error_num = NU_SUCCESS;
#endif
        }
#endif
    }
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* If the family argument is NU_FAMILY_IP, then a query is made for
     * A records.
     */
    else
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    {
        if ( (!(flags & DNS_ADDRCONFIG)) || (IP_Find_Configured_Device()) )
        {
            /* Search for a matching IPv4 host in the database. */
            ipv4_host = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_A);

            /* If a match does not already exist in the database, perform
             * address resolution.
             */
#if (INCLUDE_DNS == NU_TRUE)
            if (!ipv4_host)
            {
                /* Attempt to find an A record */
                *error_num = DNS_Resolve(name, (CHAR*)ip_addr, &ttl, DNS_TYPE_A,
                                         family, &resolved_addrs);

                /* If an A record was found, add it to the database */
                if (*error_num == NU_SUCCESS)
                {
                    if (DNS_Add_Host(name, (CHAR*)ip_addr, ttl, family, DNS_TYPE_A,
                                     resolved_addrs, 0, NU_NULL) == NU_NULL)
                        NLOG_Error_Log("Failed to add DNS host", NERR_SEVERE,
                                       __FILE__, __LINE__);
                }
            }
            else
                *error_num = NU_SUCCESS;
#endif
        }

        /* The DNS_ADDRCONFIG flag was set, but there is no device in the
         * system configured with an IPv4 address.
         */
        else
            *error_num = NU_DNS_ERROR;
    }
#endif

    /* If the address was successfully resolved store the answer in the entry
     * to return to the user.
     */
    if (*error_num == NU_SUCCESS)
    {
        host_entry = DNS_Create_Host_Entry(name, family, (CHAR*)ip_addr,
                                           error_num);

        if (host_entry)
        {
            /* If an existing IPv4 or IPv6 record was found in the database, merge
             * the records.
             */
#if (INCLUDE_IPV6 == NU_TRUE)
            if ( (ipv4_host) || (ipv6_host) )
            {
                DNS_Merge_Records(*host_entry->h_addr_list, ipv6_host,
                                  ipv4_host, (UINT16)resolved_addrs, family);
            }
#else
            if (ipv4_host)
                DNS_Merge_Records(*host_entry->h_addr_list, NU_NULL,
                                  ipv4_host, (UINT16)resolved_addrs, family);
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

            /* Sort the destination addresses based on the rules for
             * Destination Address Selection defined in RFC 3484.
             */
            DNS6_Sort_Records(*host_entry->h_addr_list);
#endif
        }
    }

    return (host_entry);

} /* DNS_Find_Host_By_Name */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Merge_Records
*
*   DESCRIPTION
*
*       This function will merge the contents of a record existing in the
*       database with any records that were resolved via a DNS query.
*
*   INPUTS
*
*       *addr_buff              A pointer to the memory area containing the
*                               records the were resolved via a DNS query.
*       *ipv6_host              A pointer to the IPv6 record found in the
*                               existing database.
*       *ipv4_host              A pointer to the IPv4 record found in the
*                               existing database.
*       resolved_addrs          The number of addresses that were resolved
*                               via DNS queries.
*       family                  The family type of the final merged records.
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
STATIC VOID DNS_Merge_Records(CHAR *addr_buff, DNS_HOST *ipv6_host,
                              DNS_HOST *ipv4_host, UINT16 resolved_addrs,
                              INT16 family)
{
    UINT8       i, j, n;
    DNS_HOST    *l_host, *host_ptr;

    i = (UINT8)(resolved_addrs * MAX_ADDRESS_SIZE);

#if (INCLUDE_IPV4 != NU_TRUE)
    UNUSED_PARAMETER(ipv4_host);
    UNUSED_PARAMETER(family);
#endif

#if (INCLUDE_IPV6 != NU_TRUE)
    /* Remove compiler warnings */
    UNUSED_PARAMETER(ipv6_host);
    UNUSED_PARAMETER(family);
#endif

    /* Process the IPv4 records first. */
    host_ptr = ipv4_host;

    for (n = 0; n < 2; n ++)
    {
        /* Search the list for records that match this same question. */
        for (l_host = host_ptr; l_host; l_host = l_host->dns_next)
        {
            /* There could be multiple records that answer the same question.  Check if
             * this record matches the query.
             */
            if ( (l_host == host_ptr) ||
                 ((l_host->dns_h_length == host_ptr->dns_h_length) &&
                  (NU_STRICMP((const char *)l_host->dns_name, host_ptr->dns_name) == 0) &&
                  (l_host->dns_type != DNS_TYPE_PTR)) )
            {
                /* If this is a permanent entry, or if the ttl has not expired. */
                if ( (l_host->dns_flags & DNS_PERMANENT_ENTRY) ||
                     (INT32_CMP(l_host->dns_ttl, NU_Retrieve_Clock()) > 0) )
                {
                    /* While there are IPv4 addresses within the record, map them to
                     * IPv4-Mapped IPv6 addresses and copy them into the memory region.
                     */
                    for (j = 0; (resolved_addrs < DNS_MAX_IP_ADDRS);
                         j += MAX_ADDRESS_SIZE, i += (UINT8)MAX_ADDRESS_SIZE, resolved_addrs++)
                    {
                        if (memcmp(&l_host->dns_record_data[j], "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                                   MAX_ADDRESS_SIZE) != 0)
                        {
#if (INCLUDE_IPV6 == NU_TRUE)
                            /* If the user wants IPv4 records to be returned as IPv4-mapped IPv6
                             * addresses, convert the address.
                             */
                            if ( (host_ptr == ipv4_host) && (family == NU_FAMILY_IP6) )
                            {
                                IP6_Create_IPv4_Mapped_Addr((UINT8*)&addr_buff[i],
                                                            IP_ADDR((const unsigned char*)&l_host->dns_record_data[j]));
                            }
                            else
#endif
                                NU_BLOCK_COPY(&addr_buff[i], &l_host->dns_record_data[j], MAX_ADDRESS_SIZE);
                        }

                        else
                        {
                            break;
                        }
                    }
                }
            }
        }

        /* Process the IPv6 records. */
        host_ptr = ipv6_host;
    }

} /* DNS_Merge_Records */
