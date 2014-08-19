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
* FILE NAME
*
*       sck_gnba.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Get_IP_Node_By_Addr.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Get_IP_Node_By_Addr
*       DNS_Find_Host_By_Addr
*       DNS_Find_Matching_Host_By_Addr
*
* DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

extern DNS_HOST_LIST        DNS_Hosts;

extern NU_HOSTENT   *DNS_Create_Host_Entry(CHAR *, INT16, const CHAR *, STATUS *);

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_IP_Node_By_Addr
*
*   DESCRIPTION
*
*       Given a host IP Address this function searches for the host in
*       the "hosts file".  A NU_HOSTENT data structure is allocated,
*       filled in with the appropriate information and a pointer to
*       the data structure returned to the application.
*
*       After the application has finished using the data structure
*       returned by this function, a call to NU_Free_Host_Entry must
*       be made to free the memory allocated by the DNS lookup.
*
*   INPUTS
*
*       *addr                   Pointer to the IP address of the host to
*                               find.
*       len                     The length of the address.  If family
*                               is NU_FAMILY_IP, this value must be 4.
*                               If family is NU_FAMILY_IP6, this value
*                               must be 16.
*       type                    The type of address.  Either NU_FAMILY_IP
*                               or NU_FAMILY_IP6.
*       *error_num              Status returned to the caller:
*
*                               NU_SUCCESS
*                               NU_NO_MEMORY        No memory so host
*                                                   was not added
*                               NU_DNS_ERROR        DNS could not resolve
*                               NU_NO_DNS_SERVER    No DNS server exists
*                               NU_INVALID_PARM     Type was not valid
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
NU_HOSTENT *NU_Get_IP_Node_By_Addr(CHAR *addr, INT len, INT type,
                                   STATUS *error_num)
{
    NU_HOSTENT      *host_entry = NU_NULL;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Verify the parameters are correct. */
    if ( (!addr) || (!error_num) || (
#if (INCLUDE_IPV4 == NU_TRUE)
         ((type == NU_FAMILY_IP) && (len != IP_ADDR_LEN))
#if (INCLUDE_IPV6 == NU_TRUE)
         ||
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
         ((type == NU_FAMILY_IP6) && (len != IP6_ADDR_LEN))
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

    /* Obtain the semaphore. */
    status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        host_entry = DNS_Find_Host_By_Addr(addr, (INT16)type, error_num);

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (host_entry);

} /* NU_Get_IP_Node_By_Addr */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Find_Host_By_Addr
*
*   DESCRIPTION
*
*       This function searches the "hosts" file for a host with a matching
*       addr.  If the host is not found DNS is used to resolve the host
*       address if possible.
*
*   INPUTS
*
*       *addr                   A pointer to the address of the host to find.
*       family                  The family of the host.
*       *error_num              A pointer to the status of the call:
*
*                               NU_SUCCESS          Success
*                               NU_NO_MEMORY        No memory
*                               NU_DNS_ERROR        DNS could not resolve
*                               NU_NO_DNS_SERVER    No DNS server exists
*                               NU_INVALID_PARM     Type invalid
*                               NU_INVALID_LABEL    Label invalid
*
*   OUTPUTS
*
*       DNS_HOST*               Host pointer
*       NU_NOT_A_HOST           It is not a host because ttl failed
*       NU_NO_MEMORY            No memory so host was not added
*       NU_DNS_ERROR            DNS could not resolve
*       NU_NO_DNS_SERVER        No DNS server exists
*       NU_INVALID_PARM         Type was not valid for DNS_Resolve
*       NU_INVALID_LABEL        Label was not correct
*
******************************************************************************/
NU_HOSTENT *DNS_Find_Host_By_Addr(CHAR *addr, INT16 family, STATUS *error_num)
{
    NU_HOSTENT      *host_entry = NU_NULL;
    DNS_HOST        *dns_host;

#if (INCLUDE_DNS == NU_TRUE)
    CHAR            name[255];
    UNSIGNED        ttl = 0;
    INT16           resolved_addrs = 0;
#endif


#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* RFC 2133 section 6.2 - If the family is NU_FAMILY_IP6 and the address
     * is either an IPv4-Mapped address or an IPv4-Compatible address, skip
     * over the first 12 bytes of the address and set the family to
     * NU_FAMILY_IP.  Do an IPv4 lookup on the address.
     */
    if ( (family == NU_FAMILY_IP6) &&
         ((IPV6_IS_ADDR_V4MAPPED(addr)) || (IPV6_IS_ADDR_V4COMPAT(addr))) )
    {
        addr += 12;
        family = NU_FAMILY_IP;
    }

#endif

    dns_host = DNS_Find_Matching_Host_By_Addr((UINT8*)addr, family);

    if (dns_host != NU_NULL)
        *error_num = NU_SUCCESS;

#if (INCLUDE_DNS == NU_TRUE)

    /* If the host was not found in the local cache or the ttl had expired, then
       try to resolve it through DNS. */
    else
    {
        *error_num = DNS_Resolve(name, addr, &ttl, DNS_TYPE_PTR, family,
                                 &resolved_addrs);

        if (*error_num == NU_SUCCESS)
        {
            /* The name was resolved. Add this host to the cache. The only reason
               this addition can fail is if there is no memory. */
            dns_host = DNS_Add_Host(name, addr, ttl, family, DNS_TYPE_PTR, 1, 0, NU_NULL);

            if (dns_host == NU_NULL)
                *error_num = NU_NO_MEMORY;
        }
    }

#else

    /* If the host was not found in the local cache or the ttl had expired, then
       just setting error to NU_DNS_ERROR as DNS is not enabled to resolve it. */
    else
    {
        *error_num = NU_DNS_ERROR;
    }

#endif /* INCLUDE_DNS == NU_TRUE */

    if (dns_host)
    {
        host_entry = DNS_Create_Host_Entry(dns_host->dns_name, family,
                                           (CHAR*)dns_host->dns_record_data,
                                           error_num);
    }

    return (host_entry);

} /* DNS_Find_Host_By_Addr */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Find_Matching_Host_By_Addr
*
*   DESCRIPTION
*
*       This function searches the host list for a host matching the IP
*       address passed in.
*
*   INPUTS
*
*       *addr                   A pointer to the address of the host to
*                               find.
*       family                  The family type of the host.
*
*   OUTPUTS
*
*       DNS_HOST*               A pointer to the DNS host.
*       NU_NULL                 A matching entry does not exist.
*
******************************************************************************/
DNS_HOST *DNS_Find_Matching_Host_By_Addr(const UINT8 *addr, INT16 family)
{
    DNS_HOST        *l_host;
    INT16           h_length;
    INT             i, j;
    UINT8           found = 0;

#if ( (INCLUDE_IPV6 == NU_FALSE) || (INCLUDE_IPV4 == NU_FALSE) )
    UNUSED_PARAMETER(family);
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        h_length = IP6_ADDR_LEN;
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        h_length = IP_ADDR_LEN;
#endif

    /* Search for a matching host. */
    l_host = DNS_Hosts.dns_head;

    while (l_host)
    {
        /* Only check this record's addresses if it is a PTR. */
        if (l_host->dns_type == DNS_TYPE_PTR)
        {
            /* Search through each IP address entry */
            for (i = 0, j = 0; i < DNS_MAX_IP_ADDRS; i++, j += MAX_ADDRESS_SIZE)
            {
                /* Is this one we're looking for. */
                if (memcmp((UINT8 *)&l_host->dns_record_data[j], addr,
                           (unsigned int)h_length) == 0)
                {
#if (INCLUDE_MDNS == NU_TRUE)
                    /* If this entry is marked for deletion, ignore it as if it doesn't
                     * exist.
                     */
                    if ( (l_host->dns_flags & DNS_DELAY_RESPONSE) && (l_host->dns_ttl == 0) )
                        continue;
#endif

                    found = 1;

                    /* We found a match.  If this is a permanent entry or if the ttl has
                       not expired then return it. Else it needs to be updated so break.
                       The DNS resolver will be invoked below. */
                    if ( (l_host->dns_flags & DNS_PERMANENT_ENTRY) ||
                         (INT32_CMP(l_host->dns_ttl, NU_Retrieve_Clock()) > 0) )
                        break;

                    else
                    {
                        l_host = NU_NULL;
                        break;
                    }
                }
            }

            if (found)
                break;
        }

        /* Get a pointer to the next entry */
        l_host = l_host->dns_next;
    }

    return (l_host);

} /* DNS_Find_Matching_Host_By_Addr */
