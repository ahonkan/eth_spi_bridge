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
*       dns_gnbn.c
*
*   DESCRIPTION
*
*       This file contains the implementation of
*       DNS_Find_Matching_Host_By_Name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DNS_Find_Matching_Host_By_Name
*       DNS_Find_Addr_In_Record
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

extern DNS_HOST_LIST        DNS_Hosts;

CHAR *DNS_Find_Addr_In_Record(CHAR *record, CHAR *addr, INT16 addr_len);

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Find_Matching_Host_By_Name
*
*   DESCRIPTION
*
*       This function searches the host list for a host matching the name
*       passed in.
*
*   INPUTS
*
*       *name                   The name of the host to find.
*       *data                   The data associated with the host or NU_NULL
*                               to ignore data.
*       type                    The type of record to find; DNS_TYPE_A,
*                               DNS_TYPE_AAAA, or DNS_TYPE_PTR.
*
*   OUTPUTS
*
*       DNS_HOST*               A pointer to the DNS host.
*       NU_NULL                 No matching entry exists.
*
******************************************************************************/
DNS_HOST *DNS_Find_Matching_Host_By_Name(const CHAR *name, CHAR *data,
                                         INT16 type)
{
    DNS_HOST        *l_host;

    for (l_host = DNS_Hosts.dns_head; l_host; l_host = l_host->dns_next)
    {
        /* Is this one we're looking for. */
        if ( (l_host->dns_type == type) &&
             (NU_STRICMP((const char *)l_host->dns_name, name) == 0) &&
             ((!data) ||
#if (INCLUDE_IPV4 == NU_TRUE)
              ((type == DNS_TYPE_A) &&
               (DNS_Find_Addr_In_Record(l_host->dns_record_data, data, IP_ADDR_LEN))) ||
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
              ((type == DNS_TYPE_AAAA) &&
               (DNS_Find_Addr_In_Record(l_host->dns_record_data, data, IP6_ADDR_LEN))) ||
#endif
              ((type == DNS_TYPE_TXT) &&
               (memcmp(l_host->dns_record_data, data, l_host->dns_h_length) == 0)) ||
              (NU_STRICMP((const char *)l_host->dns_record_data, data) == 0)) )
        {
#if (INCLUDE_MDNS == NU_TRUE)
            /* If this entry is marked for deletion, ignore it as if it doesn't
             * exist.
             */
            if ( (l_host->dns_flags & DNS_DELAY_RESPONSE) && (l_host->dns_ttl == 0) )
                continue;
#endif

            /* We found a match.  If this is a permanent entry, or if the ttl
             * has not expired then return it.
             */
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

    return (l_host);

} /* DNS_Find_Matching_Host_By_Name */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Find_Addr_In_Record
*
*   DESCRIPTION
*
*       This function searches a list of addresses in a local record to find
*       the target IP address.
*
*   INPUTS
*
*       *record                 The list of addresses to search.
*       *addr                   The target address to find.
*       addr_len                The length of the target address.
*
*   OUTPUTS
*
*       A pointer to the target address in the record or NU_NULL if the
*       address does not exist.
*
******************************************************************************/
CHAR *DNS_Find_Addr_In_Record(CHAR *record, CHAR *addr, INT16 addr_len)
{
    INT     i, ip_idx = 0;
    CHAR    *addr_ptr = NU_NULL;

    /* Search through the list of IP addresses. */
    for (i = 0; i < DNS_MAX_IP_ADDRS; i ++, ip_idx += MAX_ADDRESS_SIZE)
    {
        /* If this IP address matches. */
        if (memcmp(&record[ip_idx], addr, addr_len) == 0)
        {
            addr_ptr = &record[ip_idx];
            break;
        }
    }

    return (addr_ptr);

} /* DNS_Find_Addr_In_Record */

