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
*       dns_res.c
*
*   DESCRIPTION
*
*       This file contains name resolution routines through a DNS
*       server.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       DNS_Build_Query
*       DNS_Build_Header
*       DNS_Insert_Record
*       DNS_Query
*       DNS_Pack_Domain_Name
*       DNS_Unpack_Domain_Name
*       DNS_Extract_Data
*       DNS_Resolve
*       DNS_Add_Host
*       DNS_Delete_Host
*       DNS_Update_Host
*       DNS_Remove_DNS_Server
*       DNS_Find_Host_By_ID
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_extr.h
*       dns4.h
*       dns6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#include "networking/dns4.h"
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/dns6.h"
#endif

/* This is a list of DNS servers or resolvers. */
extern DNS_SERVER_LIST      DNS_Servers;
#if (INCLUDE_MDNS == NU_TRUE)
extern DNS_SERVER_LIST      MDNS_Servers;
extern NU_TIMER             MDNS_Response_Timer;
#endif

/* This is the list of hosts.  If a host can not be found in this list,
   DNS will be used to retrieve the information.  The new host will be
   added to the list. */
extern DNS_HOST_LIST    DNS_Hosts;

extern UNSIGNED         DNS_Id;

/* Function Prototypes. */
STATIC  INT DNS_Pack_Domain_Name (CHAR *dst, const CHAR *src, UINT32 dst_len);

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for DNS Resolve packets */
CHAR        NET_DNS_Resolve_Memory[DNS_MAX_MESSAGE_SIZE * NET_MAX_DNS_PACKETS];

/* Declare a memory flag array for the memory declared above */
UINT8       NET_DNS_Resolve_Memory_Flags[NET_MAX_DNS_PACKETS]={0};

/* Declare memory for the DNS hosts additions and their name */
DNS_HOST    NET_DNS_Add_Memory[NET_MAX_DNS_HOSTS];
CHAR        NET_DNS_Name_Memory[DNS_MAX_NAME_SIZE * NET_MAX_DNS_HOSTS];

/* Counter to count the DNS hosts added*/
INT         No_of_RES_Hosts = 0;
#endif

