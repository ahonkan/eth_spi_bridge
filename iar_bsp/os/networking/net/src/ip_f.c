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
*       ip_f.c
*
* DESCRIPTION
*
*       This file contains the implementation of IP_Forward.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Forward
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_api.h"
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_FORWARDING == NU_TRUE) )

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/***********************************************************************
*
*   FUNCTION
*
*       IP_Forward
*
*   DESCRIPTION
*
*       This function checks attempts to forward an IP packet out of
*       one of the network interfaces.
*
*   INPUTS
*
*       *buf_ptr                Pointer to a buffer containing the
*                               datagram to forward.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_ADDRESS      IP_Canforward fails or buf_ptr->mem_flags
*                               is not set to NET_BCAST
*       NU_HOST_UNREACHABLE     HOST is outside of the scope.
*       -1                      Packet not forwarded.
*
*************************************************************************/
STATUS IP_Forward(NET_BUFFER *buf_ptr)
{
    UINT8               ttl;
    UINT16              checksum;
    UINT32              dest_addr;
    STATUS              stat;
    RTAB_ROUTE          rt_entry;
    RTAB4_ROUTE_ENTRY   *rt;
#if (INCLUDE_LITE_ICMP == NU_FALSE)
    DEV_IF_ADDR_ENTRY   *d_addr;
    UINT32              src;
#endif
    SCK_SOCKADDR_IP     *dest_ip;
    IPLAYER             *ip_pkt;
    NET_BUFFER          *hdr_buf;

#if (INCLUDE_IPSEC == NU_TRUE)
    UINT8               protocol;
    IPLAYER             *ip_dgram;
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
    /* Initialize the status to NU_SUCCESS. */
    stat = NU_SUCCESS;
#endif

    /* Initialize local variables. */
    ip_pkt = (IPLAYER *)buf_ptr->data_ptr;

    dest_addr = GET32(ip_pkt, IP_DEST_OFFSET);

    if ( (buf_ptr->mem_flags & NET_BCAST) ||
         (buf_ptr->mem_flags & NET_MCAST) ||
         (IP_CANFORWARD(dest_addr) != NU_SUCCESS) )
    {
        /* Deallocate the buffer. */
        MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

        /* Increment the number of IP packets received with the wrong IP addr.*/
        MIB2_ipInAddrErrors_Inc;

        return (NU_INVALID_ADDRESS);
    }

    /* Extract the TTL of the packet */
    ttl = GET8(ip_pkt, IP_TTL_OFFSET);

    /* Check the time to live field. */
    if (ttl <= 1)
    {
        ICMP_Send_Error(buf_ptr, ICMP_TIMXCEED, ICMP_TIMXCEED_TTL, 0,
                        buf_ptr->mem_buf_device);

        /* Deallocate the buffer. */
        MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

        return (-1);
    }

    /* Decrement the time to live. */
    PUT8(ip_pkt, IP_TTL_OFFSET, (ttl - 1));

    /* Increment the number of packets that we attempted to find a route
       and forward. */
    MIB2_ipForwDatagrams_Inc;

    /* Check to see if the cached route is still valid. */
    if ( (buf_ptr->mem_buf_device->dev_forward_rt.rt_route == NU_NULL) ||
         (dest_addr != buf_ptr->mem_buf_device->dev_forward_rt.rt_ip_dest.sck_addr) )
    {
        rt_entry.rt_route = NU_NULL;

        rt_entry.rt_ip_dest.sck_family = SK_FAM_IP;
        rt_entry.rt_ip_dest.sck_len = sizeof(rt_entry.rt_ip_dest);
        rt_entry.rt_ip_dest.sck_addr = dest_addr;

        IP_Find_Route(&rt_entry);

        /* A route was found */
        if (rt_entry.rt_route)
        {
            /* We cannot use the cached route.  If there is one then free it. */
            if (buf_ptr->mem_buf_device->dev_forward_rt.rt_route)
                RTAB_Free((ROUTE_ENTRY*)buf_ptr->mem_buf_device->dev_forward_rt.rt_route,
                          NU_FAMILY_IP);

            /* Set the cached route for the device to this route */
            buf_ptr->mem_buf_device->dev_forward_rt = rt_entry;
        }

        /* If a route could not be found, send an ICMP Host Unreachable
         * error to the source.
         */
        else
        {
            /* Send ICMP host unreachable message. */
            ICMP_Send_Error(buf_ptr, ICMP_UNREACH, ICMP_UNREACH_HOST, 0,
                            buf_ptr->mem_buf_device);

            /* Deallocate the buffer. */
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);

            /* Increment the number of packets that could not be delivered
               because a route could not be found. */
            MIB2_ipOutNoRoutes_Inc;

            return (NU_HOST_UNREACHABLE);
        }
    }

    /* Get a pointer to the route */
    rt = buf_ptr->mem_buf_device->dev_forward_rt.rt_route;

    /* Get a pointer to the next-hop */
    if (rt->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
        dest_ip = &buf_ptr->mem_buf_device->dev_forward_rt.rt_route->rt_gateway_v4;
    else
        dest_ip = &buf_ptr->mem_buf_device->dev_forward_rt.rt_ip_dest;

#if (INCLUDE_LITE_ICMP == NU_FALSE)

    /* If the packet was received on the same device out which it will be
     * forwarded, send an ICMP Redirect message to the source.
     */
    if ( (rt->rt_entry_parms.rt_parm_device == buf_ptr->mem_buf_device) &&
         ((rt->rt_entry_parms.rt_parm_flags & (RT_DYNAMIC | RT_MODIFIED)) == 0) &&
         (*(UINT32 *)rt->rt_route_node->rt_ip_addr != 0) )
    {
        d_addr = rt->rt_entry_parms.rt_parm_device->dev_addr.dev_addr_list.dv_head;

        /* Extract the source address from the packet */
        src = GET32(ip_pkt, IP_SRC_OFFSET);

        /* Loop through the list of addresses on the device */
        while (d_addr)
        {
            if ((src & d_addr->dev_entry_netmask) == d_addr->dev_entry_net)
            {
                /* Send ICMP host unreachable message. */
                ICMP_Send_Error(buf_ptr, ICMP_REDIRECT, ICMP_REDIRECT_HOST,
                                dest_ip->sck_addr, buf_ptr->mem_buf_device);

                break;
            }

            /* Get a pointer to the next address on the device */
            d_addr = d_addr->dev_entry_next;
        }
    }

#endif

#if (HARDWARE_OFFLOAD == NU_TRUE)
    /* If the hardware is not going to compute the checksum, incrementally update
     * the checksum to reflect the 1 byte decrement of the TTL.
     */
    if (!(rt->rt_entry_parms.rt_parm_device->dev_hw_options_enabled & HW_TX_IP4_CHKSUM))
#endif
    {
        /* Extract the current checksum from the packet. */
        checksum = GET16(ip_pkt, IP_CHECK_OFFSET);

        /* If the current checksum is less than 0xfeff, increment only the high
         * byte of the checksum; ie, 0x3602 becomes 0x3702.
         */
        if (checksum < 0xfeff)
            checksum += (1 << 8);

        /* Otherwise, the low byte of the checksum will also need to be incremented;
         * ie - 0xfff8 becomes 0x00f9, and 0xfeff becomes 0x0000. Note that
         * -0 (0xffff) is not a valid checksum value; therefore, 0xfeff does
         * not become 0xffff.
         */
        else
            checksum += ((1 << 8) + 1);

        /* Store the new checksum back in the packet. */
        PUT16(ip_pkt, IP_CHECK_OFFSET, checksum);
    }

#if (HARDWARE_OFFLOAD == NU_TRUE)
    else
        PUT16(ip_pkt, IP_CHECK_OFFSET, 0);
#endif

    /* If the header length of the outgoing interface is less than or
     * equal to the header length of the interface on which the packet
     * was received, reuse the incoming header.
     */
    if (rt->rt_entry_parms.rt_parm_device->dev_hdrlen <=
        buf_ptr->mem_buf_device->dev_hdrlen)
    {
        hdr_buf = buf_ptr;

        /* Set the flag indicating that this chain of buffers does not contain
         * a buffer dedicated only to the link-layer and IP layer headers.  This
         * is required by PPPoE when performing MSS modifications.
         */
        hdr_buf->mem_flags |= NET_NOHDR;
    }

    /* Otherwise, append a new buffer to the chain for the IP header. */
    else
    {
        /* Allocate a new buffer chain for the link-layer and IP header */
        hdr_buf =
            MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                     (INT32)rt->rt_entry_parms.
                                     rt_parm_device->dev_hdrlen);

        /* If a new chain of buffers was allocated, link the buffer in as
         * the first buffer in the list.
         */
        if (hdr_buf)
        {
            /* Link the new buffer in to the list */
            hdr_buf->next_buffer = buf_ptr;

            /* Set the list to which the header buffers will be freed */
            hdr_buf->mem_dlist = &MEM_Buffer_Freelist;

            /* Set the data pointer past the beginning of the link-layer header,
             * because the interface output routine will move the pointer back
             * to the beginning of the link-layer header.
             */
            hdr_buf->data_ptr = hdr_buf->mem_parent_packet +
                rt->rt_entry_parms.rt_parm_device->dev_hdrlen;

            /* Set the total data length of the chain of buffers */
            hdr_buf->mem_total_data_len = buf_ptr->mem_total_data_len;
        }

        /* If a buffer could not be allocated, return an error. */
        else
            return (NU_NO_BUFFERS);
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Check if IPsec is enabled on the output device. */
    if (rt->rt_entry_parms.rt_parm_device->dev_flags2 & DV_IPSEC_ENABLE)
    {
        /* Get the protocol. */
        protocol = GET8(hdr_buf->data_ptr, IP_PROTOCOL_OFFSET);

        /* Set the pointer to the IP header. */
        ip_dgram = (IPLAYER*)hdr_buf->data_ptr;

        /* Apply IPsec. */

        /* If IPsec 2.0 is used */
        #ifdef IPSEC_VERSION_COMP
            stat = IP_IPSEC_Forward(rt->rt_entry_parms.rt_parm_device,
                       &hdr_buf, (VOID **)&ip_dgram,
                       IPSEC_IPV4, (UINT8)protocol,
                       (UINT8 *)&(rt->rt_entry_parms.rt_parm_device->
                       dev_addr.dev_addr_list.dv_head->
                       dev_entry_ip_addr), NU_NULL );
        #else
            /* Otherwise IPsec 1.x is used. */
            stat = IP_IPSEC_Forward(rt->rt_entry_parms.rt_parm_device,
                       &hdr_buf, (VOID **)&ip_dgram,
                       IPSEC_IPV4, (UINT8)protocol);
        #endif

        /* Update the IP packet pointer since it might be modified if
         * IPsec is used in Tunnel Mode.
         */
        ip_pkt = ip_dgram;

        /* Compute the IP checksum.*/
        PUT16(ip_pkt, IP_CHECK_OFFSET, 0);

        PUT16(ip_pkt, IP_CHECK_OFFSET,
              TLS_IP_Check((VOID*)ip_pkt, (UINT16)(IP_HEADER_LEN >> 1)));
    }

    if (stat == NU_SUCCESS)
#endif
    {

        /* If this packet is small enough send it.*/
        if (GET16(ip_pkt, IP_TLEN_OFFSET) <=
            rt->rt_entry_parms.rt_parm_device->dev_mtu)
        {
            /* Set the packet type that is in the buffer. */
            hdr_buf->mem_flags |= NET_IP;

            /* Send the packet. */
            stat =
                (*(rt->rt_entry_parms.rt_parm_device->dev_output))(hdr_buf,
                                                                   rt->rt_entry_parms.
                                                                   rt_parm_device,
                                                                   dest_ip, rt);
        }

        /* This packet must be fragmented. */
        else
        {
#if (INCLUDE_IP_FRAGMENT == NU_TRUE)

            /* If the don't fragment bit is set return an error. */
            if (GET16(ip_pkt, IP_FRAGS_OFFSET) & IP_DF)
            {
                /* Increment the number of IP packets that could not be
                   fragmented. In this case because the don't fragment
                   bit is set. */
                MIB2_ipFragFails_Inc;

                stat = NU_MSGSIZE;
            }

            else
                stat = IP_Fragment(hdr_buf, ip_pkt,
                                   rt->rt_entry_parms.rt_parm_device, dest_ip,
                                   &buf_ptr->mem_buf_device->dev_forward_rt);
#else
            stat = NU_MSGSIZE;
#endif
        }
    }

    if (stat != NU_SUCCESS)
    {
        /* Send ICMP host unreachable message. */
        if (stat == NU_HOST_UNREACHABLE)
            ICMP_Send_Error(buf_ptr, ICMP_UNREACH, ICMP_UNREACH_HOST,
                            dest_ip->sck_addr, buf_ptr->mem_buf_device);

        /* Send a Datagram Too Big message. */
        else if (stat == NU_MSGSIZE)
            ICMP_Send_Error(buf_ptr, ICMP_UNREACH, ICMP_UNREACH_NEEDFRAG,
                            dest_ip->sck_addr, buf_ptr->mem_buf_device);

        MEM_One_Buffer_Chain_Free(hdr_buf, hdr_buf->mem_dlist);

         /* Increment the number of IP packets received with the wrong IP addr.*/
        MIB2_ipInAddrErrors_Inc;
    }

    return (stat);

} /* IP_Forward */

#endif