/*****************************************************************************
*
*   FUNCTION
*
*       DNS_Resolve
*
*   DESCRIPTION
*
*       This routine will handle domain based name lookup.  It can handle
*       both queries for names and queries for IP addresses.  Either the
*       name and ttl are returned or an IP address and ttl are returned.
*
*   INPUTS
*
*       *name                   Pointer to a host name.
*       *ip_addr                Pointer to the host IP address, filled in
*                               by this function.
*       *ttl                    Pointer to the Time To Live, length of time
*                               the host info can be cached.  Filled in here.
*       type                    Type of query, name or address.
*       family                  Family of the server to query.
*       *resolved_addrs         Pointer to the number of addresses resolved.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_NO_MEMORY            No memory so host was not added
*       NU_DNS_ERROR            DNS could not resolve
*       NU_NO_DNS_SERVER        No DNS server exists
*       NU_INVALID_PARM         Type was not valid
*       NU_INVALID_LABEL        Label was not correct
*
******************************************************************************/
STATUS DNS_Resolve(CHAR *name, CHAR *ip_addr, UNSIGNED *ttl, UINT16 type,
                   INT16 family, INT16 *resolved_addrs)
{
    STATUS                  stat = NU_DNS_ERROR;
    CHAR                    *buffer = NU_NULL;
    INT                     q_size;
    DNS_SERVER              *cur_server;
    struct addr_struct      dns_addr;

    /* Fill in the address structure with the family and name. */
    dns_addr.name = "DNSclnt";

#if (INCLUDE_MDNS == NU_TRUE)
    /* Any time the name being queried for falls within one of the reserved
     * mDNS domains ... rather than using the configured unicast DNS server
     * address, the query is instead sent to 224.0.0.251:5353 (or its IPv6
     * equivalent [FF02::FB]: 5353).
     * ...
     * DNS queries for names that do not end with ".local." MAY be sent to
     * the mDNS multicast address, if no other conventional DNS server is
     * available.
     */
    if ( ((
#if (INCLUDE_IPV6 == NU_TRUE)
           (type == DNS_TYPE_AAAA)
#if (INCLUDE_IPV4 == NU_TRUE)
           ||
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
           (type == DNS_TYPE_A)
#endif
           ) &&
          (NU_STRICMP(&name[strlen(name) - 6], ".local") == 0)) ||
         ((type == DNS_TYPE_PTR) &&
          (
#if (INCLUDE_IPV6 == NU_TRUE)
           ((family == NU_FAMILY_IP6) && (IPV6_IS_ADDR_LINKLOCAL(ip_addr)))
#if (INCLUDE_IPV4 == NU_TRUE)
           ||
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
           ((family == NU_FAMILY_IP) && IP_LL_ADDR(ip_addr))
#endif
           )) || (DNS_Servers.dnss_head == NU_NULL) )
    {
        dns_addr.port = MDNS_PORT;

        /* Get a pointer to the first server in the list. */
        cur_server = MDNS_Servers.dnss_head;
    }

    else
#else
    UNUSED_PARAMETER(name);
#endif
    {
        dns_addr.port = DNS_PORT;

        /* Get a pointer to the first server in the list. */
        cur_server = DNS_Servers.dnss_head;

        /* If there are no servers in the list, return an error. */
        if ( (cur_server) &&
             (memcmp(cur_server->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                     MAX_ADDRESS_SIZE) == 0) )
        {
            cur_server = NU_NULL;
        }
    }

    /* While there are servers in the list to query */
    while (cur_server)
    {
        /* If the current server's IP address is NULL, it is the last
         * server in the list.  Break out of the loop.
         */
        if (memcmp(cur_server->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                   MAX_ADDRESS_SIZE) == 0)
        {
            stat = NU_DNS_ERROR;
            break;
        }

        /* Build the DNS query. */
        switch (type)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            case DNS_TYPE_A :
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
            case DNS_TYPE_AAAA :
#endif

                q_size = DNS_Build_Header((VOID **)&buffer, 1, DNS_RD);

                if (q_size >= 0)
                {
                    q_size = DNS_FIXED_HDR_LEN;

                    /* Query built successfully */
                    stat = NU_SUCCESS;

                    /* Add the record to the packet. */
                    q_size += DNS_Build_Query(name, 0, &buffer[q_size],
                                              type, family, DNS_CLASS_IN,
                                              DNS_MAX_MESSAGE_SIZE - q_size);

                    /* There is one query in the packet. */
                    PUT16(buffer, DNS_QDCOUNT_OFFSET, 1);
                }
                else
                    /* Invalid query */
                    stat = q_size;

                break;

            case DNS_TYPE_PTR :

                q_size = DNS_Build_Header((VOID **)&buffer, 1, DNS_RD);

                if (q_size >= 0)
                {
                    q_size = DNS_FIXED_HDR_LEN;

                    /* Query built successfully */
                    stat = NU_SUCCESS;

                    /* Add the record to the packet. */
                    q_size += DNS_Build_Query(ip_addr, 0, &buffer[q_size],
                                              type, family, DNS_CLASS_IN,
                                              DNS_MAX_MESSAGE_SIZE - q_size);

                    /* There is one query in the packet. */
                    PUT16(buffer, DNS_QDCOUNT_OFFSET, 1);
                }
                else
                    /* Invalid query */
                    stat = q_size;

                break;

            default:

                stat = NU_INVALID_PARM;
        }

        /* Check to the see if the DNS Query was successfully built */
        if (stat == NU_SUCCESS)
        {
            /* Set the family and address of the destination. */
            dns_addr.family = cur_server->family;
            memcpy(dns_addr.id.is_ip_addrs, cur_server->dnss_ip, MAX_ADDRESS_SIZE);

            /* Query the DNS server. Upon returning the buffer will contain the
             * response to our query.
             */
            if (DNS_Query(buffer, q_size, &dns_addr) > 0)
            {
                /* Now process the response. */
                switch (type)
                {

#if (INCLUDE_IPV4 == NU_TRUE)
                    case DNS_TYPE_A :
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                    case DNS_TYPE_AAAA :
#endif
                        stat = DNS_Extract_Data((DNS_PKT_HEADER *)buffer, ip_addr, NU_NULL, ttl,
                                                 type, family, resolved_addrs);
                        break;

                    case DNS_TYPE_PTR :

                        stat = DNS_Extract_Data((DNS_PKT_HEADER *)buffer, name, NU_NULL, ttl,
                                                type, family, resolved_addrs);
                        break;

                    default:

                        stat = NU_INVALID_PARM;

                        break;
                }

                /* Break out if a valid DNS record has been located */
                if (stat == NU_SUCCESS)
                    break;
            }

            /* The server did not respond - set an error in case this
             * was the last server in the list.
             */
            else
                stat = NU_DNS_ERROR;

        }

        else
        {
            /* Failed to build DNS Query */
            break;
        }

        cur_server = cur_server->dnss_next;
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Deallocate the memory buffer. */
    if (buffer)
    {
        if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for buffer", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

#else

    /* Turn the memory flag off to indicate an unused memory */
    NET_DNS_Resolve_Memory_Flags[(UINT8)(((CHAR *)buffer -
                                        (CHAR *)NET_DNS_Resolve_Memory)/
                                        DNS_MAX_MESSAGE_SIZE)] = NU_FALSE;

#endif

    if (stat != NU_SUCCESS)
        return (NU_DNS_ERROR);
    else
        return (stat);

} /* DNS_Resolve */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Build_Header
*
*   DESCRIPTION
*
*       This function will build a DNS query header.
*
*   INPUTS
*
*       **buffer                A pointer to a buffer pointer.
*       id                      The ID to place in the header.
*       flags                   Flags to set in the header.
*
*   OUTPUTS
*
*       > 0                     The size of the packed name.
*       NU_NO_MEMORY            No memory so host was not added
*
******************************************************************************/
INT DNS_Build_Header(VOID **buffer, INT16 id, UINT16 flags)
{
    DNS_PKT_HEADER      *dns_pkt;

    /* Allocate a block of memory to build the query packet in. */
    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&dns_pkt,
                           DNS_MAX_MESSAGE_SIZE, NU_NO_SUSPEND) != NU_SUCCESS)
    {
        return (NU_NO_MEMORY);
    }

    /* Setup the packet. */
    PUT16(dns_pkt, DNS_ID_OFFSET, id);
    PUT16(dns_pkt, DNS_FLAGS_OFFSET, flags); /* Set the Recursion desired bit. */
    PUT16(dns_pkt, DNS_QDCOUNT_OFFSET, 0);
    PUT16(dns_pkt, DNS_ANCOUNT_OFFSET, 0);
    PUT16(dns_pkt, DNS_NSCOUNT_OFFSET, 0);
    PUT16(dns_pkt, DNS_ARCOUNT_OFFSET, 0);

    /* Return a pointer to the DNS packet. */
    *buffer = dns_pkt;

    return (DNS_MAX_MESSAGE_SIZE);

} /* DNS_Build_Header */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Build_Query
*
*   DESCRIPTION
*
*       This function will add a DNS query to a packet.
*
*   INPUTS
*
*       *data                   Pointer to a buffer containing data used
*                               for the query.
*       offset                  If data is null, this is the value that should
*                               be placed in the packet instead of the data.
*       *buffer                 A pointer to a buffer.
*       type                    A query type.
*       family                  The family type of the server to query.
*       dest_len                Length of the destination buffer.
*
*   OUTPUTS
*
*       > 0                     The size of the packed name.
*       NU_INVALID_PARM         Type was not valid
*       NU_INVALID_LABEL        Label was not correct
*
******************************************************************************/
INT DNS_Build_Query(CHAR *data, INT16 offset, CHAR *buffer, UINT16 type,
                    INT16 family, UINT16 flags, UINT32 dest_len)
{
    DNS_RR              *rr_ptr;
    INT                 name_size = 0;
    CHAR                name[80];

    /* Pack the domain name, i.e., put it in a format the server will
       understand.  The packed domain name will be copied into the location
       pointed to by q_ptr. */
    switch (type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        case DNS_TYPE_A :
#endif

        case DNS_TYPE_MX :
        case DNS_TYPE_CNAME :
        case DNS_TYPE_ANY :

#if (INCLUDE_IPV6 == NU_TRUE)
        case DNS_TYPE_AAAA :
#endif

            /* Add the name to the query.  The only data in the packet is
             * the DNS header at this time.
             */
            if (data)
            {
                name_size = DNS_Pack_Domain_Name(buffer, data, dest_len);
            }

            /* Ensure there is two bytes room for the pointer. */
            else if (dest_len >= 2)
            {
                /* The caller is using compression - add the offset to the packet. */
                PUT16(buffer, 0, 0xc000 | offset);
                name_size = 2;
            }

            else
            {
                name_size = NU_NO_MEMORY;
            }

            break;

        case DNS_TYPE_PTR :
        case DNS_TYPE_SRV :
        case DNS_TYPE_TXT :

            if (family == NU_FAMILY_UNSPEC)
            {
                name_size = DNS_Pack_Domain_Name(buffer, data, dest_len);
            }

            else
            {
                /* If we have a IP address and trying to get a name, convert the address to
                   a character string. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
                if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                    DNS6_Addr_To_String(data, name);

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                    DNS4_Addr_To_String(data, name);
#endif

                /* Add the name to the query.  The only data in the packet is
                 * the DNS header at this time.
                 */
                name_size = DNS_Pack_Domain_Name(buffer, name,
                                                 DNS_MAX_MESSAGE_SIZE - DNS_FIXED_HDR_LEN);
            }

            break;

        default :

            break;
    }

    if ( (name_size > 0) && ((name_size + 4) < dest_len) )
    {
        /* Move the pointer past the end of the name. */
        rr_ptr = (DNS_RR *)(buffer + name_size);

        /* Load the type and class fields of the query structure. */
        PUT16(rr_ptr, DNS_TYPE_OFFSET, type);
        PUT16(rr_ptr, DNS_CLASS_OFFSET, flags);

        /* The ttl and rdlength are not needed for a query. Set them to zero. */
        PUT32(rr_ptr, DNS_TTL_OFFSET, 0);
        PUT16(rr_ptr, DNS_RDLENGTH_OFFSET, 0);

        name_size += 4;
    }

    else
        name_size = NU_NO_MEMORY;

    return (name_size);

} /* DNS_Build_Query */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Insert_Record
*
*   DESCRIPTION
*
*       This function will insert a DNS resource record into the packet.
*
*   INPUTS
*
*       *dns_ptr                Pointer to the record to transmit.
*       *host_name              Pointer to the host name of the record.
*       offset                  Offset into the packet to insert the record.
*       *buffer                 Pointer to the buffer into which to insert the
*                               record.
*       type                    The type of record to insert.
*       r_class                 The class of the record.
*       *data                   Pointer to the data to insert in the packet.
*       dest_len                The length of the destination buffer.
*
*   OUTPUTS
*
*       The length of the data added to the packet.
*       NU_NO_MEMORY            The data will not fit in the packet.
*
******************************************************************************/
INT DNS_Insert_Record(DNS_HOST *dns_ptr, CHAR *host_name, INT16 offset, CHAR *buffer,
                      UINT16 type, UINT16 r_class, CHAR *data, UINT32 dest_len)
{
    INT     q_size = 0, name_len;
    CHAR    name[80];

    /* For reverse-address mapped PTR records, the host_name is inserted at
     * the end of the packet, and the data at the beginning.
     */
    if ( (type == DNS_TYPE_PTR) && (dns_ptr->dns_family != NU_FAMILY_UNSPEC) )
    {
        /* Build the reverse address for the record. */
#if (INCLUDE_IPV6 == NU_TRUE)
        if (dns_ptr->dns_family == NU_FAMILY_IP6)
        {
            DNS6_Addr_To_String(data, name);
        }
#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        {
            DNS4_Addr_To_String(data, name);
        }
#endif

        /* Add the reverse address to the packet. */
        q_size = DNS_Pack_Domain_Name(buffer, name, dest_len);
    }

    else
    {
        if (host_name)
        {
            /* Add the name to the packet. */
            q_size = DNS_Pack_Domain_Name(buffer, host_name, dest_len);
        }

        /* If compression is being used. */
        else if (dest_len >= 2)
        {
            PUT16(buffer, 0, 0xc000 | offset);
            q_size = 2;
        }
    }

    /* If the type, class and ttl will fit in the buffer. */
    if ( (q_size > 0) && (q_size + 8 <= dest_len) )
    {
        /* Load the type and class fields of the query structure. */

        /* If the NSEC flag is set then we need to use that type. */
        if(dns_ptr->dns_flags & DNS_NSEC_RECORD)
        {
            PUT16(&buffer[q_size], DNS_TYPE_OFFSET, DNS_TYPE_NSEC);
        }
        else
        {
            PUT16(&buffer[q_size], DNS_TYPE_OFFSET, type);
        }

        PUT16(&buffer[q_size], DNS_CLASS_OFFSET, r_class);

        /* If this is an NSEC, the TTL must be 120 seconds. */
        if (dns_ptr->dns_flags & DNS_NSEC_RECORD)
        {
            PUT32(&buffer[q_size], DNS_TTL_OFFSET, DNS_NSEC_TTL);
        }

        /* Compute the remaining TTL and add it to the packet. */
        else if (!(dns_ptr->dns_flags & DNS_PERMANENT_ENTRY))
        {
            PUT32(&buffer[q_size], DNS_TTL_OFFSET,
                  (TQ_Check_Duetime(dns_ptr->dns_ttl) / SCK_Ticks_Per_Second));
        }

        /* Do not check the due time for a permanent entry. */
        else
        {
            PUT32(&buffer[q_size], DNS_TTL_OFFSET, dns_ptr->dns_ttl / SCK_Ticks_Per_Second);
        }

        /* If this is a SRV record - SRV records have three additional fields;
         * weight, priority and port.
         */
        if (type == DNS_TYPE_SRV)
        {
            /* Ensure the length, priority, weight and port will fit in the buffer. */
            if ((q_size + 16) <= dest_len)
            {
                /* Add the additional fields. */
                PUT16(&buffer[q_size], DNS_SRV_PRIO_OFFSET, dns_ptr->dns_prio);
                PUT16(&buffer[q_size], DNS_SRV_WEIGHT_OFFSET, dns_ptr->dns_weight);
                PUT16(&buffer[q_size], DNS_SRV_PORT_OFFSET, dns_ptr->dns_port);

                /* Add the data to the packet. */
                name_len = DNS_Pack_Domain_Name(&buffer[q_size + DNS_SRV_DATA_OFFSET],
                                                data, dest_len - (q_size + DNS_SRV_DATA_OFFSET));

                if (name_len > 0)
                {
                    /* Add the data length - this includes the 6 bytes for priority, weight
                     * and port.
                     */
                    PUT16(&buffer[q_size], DNS_RDLENGTH_OFFSET, (name_len + 6));

                    q_size += (DNS_SRV_DATA_OFFSET + name_len);
                }

                else
                {
                    q_size = NU_NO_MEMORY;
                }
            }

            else
            {
                q_size = NU_NO_MEMORY;
            }
        }

        else
        {
            /* Add bytes for type, class and ttl. */
            q_size += 8;

            /* If this is a reverse-address mapped PTR record. */
            if ( (type == DNS_TYPE_PTR) && (dns_ptr->dns_family != NU_FAMILY_UNSPEC) )
            {
                if (host_name)
                {
                    /* Add the name to the end of the packet. */
                    name_len = DNS_Pack_Domain_Name(&buffer[q_size + 2], host_name,
                                                    (dest_len - (q_size + 2)));

                    /* If the name fits in the buffer. */
                    if (name_len > 0)
                    {
                        /* Add the data length to the packet.  Do not check that this
                         * data will fit as the above call included this data.
                         */
                        PUT16(&buffer[q_size], 0, name_len);

                        q_size += (2 + name_len);
                    }

                    else
                    {
                        q_size = NU_NO_MEMORY;
                    }
                }

                /* If compression is being used. */
                else
                {
                    /* If the data will fit in the buffer. */
                    if ( (q_size + 4) <= dest_len)
                    {
                        PUT16(&buffer[q_size + 2], 0, 0xc000 | offset);

                        /* Add the data length to the packet. */
                        PUT16(&buffer[q_size], 0, 2);

                        q_size += 4;
                    }

                    else
                    {
                        q_size = NU_NO_MEMORY;
                    }
                }
            }

            else
            {
                /* If this is an NSEC record.  host_name will never be NULL if this is
                 * an NSEC record, but this check is required to remove a warning.
                 */
                if ( (dns_ptr->dns_flags & DNS_NSEC_RECORD) && (host_name) )
                {
                    /* Add the domain name to the next domain name field. */
                    name_len = DNS_Pack_Domain_Name(&buffer[q_size + 2], host_name,
                                                    (dest_len - (q_size + 2)));

                    if (name_len > 0)
                    {
                        /* Set the data length size. */
                        if (dns_ptr->dns_type == DNS_TYPE_AAAA)
                        {
                            PUT16(&buffer[q_size], 0, name_len + 3);
                        }

                        else
                        {
                            PUT16(&buffer[q_size], 0, name_len + 6);
                        }

                        /* Move the buffer pointer past our data length and the
                         * next domain section.
                         */
                        q_size += (2 + name_len);

                        /* Set the window block number. */
                        PUT8(&buffer[q_size], 0, 0);

                        /* We only support NSEC for A and AAAA records.  If the
                         * foreign side is asking for the AAAA, but we only found
                         * the A, NSEC the AAAA record.
                         */
                        if (dns_ptr->dns_type == DNS_TYPE_A)
                        {
                            /* Add the length of the bitmap. */
                            PUT8(&buffer[q_size + 1], 0, 4);

                            /* Add the bitmap. */
                            PUT32(&buffer[q_size + 2], 0, DNS_TYPE_AAAA_BITMAP);

                            q_size += 6;
                        }

                        /* NSEC the A record. */
                        else
                        {
                            PUT8(&buffer[q_size+1], 0, 1);
                            PUT8(&buffer[q_size+2], 0, DNS_TYPE_A_BITMAP);

                            q_size += 3;
                        }
                    }

                    else
                    {
                        q_size = NU_NO_MEMORY;
                    }
                }

                else
                {
                    /* If the data does not have to be encoded before being inserted
                     * into the packet.
                     */
                    if ( (dns_ptr->dns_family != NU_FAMILY_UNSPEC) ||
                         (dns_ptr->dns_type == DNS_TYPE_TXT) )
                    {
                        /* If the data will fit in the buffer. */
                        if ( (q_size + dns_ptr->dns_h_length + 2) <= dest_len)
                        {
                            /* Add the data. */
                            PUT16(&buffer[q_size], 0, dns_ptr->dns_h_length);
                            memcpy(&buffer[q_size + 2], data, dns_ptr->dns_h_length);

                            q_size += (2 + dns_ptr->dns_h_length);
                        }

                        else
                        {
                            q_size = NU_NO_MEMORY;
                        }
                    }

                    /* The record contains data that must be encoded before inserting
                     * into the record.
                     */
                    else
                    {
                        /* Add the data to the end of the packet. */
                        name_len = DNS_Pack_Domain_Name(&buffer[q_size + 2], data,
                                                        (dest_len - (q_size + 2)));

                        if (name_len > 0)
                        {
                            /* Set the data length. */
                            PUT16(&buffer[q_size], 0, name_len);

                            q_size += (2 + name_len);
                        }

                        else
                        {
                            q_size = NU_NO_MEMORY;
                        }
                    }
                }
            }
        }
    }

    else
    {
        q_size = NU_NO_MEMORY;
    }

    return (q_size);

} /* DNS_Insert_Record */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Query
*
*   DESCRIPTION
*
*       This function queries a DNS server.  It sends a DNS query packet and
*       waits for the response.
*
*   INPUTS
*
*       *buffer                 A pointer to a buffer containing a query pkt.
*       q_size                  The size of the query.
*       *dns_addr               The destination of the packet.
*
*   OUTPUTS
*
*       > 0                     The size of the received response.
*       NU_FAILED_QUERY         General failure.
*       NU_INVALID_PROTOCOL     Invalid protocol for the NU_Sockets
*       NU_NO_SOCK_MEMORY       No memory to allocate
*
******************************************************************************/
INT DNS_Query(CHAR *buffer, INT q_size, struct addr_struct *dns_addr)
{
    FD_SET                  readfs;
    STATUS                  stat;
    INT                     attempts = DNS_MAX_ATTEMPTS;
    STATUS                  r_size = 0;
    INT32                   bytes_sent;
    INT                     socketd;

    socketd = NU_Socket(dns_addr->family, NU_TYPE_DGRAM, 0);

    if (socketd < 0)
        return (socketd);

    /* Initially all the bits should be cleared. */
    NU_FD_Init(&readfs);

    while (attempts)
    {
        /* Send the DNS query. */
        bytes_sent = NU_Send_To(socketd, buffer, (UINT16)q_size, 0,
                                dns_addr, 0);

        /*  If the data was not sent, we have a problem. */
        if (bytes_sent < 0)
            break;

        /* Decrement the number of attempts left. */
        attempts--;

        /* Specify which socket we want to select on. */
        NU_FD_Set(socketd, &readfs);

        /* Select on the specified socket for one second. */
        stat = NU_Select(NSOCKETS, &readfs, NU_NULL, NU_NULL,
                         SCK_Ticks_Per_Second);

        /* If there is no data on the socket, either send the query again or
           give up. */
        if (stat != NU_SUCCESS)
            continue;

        /*  Go get the server's response.  */
        r_size = (INT)NU_Recv_From(socketd, buffer, DNS_MAX_MESSAGE_SIZE, 0,
                                   dns_addr, 0);

        break;
    }

    /* Close the socket. */
    if (NU_Close_Socket(socketd) != NU_SUCCESS)
        NLOG_Error_Log("Failed to close socket", NERR_SEVERE, __FILE__,
                       __LINE__);
    if (r_size > 0)
        return (r_size);
    else
        return (NU_FAILED_QUERY);

} /* DNS_Query */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Pack_Domain_Name
*
*   DESCRIPTION
*
*       This function takes a domain name and converts it to a format a DNS
*       server expects.
*
*   INPUTS
*
*       *dst                    A pointer to the converted name.
*       *src                    A pointer to the original name.
*
*   OUTPUTS
*
*       > 0                     The size of the new name.
*       NU_INVALID_PARM         src is not valid
*       NU_INVALID_LABEL        Label size failed
*
******************************************************************************/
STATIC INT DNS_Pack_Domain_Name(CHAR *dst, const CHAR *src, UINT32 dst_len)
{
    CHAR        *p;
    CHAR        *original_dest;
    INT         n, dest_offset = 0;
    UINT8       temp;

    /* If this is a null string return an error. */
    if (!*src)
        return (NU_INVALID_PARM);

    p = original_dest = dst;

    do
    {
        /* Move past the byte where the length will be saved. */
        dest_offset ++;

        if (dest_offset >= dst_len)
            return (NU_NO_MEMORY);

        /* Assume all labels have been copied until proven otherwise. */
        *p = 0;

        /* Copy the label. */
        for (n = 0;
             *src && (*src != '.') && (n <= DNS_MAX_LABEL_SIZE) &&
             (dest_offset < dst_len);
             dst[dest_offset] = *src++, dest_offset ++)
        {
            ++n;
        }

        /* Check to see if the label exceeded the maximum length. */
        if (n > DNS_MAX_LABEL_SIZE)
            return (NU_INVALID_LABEL);

        else if (dest_offset >= dst_len)
            return (NU_NO_MEMORY);

        /* Store the length of the label. */
        temp = (UINT8)(&dst[dest_offset] - p);
        *p   = (UINT8)(temp - 1);

        /* Point to where the next length value will be stored. */
        p = &dst[dest_offset];

        if (*src)
            src++;

    } while (*src);

    /* The end of the name is marked with a 0 length label. */
    *p = 0;

    /* Add one more byte so we can calculate the length of the string. */
    dest_offset ++;

    if (dest_offset < dst_len)
        return (INT)(&dst[dest_offset] - original_dest);
    else
        return (NU_NO_MEMORY);

} /* DNS_Pack_Domain_Name */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Unpack_Domain_Name
*
*   DESCRIPTION
*
*       This function packed name and converts it to a character string.
*
*   INPUTS
*
*       *dst                    A pointer to the new name or NU_NULL if the
*                               user just wants to learn the length.
*       dest_length             The size of the destination buffer.
*       *src                    A pointer to the original name.
*       *buf_begin              A pointer to the DNS header.
*
*   OUTPUTS
*
*       The size of the new name.
*
******************************************************************************/
INT DNS_Unpack_Domain_Name(CHAR *dst, INT dest_length, CHAR *src,
                           CHAR *buf_begin)
{
    INT16           size;
    INT             i, j = 0, retval = 0;
    CHAR            *savesrc;

    savesrc = src;

    /* The end of the name is marked by a 0 length label. */
    while ( (*src) && (j < dest_length) )
    {
        /* Get the size of the label. */
        size = *src;

        /* Check to see if this is a pointer instead of a label size. */
        while ((size & 0xC0) == 0xC0)
        {
            /* If we have not encountered a pointer yet compute the size of
             * the name so far.
             */
            if (!retval)
            {
                retval = (INT)((src - savesrc) + 2);
            }

            src++;

            /* Point to the new location. */
            src = &buf_begin[(size & 0x3f) * 256 + *src];
            size = *src;
        }

        /* Move the pointer past the label size. */
        src++;

        /* Copy the label */
        for (i = 0; (i < (size & 0x3f)) && (j < dest_length); i++, j++)
        {
            if (dst)
                *dst++ = *src;

            src++;
        }

        /* Insert the period between labels. */
        if (j < dest_length)
        {
            if (dst)
                *dst++ = '.';

            j++;
        }
    }

    /* Add the terminator. */
    if (dst)
        *(--dst) = 0;

    /* Account for the terminator on src. */
    src++;

    /* If the name included a pointer then the return value has already been
       computed. */
    if (!retval)
    {
        retval = (INT)(src - savesrc);
    }

    return (retval);

}  /* DNS_Unpack_Domain_Name */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Extract_Data
*
*   DESCRIPTION
*
*       This function takes a DNS response and extracts either an IP address
*       or a domain name, which ever the case may be.
*
*   INPUTS
*
*       *pkt                    A pointer to the DNS response.
*       *data                   A pointer to the buffer into which to
*                               put the address or name.
*       *question               The question being answered.
*       *ttl                    A pointer to the buffer into which to
*                               put the Time To Live.
*       type                    The type of query.
*       family                  The family of the DNS Server.
*       *resolved_addrs         The number of addresses resolved.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates success.
*       NU_DNS_ERROR            rcode is NU_NULL
*       NU_MEM_ALLOC            No memory to allocate
*
******************************************************************************/
STATUS DNS_Extract_Data(DNS_PKT_HEADER *pkt, CHAR *data, CHAR *question,
                        UNSIGNED *ttl, UINT16 type, INT16 family,
                        INT16 *resolved_addrs)
{
    DNS_RR          *rr_ptr;
    INT             name_size, n_answers, rcode, i;
    UINT16          length;
    CHAR            *p_ptr, *name;
    STATUS          answer_received = NU_DNS_ERROR;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Declare memory for the dns name */
    CHAR            dns_name_memory[DNS_MAX_NAME_SIZE];
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    UINT32          ipv4_addr;
#endif

#if ( (INCLUDE_IPV6 != NU_TRUE) || (INCLUDE_IPV4 != NU_TRUE) )
    /* Remove compiler warnings */
    UNUSED_PARAMETER(family);
#endif

    /* Get the number of answers in this message. */
    n_answers = GET16(pkt, DNS_ANCOUNT_OFFSET) + GET16(pkt, DNS_ARCOUNT_OFFSET);

    /* Extract the return code. */
    rcode = DNS_RCODE_MASK & GET16(pkt, DNS_FLAGS_OFFSET);

    /* Was an error returned? */
    if (rcode)
        return (NU_DNS_ERROR);

    /* Initialize i so that new addresses are added after existing addresses
     * in the memory area provided.
     */
    i = (*resolved_addrs) * MAX_ADDRESS_SIZE;

    /* If there is at least one answer and this is a response, process it. */
    if ((n_answers > 0) && (GET16(pkt, DNS_FLAGS_OFFSET) & DNS_QR))
    {
        /* Point to where the question/answer section starts. */
        p_ptr = (CHAR *)(pkt + 1);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Allocate a block of memory to put the name in. */
        if (NU_Allocate_Memory(MEM_Cached, (VOID **)&name,
                               DNS_MAX_NAME_SIZE,
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            return (NU_NO_MEMORY);
        }
#else
        /* Assign memory to the name */
        name = dns_name_memory;
#endif

        /* If there is a question in the packet. */
        if (GET16(pkt, DNS_QDCOUNT_OFFSET) > 0)
        {
            /* Extract the name. */
            name_size = DNS_Unpack_Domain_Name(name, DNS_MAX_NAME_SIZE, p_ptr,
                                               (CHAR *)pkt);

            /*  Move the pointer past the name QTYPE and QCLASS to point at the
                answer section of the response. */
            p_ptr += name_size + 4;
        }

        /*
        *  At this point, there may be several answers.  We will take the first
        *  one which has an IP number.  There may be other types of answers that
        *  we want to support later.
        */
        while ((n_answers--) > 0)
        {
            /* Extract the name from the answer. */
            name_size = DNS_Unpack_Domain_Name(name, DNS_MAX_NAME_SIZE, p_ptr,
                                               (CHAR *)pkt);

            /* Move the pointer past the name. */
            p_ptr += name_size;

            /* Point to the resource record. */
            rr_ptr = (DNS_RR *)p_ptr;

            /* Verify the question, type and class match. */
            if ( ((GET16(p_ptr, DNS_TYPE_OFFSET) == DNS_TYPE_PTR) ||
                  (question == NU_NULL) || (NU_STRICMP(question, name) == 0)) &&
                 (GET16(p_ptr, DNS_TYPE_OFFSET) == type) &&
                 (GET16(p_ptr, DNS_CLASS_OFFSET) & DNS_CLASS_IN))
            {
                switch (type)
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    case DNS_TYPE_A :

                        if (*resolved_addrs < DNS_MAX_IP_ADDRS)
                        {
#if (INCLUDE_IPV6 == NU_TRUE)
                            if (family == NU_FAMILY_IP)
#endif
                                GET_STRING (rr_ptr, DNS_RDATA_OFFSET, &data[i],
                                            IP_ADDR_LEN);

#if (INCLUDE_IPV6 == NU_TRUE)

                            /* If the family type is NU_FAMILY_IP6 and the record
                             * type is A, the user wants the address returned as
                             * an IPv4-Mapped IPv6 address.
                             */
                            else
                            {
                                /* Extract the IPv4 address from the packet */
                                GET_STRING(rr_ptr, DNS_RDATA_OFFSET, (CHAR*)&ipv4_addr,
                                           IP_ADDR_LEN);

                                /* Create the IPv4-Mapped IPv6 address and store
                                 * it in the memory area to return to the user.
                                 */
                                IP6_Create_IPv4_Mapped_Addr((UINT8*)(&data[i]),
                                                            LONGSWAP(ipv4_addr));
                            }
#endif

                            i += MAX_ADDRESS_SIZE;

                            (*resolved_addrs)++;
                        }

                        break;
#endif

                    case DNS_TYPE_PTR :

                        /* The answer has the correct type and class. Get the name. */
                        DNS_Unpack_Domain_Name(data, DNS_MAX_NAME_SIZE,
                                               rr_ptr->dns_rdata, (CHAR *)pkt);
                        break;

#if (INCLUDE_IPV6 == NU_TRUE)

                    case DNS_TYPE_AAAA :

                        if (*resolved_addrs < DNS_MAX_IP_ADDRS)
                        {
                            GET_STRING (rr_ptr, DNS_RDATA_OFFSET, &data[i],
                                        IP6_ADDR_LEN);
                            i += MAX_ADDRESS_SIZE;

                            (*resolved_addrs)++;
                        }

                        break;
#endif

                    default :

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

                        if (NU_Deallocate_Memory(name) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to deallocate memory for buffer",
                                           NERR_SEVERE, __FILE__, __LINE__);
#endif
                        return (NU_INVALID_PARM);
                }

                /* Get the time to live for this RR. */
                *ttl = GET32(rr_ptr, DNS_TTL_OFFSET);

                /* Indicate an answer was found. */
                answer_received = NU_SUCCESS;

                /* Invalidate this record so it isn't processed again by a subsequent
                 * call to DNS_Extract_Data.
                 */
                PUT16(p_ptr, DNS_TYPE_OFFSET, 0xffff);
            }

            /* Copy the length of this answer. */
            length = GET16(rr_ptr, DNS_RDLENGTH_OFFSET);

            /* Point to the next answer, if any.  The rdlength field is the
               length of the data section of the RR.  Add 10 for the sizes of
               the type, class, ttl, and rdlength.
            */
            p_ptr += (10 + length);
        }
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        if (NU_Deallocate_Memory(name) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for buffer",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif
    }

    return (answer_received);

} /* DNS_Extract_Data */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Add_Host
*
*   DESCRIPTION
*
*       This function adds a new host to the "hosts file".
*       A block of memory is allocated each time we want to add a new host.
*       This can be wasteful of memory because Nucleus PLUS has a minimum
*       allocation size of 50 bytes.  The alternative would have been to
*       allocate enough memory for several of these at one time.  The problem
*       with that is the RFC defines the maximum size of a name at 255 bytes.
*       We would have to assume that a maximum sized name would have to be
*       stored and allocate memory accordingly.  Since most names are nowhere
*       close to 255 bytes this method would have been even more wasteful.
*
*   INPUTS
*
*       *name                   A pointer to the name of the host to add.
*       *data                   A pointer to the record-specific data
*       ttl                     The Time To Live for this entry.
*       family                  The family type of the new entry.
*       type                    The type of entry; DNS_TYPE_A, DNS_TYPE_AAAA,
*                               DNS_TYPE_PTR.
*       data_len                If the data is an IP address, the number of
*                               IP addresses; otherwise, the length of the
*                               data in bytes.
*       flags                   Flags associated with the entry.
*       *extra_data             Data specific to the record type.
*
*   OUTPUTS
*
*       DNS_HOST*               A pointer to the new entry.
*       NU_NULL                 Insufficient memory to create the new entry.
*
******************************************************************************/
DNS_HOST *DNS_Add_Host(const CHAR *name, const CHAR *data, UNSIGNED ttl,
                       INT16 family, INT16 type, INT16 data_len, UINT8 flags,
                       VOID *extra_data)
{
    INT             size, name_size, i, j;
    DNS_HOST        *dns_host = NU_NULL;
    UNSIGNED        time;

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(family);
#endif

    /* Get the size of the name. */
    name_size = (INT)strlen(name) + 1;

    /* Retrieve the current time. */
    time = NU_Retrieve_Clock();

    /* Only reuse entries that contain IP address information. */
    if (family != NU_FAMILY_UNSPEC)
    {
        /* Add memory for the IP addresses. */
        size = ((DNS_MAX_IP_ADDRS + 1) * (MAX_ADDRESS_SIZE));

        /* Before allocating memory for a new entry, check to see if the TTL of an
           old entry has expired.  If so re-use that entry. */
        for (dns_host = DNS_Hosts.dns_head;
             dns_host;
             dns_host = dns_host->dns_next)
        {
            /* If this entry is not permanent. */
            if ( (dns_host->dns_family != NU_FAMILY_UNSPEC) &&
                 (!(dns_host->dns_flags & DNS_PERMANENT_ENTRY)) )
            {
                /* Check to see if the ttl has expired. */
                if (INT32_CMP(dns_host->dns_ttl, time) <= 0)
                {
                    /* Good so far.  Check to see if the name will fit in this
                       entry. */
                    if (dns_host->dns_name_buffer_size >= name_size)
                    {
                        name_size = dns_host->dns_name_buffer_size;
                        break;
                    }
                }
            }
        }
    }

    else if (data)
    {
        /* Add a byte for the null-terminator. */
        size = data_len + 1;
    }

    else
    {
        size = 0;
    }

    /* Did we find an entry that can be re-used. */
    if (!dns_host)
    {
        /* A new host structure must be created. */
        /* We need to compute the size of the block of memory to allocate.  The
           size will include the size of a host structure + length of the name +
           1 (for the null terminator).  We may want to reuse this entry later
           for other hosts.  So allocate a minimum number of bytes for the name.
           This will make it more likely that this entry can be reused.
        */

        /* If the name size is less than the minimum, then bump it up. */
        if (name_size < DNS_MIN_NAME_ALLOC)
            name_size = DNS_MIN_NAME_ALLOC;

        /* Ensure the size will be a multiple of 4 since we will be pointing
         * the data pointer after the name in memory.
         */
        if ((UNSIGNED)name_size % 4)
            name_size += (4 - ((UNSIGNED)name_size % 4));

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Allocate a block of memory for the new DNS host structure, name and data. */
        if (NU_Allocate_Memory(MEM_Cached, (VOID **)&dns_host,
                               (UNSIGNED)((UNSIGNED)size + name_size + sizeof(DNS_HOST)),
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            return (NU_NULL);
        }

        /* The name will be stored immediately after the structure. */
        dns_host->dns_name = (CHAR *)(dns_host + 1);
#else
        /* Assign memory to the dns host and their names */
        if (No_of_RES_Hosts == NET_MAX_DNS_HOSTS)
            return (NU_NULL);                 /* return if free memory could not be found*/

        dns_host =  &NET_DNS_Add_Memory[No_of_RES_Hosts];
        dns_host->dns_name = &NET_DNS_Name_Memory[No_of_RES_Hosts++];

#endif  /* INCLUDE_STATIC_BUILD */

        /* Initialize the size of the name of this host. */
        dns_host->dns_name_buffer_size = name_size;

        /* Add this host to the list. */
        DLL_Enqueue(&DNS_Hosts, dns_host);
    }

    /* Copy the parameters into the structure. */
    strcpy(dns_host->dns_name, name);
    dns_host->dns_type = type;
    dns_host->dns_flags = flags;
    dns_host->dns_family = family;

    /* Set up the ID. */
    dns_host->dns_id = DNS_Id ++;

    /* Ensure the ID is not zero. */
    if (dns_host->dns_id == 0)
    {
        dns_host->dns_id = DNS_Id ++;
    }

#if (INCLUDE_MDNS == NU_TRUE)

    /* Zero out the memory for the mDNS structure. */
    memset(&dns_host->dns_mdns, 0, sizeof(MDNS_STRUCT));

    /* Initialize the mDNS fields. */
    dns_host->mdns_state = MDNS_UNINITIALIZED;

#endif

    /* If there is data associated with the record. */
    if (data)
    {
        /* Set up the pointer to the data after the memory for the name. */
        dns_host->dns_record_data = (CHAR *)(&dns_host->dns_name[name_size]);
    }

    else
    {
        dns_host->dns_record_data = NU_NULL;
        dns_host->dns_h_length = 0;
    }

    /* If this record has an IP address associated with it. */
    if (family != NU_FAMILY_UNSPEC)
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        if (family == NU_FAMILY_IP6)
        {
            dns_host->dns_h_length = IP6_ADDR_LEN;
        }
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        else
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            dns_host->dns_h_length = IP_ADDR_LEN;
        }
#endif

        UTL_Zero(dns_host->dns_record_data, (DNS_MAX_IP_ADDRS + 1) * MAX_ADDRESS_SIZE);

        for (i=0, j=0; i < data_len; i++, j+=MAX_ADDRESS_SIZE)
        {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

            /* If this is an IPv4-Mapped IPv6 address, extract the IPv4 address from
             * the IPv6 address and add it to the record.
             */
            if ( (family == NU_FAMILY_IP) && (IPV6_IS_ADDR_V4MAPPED(&data[j])) )
                memcpy(&dns_host->dns_record_data[j], &data[j + 12],
                       (unsigned int)dns_host->dns_h_length);
            else
#endif
                memcpy(&dns_host->dns_record_data[j], &data[j],
                       (unsigned int)dns_host->dns_h_length);
        }
    }

    /* This is a SRV, TXT or PTR record that does not hold an IP address. */
    else if (data)
    {
        /* Copy the data into the structure.  The data will not be null-terminated
         * already if being added from a DNS response.
         */
        memcpy(dns_host->dns_record_data, data, data_len);

        /* Null terminate the data if it is not already. */
        dns_host->dns_record_data[data_len] = 0;

        /* Set the number of bytes (minus the null-terminator). */
        dns_host->dns_h_length = data_len;
    }

    /* If this is a SRV record, set the default priority and weight. */
    if ( (extra_data) && (dns_host->dns_type == DNS_TYPE_SRV) )
    {
        dns_host->dns_prio = ((DNS_SRV_DATA*)extra_data)->dns_srv_prio;
        dns_host->dns_weight = ((DNS_SRV_DATA*)extra_data)->dns_srv_weight;
        dns_host->dns_port = ((DNS_SRV_DATA*)extra_data)->dns_srv_port;
    }

    /* Convert TTL to ticks. */
    dns_host->dns_ttl = (ttl * SCK_Ticks_Per_Second);

    /* If the entry is not permanent, add the current clock to the value
     * so we can tell when the entry times out.
     */
    if (!(flags & DNS_PERMANENT_ENTRY))
    {
        dns_host->dns_ttl += NU_Retrieve_Clock();
    }

    return (dns_host);

} /* DNS_Add_Host */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Delete_Host
*
*   DESCRIPTION
*
*       This routine deletes a record from the DNS database.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record to delete.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID DNS_Delete_Host(DNS_HOST *dns_ptr)
{
#if (INCLUDE_MDNS == NU_TRUE)
    MDNS_NTFY_STRUCT    *ntfy_ptr;
    MDNS_RESPONSE       *resp_ptr;
    UNSIGNED            time_left = 0;
#endif

#if (INCLUDE_MDNS == NU_TRUE)

    /* If there is a query associated with this record. */
    if (dns_ptr->mdns_query)
    {
        /* Get a pointer to the callback list. */
        ntfy_ptr = dns_ptr->mdns_query->mdns_callback.head;

        while (ntfy_ptr)
        {
            /* Notify the application that this record has changed. */
            NU_Send_Signals(ntfy_ptr->callback_ptr, (1UL << MDNS_SIGNAL));

            ntfy_ptr = ntfy_ptr->next;
        }

        /* Get a pointer to the query's host list. */
        resp_ptr = dns_ptr->mdns_query->mdns_host_list.dns_head;

        /* Find this record in the query's host list and remove
         * it from the list.
         */
        while (resp_ptr)
        {
            if (resp_ptr->mdns_host == dns_ptr)
            {
                /* Remove this host from the query's list. */
                DLL_Remove(&dns_ptr->mdns_query->mdns_host_list, resp_ptr);

                NU_Deallocate_Memory(resp_ptr);

                break;
            }

            /* Get the next entry. */
            resp_ptr = resp_ptr->mdns_next;
        }

        /* If there are no records left on the the query's response list, and
         * the query timer is disabled, restart the query timer.
         */
        if ( (!dns_ptr->mdns_query->mdns_host_list.dns_head) &&
             (dns_ptr->mdns_query->mdns_qry_expire == 0) )
        {
            /* The initial timer should expire between 20 and 100 ms. */
            dns_ptr->mdns_query->mdns_qry_expire = (UTL_Rand() % MDNS_INIT_QUERY_MAX_DELAY) +
                     MDNS_INIT_QUERY_MIN_DELAY;

            /* Reset the timer. */
            NU_Reset_Timer(&dns_ptr->mdns_query->mdns_qry_timer, MDNS_Query_Handler,
                           dns_ptr->mdns_query->mdns_qry_expire, 0, NU_ENABLE_TIMER);
        }
    }

    /* Cleanup the mdns timer. */
    if (dns_ptr->mdns_timer)
    {
        /* Disable the timer. */
        if (NU_Control_Timer(dns_ptr->mdns_timer, NU_DISABLE_TIMER) == NU_SUCCESS)
        {
            NU_Delete_Timer(dns_ptr->mdns_timer);

            /* Deallocate memory for the timer. */
            NU_Deallocate_Memory(dns_ptr->mdns_timer);
            dns_ptr->mdns_timer = NU_NULL;
        }
    }

    /* If the record is local and not authoritative, send an unsolicited
     * response packet with a TTL of zero and then delete the record.
     */
    if ( (dns_ptr->dns_flags & DNS_LOCAL_RECORD) &&
         (!(dns_ptr->dns_flags & DNS_UNIQUE_RECORD)) )
    {
        /* Set the TTL to zero. */
        dns_ptr->dns_ttl = 0;

        /* Flag this record as requiring a response to be sent to transmit the
         * Goodbye packet.  This packet cannot be transmitted from this thread,
         * because this routine could have been called from another API, and
         * sending data from mDNS uses API routines, which will lead to us
         * suspending indefinitely on the semaphore that we already have.
         */
        dns_ptr->dns_flags |= DNS_DELAY_RESPONSE;

        /* If the timer for sending responses has not already been set, set the
         * timer now.
         * NU_Get_Remaining_Time will return an error if our timer is currently disabled.
         */
        if((NU_Get_Remaining_Time(&MDNS_Response_Timer, &time_left) != NU_SUCCESS) || (time_left <= 0))
        {
            /* Start our timer.
               Use SCK_Ticks_Per_Second/2 because we want the timer for fire in 500ms.
             */
            NU_Reset_Timer(&MDNS_Response_Timer, MDNS_Response_Handler, MDNS_RESPONSE_DELAY, 0,
                           NU_ENABLE_TIMER);
        }
    }

    /* Otherwise, delete the record now. */
    else
    {
        /* Remove this record from the database. */
        DLL_Remove(&DNS_Hosts, dns_ptr);

        /* Deallocate the memory for this host. */
        NU_Deallocate_Memory(dns_ptr);
    }

#else

    /* Remove this record from the database. */
    DLL_Remove(&DNS_Hosts, dns_ptr);

    /* Deallocate the memory for this host. */
    NU_Deallocate_Memory(dns_ptr);

#endif

} /* DNS_Delete_Host */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Update_Host
*
*   DESCRIPTION
*
*       This function adds an IP address to the existing host record.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record to update.
*       *ip_addr                A pointer to the new IP address for the record.
*
*   OUTPUTS
*
*       NU_SUCCESS on successful addition, otherwise, an operating-system
*       specific error is returned.
*
******************************************************************************/
STATUS DNS_Update_Host(DNS_HOST *dns_host, const CHAR* ip_addr)
{
    INT     i, j, n;
    UINT8   null_addr[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    STATUS  status;

    /* Count the number of entries in the list. */
    for (i = 0, j = 0; i < DNS_MAX_IP_ADDRS; i++, j += MAX_ADDRESS_SIZE)
    {
        /* If this is the last address in the list, exit the loop. */
        if (memcmp(&dns_host->dns_record_data[j], null_addr,
                   (unsigned int)dns_host->dns_h_length) == 0)
            break;
    }

    /* If there is room for another IP address, add it. */
    if (i < DNS_MAX_IP_ADDRS)
    {
        /* IP addresses should be added in order, from lowest to highest so, if
         * mDNS is in use, comparison with incoming queries/responses is
         * easier.
         */
        for (j = 0; j < (i * MAX_ADDRESS_SIZE); j += MAX_ADDRESS_SIZE)
        {
            /* Compare the n bytes of the new IP address against this
             * entry.
             */
            for (n = 0; n < dns_host->dns_h_length; n ++)
            {
                /* If this byte of the new address is smaller than
                 * this byte of the existing address, replace the
                 * existing address with the new address.
                 */
                if (ip_addr[n] < dns_host->dns_record_data[j + n])
                {
                    /* Move all the addresses down one position. */
                    memmove(&dns_host->dns_record_data[j + MAX_ADDRESS_SIZE],
                            &dns_host->dns_record_data[j],
                            (i - j) * MAX_ADDRESS_SIZE);

                    break;
                }
            }

            /* If this address is smaller than an existing address, exit
             * the loop.
             */
            if (n != dns_host->dns_h_length)
                break;
        }

        /* Copy the new address. */
        memcpy(&dns_host->dns_record_data[j], ip_addr,
               (unsigned int)dns_host->dns_h_length);

        /* If the entry is not permanent. */
        if (!(dns_host->dns_flags & DNS_PERMANENT_ENTRY))
        {
            /* Reset the TTL to reflect that this host has been updated. */
            dns_host->dns_ttl = dns_host->dns_ttl + NU_Retrieve_Clock();
        }

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error. */
    else
    {
        status = NU_NO_MEMORY;
    }

    return (status);

} /* DNS_Update_Host */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Remove_DNS_Server
*
*   DESCRIPTION
*
*       This function removes a given DNS server from the list of DNS
*       servers.
*
*   INPUTS
*
*       *dns_server             A pointer to the DNS server to remove.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*
******************************************************************************/
STATUS DNS_Remove_DNS_Server(DNS_SERVER *dns_server)
{
    /* Remove the server from the list. */
    DLL_Remove(&DNS_Servers, dns_server);

    /* Clear the IP address. */
    UTL_Zero(dns_server->dnss_ip, MAX_ADDRESS_SIZE);

    /* Add it back to the end of the list from which it can be reused. */
    DLL_Enqueue(&DNS_Servers, dns_server);

    return (NU_SUCCESS);

} /* DNS_Remove_DNS_Server */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Find_Host_By_ID
*
*   DESCRIPTION
*
*       This function searches the host list for a host matching the ID
*       passed in.
*
*   INPUTS
*
*       INT                     The target ID.
*
*   OUTPUTS
*
*       DNS_HOST*               A pointer to the DNS host.
*       NU_NULL                 No matching entry exists.
*
******************************************************************************/
DNS_HOST *DNS_Find_Host_By_ID(INT id)
{
    DNS_HOST        *l_host;

    for (l_host = DNS_Hosts.dns_head; l_host; l_host = l_host->dns_next)
    {
        /* Is this the one we're looking for. */
        if (l_host->dns_id == id)
        {
            break;
        }
    }

    return (l_host);

} /* DNS_Find_Host_By_ID */


