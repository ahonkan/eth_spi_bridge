/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*       ip6_f.c                                      
*
*   DESCRIPTION
*
*       This file contains the implementation of IP6_Forward.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP6_Forward
*       IP6_Canforward
*
*   DEPENDENCIES
*
*       nu_net.h
*       prefix6.h
*       ip6_mib.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/prefix6.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_api.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

STATIC STATUS IP6_Canforward(UINT8 *dest);

extern RTAB6_ROUTE  IP6_Forward_Rt;

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Forward                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function checks attempts to forward an IPv6 packet out of     
*       one of the network interfaces.                                   
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                Pointer to a buffer containing the 
*                               datagram to forward.
*       *ip_pkt                 Pointer to the IPV6 header of the 
*                               incoming packet.        
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Success
*       NU_INVALID_ADDRESS      IP_Canforward fails or buf_ptr->mem_flags
*                               is not set to NET_BCAST                                            
*       NU_HOST_UNREACHABLE     HOST is outside of the scope.                                            
*       NU_INVALID_PARM         The Hop Limit has been decremented to 
*                               zero; therefore, the packet was not
*                               forwarded.
*       NU_MSG_TOO_LONG         The packet needs fragmented, but this
*                               is not the source node.
*                                                                       
*************************************************************************/
STATUS IP6_Forward(IP6LAYER *ip_pkt, NET_BUFFER *buf_ptr)
{
    SCK6_SOCKADDR_IP    *sin;
    RTAB6_ROUTE_ENTRY   *rt;
    STATUS              stat;
    IP6_S_OPTIONS       ip6_options;
    UINT32              saved_data_len, saved_total_data_len;
    UINT8 HUGE          *saved_data_ptr;
    UINT8               *dest;
    UINT16              payload_length;

#if (INCLUDE_IPSEC == NU_TRUE)

    DV_DEVICE_ENTRY         *int_face;
    IPSEC_SELECTOR          packet_selector;
    IPSEC_OUTBOUND_BUNDLE   *out_bundle;
    IPSEC_POLICY            *policy_ptr = NU_NULL;
    UINT8                   ipsec_tunnel_mode = NU_FALSE;

#endif

    /* If the destination address is multicast or the the packet cannot be
     * forwarded, free the buffer and return an error.
     */
    if ( (buf_ptr->mem_flags & NET_MCAST) ||
         (IP6_Canforward(ip_pkt->ip6_dest) != NU_SUCCESS) )
    {
        /* Increment the number of IP packets received with the wrong IP
         * Addr.
         */
        MIB_ipv6IfStatsInAddrErr_Inc(buf_ptr->mem_buf_device);

        /* Deallocate the buffer. */
        MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

        return (NU_INVALID_ADDRESS);
    }

    /* Extract the length of the payload from the packet */
    payload_length = GET16(ip_pkt, IP6_PAYLEN_OFFSET);

    /* If this node will decrement the Hop Limit to zero, transmit an ICMPv6
     * Time Exceeded message, free the buffer and return an error.
     */
    if (ip_pkt->ip6_hop <= 1)
    {
        MIB_ipv6IfStatsInHdrErrors_Inc(buf_ptr->mem_buf_device);

        /* Set the data pointer to the head of the packet */
        buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;

        /* Put the IPv6 and extension header length back on the packet length */
        buf_ptr->data_len += 
            (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

        buf_ptr->mem_total_data_len += 
            (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

        ICMP6_Send_Error(ip_pkt, buf_ptr, ICMP6_TIME_EXCEEDED, 
                         ICMP6_TIME_EXCD_HPLMT, 0, buf_ptr->mem_buf_device);

        /* Deallocate the buffer. */
        MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

        return (NU_INVALID_PARM);
    }

    /* Decrement the Hop Limit. */
    ip_pkt->ip6_hop --;

    rt = IP6_Forward_Rt.rt_route;

    /* Check to see if the cached route is still valid. */
    if ( (rt == 0) || 
         (memcmp(ip_pkt->ip6_dest, rt->rt_next_hop.sck_addr, IP6_ADDR_LEN) != 0) )
    {
        /* We cannot use the cached route.  If there is one then free it. */
        if (IP6_Forward_Rt.rt_route)
        {
            RTAB_Free((ROUTE_ENTRY*)IP6_Forward_Rt.rt_route, NU_FAMILY_IP6);
            IP6_Forward_Rt.rt_route = NU_NULL;
        }

        /* Point to the destination. */
        sin = &IP6_Forward_Rt.rt_ip_dest.rtab6_rt_ip_dest;

        sin->sck_family = SK_FAM_IP6;
        sin->sck_len = sizeof(SCK6_SOCKADDR_IP);
        NU_BLOCK_COPY(sin->sck_addr, ip_pkt->ip6_dest, IP6_ADDR_LEN);

        /* Attempt to find a route to the destination */
        IP6_Find_Route(&IP6_Forward_Rt);

        /* If a route could not be found, send an ICMPv6 Host Unreachable
         * error to the source.
         */
        if (IP6_Forward_Rt.rt_route == 0)
        {
            /* Set the data pointer to the head of the packet */
            buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;

            /* Put the IPv6 and extension header length back on the packet length */
            buf_ptr->data_len += 
                (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

            buf_ptr->mem_total_data_len += 
                (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

            /* Send ICMPv6 host unreachable message. */
            ICMP6_Send_Error(ip_pkt, buf_ptr, ICMP6_DST_UNREACH, 
                             ICMP6_DST_UNREACH_NOROUTE, 0, buf_ptr->mem_buf_device);

            /* Increment the number of packets that could not be delivered
             * because a route could not be found. 
             */
            MIB_ipv6IfStatsInNoRoutes_Inc(buf_ptr->mem_buf_device);

            /* Deallocate the buffer. */
            MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

            return (NU_HOST_UNREACHABLE);
        }

        rt = IP6_Forward_Rt.rt_route;

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Get a pointer to the interface. */
        int_face = IP6_Forward_Rt.rt_ip_dest.rtab6_rt_device;

        /* If IPsec is enabled on the interface. */
        if (int_face->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Update the selector for the packet */
            IP_IPSEC_Pkt_Selector(&packet_selector, ip_pkt, buf_ptr,
                                  (INT16)ip_pkt->ip6_next, IPSEC_IPV6, 0);

            /* Match the packet's selector fields against the outbound
             * policies in the SPD
             */
            stat = 
                IPSEC_Match_Policy_Out(int_face->dev_physical->dev_phy_ips_group,
                                       &packet_selector, &policy_ptr, 
                                       &out_bundle);

            if (stat != NU_SUCCESS)
            {
                NLOG_Error_Log("IPsec policy out check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Deallocate the buffer. */
                MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

                /* Return the error status. */
                return (IPSEC_PKT_DISCARD);
            }

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
            else
            {

                /* If tunnel mode is present, update the route. */
                if (policy_ptr->ipsec_security->ipsec_security_mode ==
                    IPSEC_TUNNEL_MODE)
                {
                    /* Set the tunnel mode flag. */
                    ipsec_tunnel_mode = NU_TRUE;

                    /* We cannot use the cached route.  If there is one
                     * then free it.
                     */
                    if (IP6_Forward_Rt.rt_route != 0)
                    {
                        RTAB_Free((ROUTE_ENTRY*)IP6_Forward_Rt.rt_route,
                                  NU_FAMILY_IP6);

                        IP6_Forward_Rt.rt_route = NU_NULL;
                    }

                    /* Point to the destination. */
                    sin = &IP6_Forward_Rt.rt_ip_dest.rtab6_rt_ip_dest;

                    /* Set the new tunnel destination address.*/
                    sin->sck_family = SK_FAM_IP6;
                    sin->sck_len = sizeof(SCK6_SOCKADDR_IP);

                    NU_BLOCK_COPY(sin->sck_addr,
                                  policy_ptr->ipsec_security->
                                  ipsec_tunnel_destination, IP6_ADDR_LEN);

                    /* Attempt to find a route to new the destination. */
                    IP6_Find_Route(&IP6_Forward_Rt);

                    /* If a route could not be found, send an ICMPv6 Host
                     * Unreachable error to the source.
                     */
                    if (IP6_Forward_Rt.rt_route == 0)
                    {
                        /* Set the data pointer to the head of the packet. */
                        buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;

                        /* Put the IPv6 and extension header length back on
                         * the packet length. 
                         */
                        buf_ptr->data_len += (IP6_HEADER_LEN + 
                           (payload_length - buf_ptr->mem_total_data_len));

                        buf_ptr->mem_total_data_len += (IP6_HEADER_LEN + 
                           (payload_length - buf_ptr->mem_total_data_len));

                        /* Send ICMPv6 host unreachable message. */
                        ICMP6_Send_Error(ip_pkt, buf_ptr, ICMP6_DST_UNREACH,
                                         ICMP6_DST_UNREACH_NOROUTE, 0,
                                         buf_ptr->mem_buf_device);

                        /* Increment the number of packets that could not 
                         * be delivered because a route could not be found.
                         */
                        MIB_ipv6IfStatsInNoRoutes_Inc(buf_ptr->mem_buf_device);

                        /* Deallocate the buffer. */
                        MEM_One_Buffer_Chain_Free(buf_ptr,buf_ptr->mem_dlist);

                        return (NU_HOST_UNREACHABLE);
                    }

                    /* Update route entry. */
                    rt = IP6_Forward_Rt.rt_route;
                }
            }
#endif
        }
#endif
    }

    /* If the packet was received on the same device out which it will be
     * forwarded and the source address of the packet identifies a neighbor, 
     * send an ICMPv6 Redirect message to the source.
     */
    if ( (rt->rt_entry_parms.rt_parm_device == buf_ptr->mem_buf_device) &&
         ((rt->rt_entry_parms.rt_parm_flags & (RT_DYNAMIC | RT_MODIFIED)) == 0) &&
         (!IPV6_IS_ADDR_UNSPECIFIED(rt->rt_route_node->rt_ip_addr)) &&
         ((PREFIX6_Match_Longest_Prefix_By_Device(buf_ptr->mem_buf_device->dev6_prefix_list, 
                                                  ip_pkt->ip6_src)) ||
          (buf_ptr->mem_buf_device->dev6_fnd_neighcache_entry(buf_ptr->mem_buf_device, 
                                                              ip_pkt->ip6_src))) )
    {
        /* Save the packet's original data pointer and length values */
        saved_data_ptr = buf_ptr->data_ptr;
        saved_data_len = buf_ptr->data_len;
        saved_total_data_len = buf_ptr->mem_total_data_len;

        /* Set the data pointer to the head of the packet */
        buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;

        /* Put the IPv6 and extension header length back on the packet length */
        buf_ptr->data_len += 
            (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

        buf_ptr->mem_total_data_len += 
            (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

        /* Determine the address to put in the packet that should be used
         * as the next-hop instead of this node.
         */
        if (rt->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
            dest = rt->rt_next_hop.sck_addr;
        else
            dest = ip_pkt->ip6_dest;

        /* Send ICMPv6 Redirect message. */
        ICMP6_Send_Error(ip_pkt, buf_ptr, ICMP6_REDIRECT, 0, 
                         (UINT32)(dest), buf_ptr->mem_buf_device);

        /* Restore the packet */
        buf_ptr->data_ptr = saved_data_ptr;
        buf_ptr->data_len = saved_data_len;
        buf_ptr->mem_total_data_len = saved_total_data_len;
    }

    /* If the header length of the outgoing interface is less than or
     * equal to the header length of the interface on which the packet
     * was received, flag IP_Send to reuse the header.
     */
    if (rt->rt_entry_parms.rt_parm_device->dev_hdrlen <= 
        buf_ptr->mem_buf_device->dev_hdrlen)
    {
        buf_ptr->mem_flags |= NET_NOHDR;
        
        /* Set the data pointer to the head of the packet */
        buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;

        /* Put the extension header length back on the packet length */
        buf_ptr->data_len += (payload_length - buf_ptr->mem_total_data_len);

        buf_ptr->mem_total_data_len += 
            (payload_length - buf_ptr->mem_total_data_len);
    }

    /* If after processing the routing header of a received packet, an
     * intermediate node determines that the packet is to be forwarded
     * onto a link whose link mtu is less than the size of the packet
     * the node must send an ICMP packet too big message to the
     * packets Source address ----- RFC 2460
     */
    if (rt->rt_device->dev6_link_mtu <= 
        (UINT32)(payload_length + IP6_HEADER_LEN + rt->rt_device->dev_hdrlen))
    {
        MIB_ipv6IfStatsInTooBigErr_Inc(buf_ptr->mem_buf_device);

        /* Set the data pointer to the head of the packet */
        buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;
        
        /* Put the IPv6 and extension header length back on the packet length */
        buf_ptr->data_len += 
            (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));
        
        buf_ptr->mem_total_data_len += 
            (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));        
        
        /* We swapped the destination and the intermediate node address.
         * We must send this error to the source.
         */
        NU_BLOCK_COPY(ip_pkt->ip6_dest, ip_pkt->ip6_src, IP6_ADDR_LEN);
        
        /* Send ICMPv6 Packet Too Big Message. */
        ICMP6_Send_Error(ip_pkt, buf_ptr, ICMP6_PACKET_TOO_BIG, 0, 
                         rt->rt_device->dev6_link_mtu, buf_ptr->mem_buf_device);
        
        MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
        
        return (NU_MSG_TOO_LONG);
    }

    memset(&ip6_options, 0, sizeof(ip6_options));

#if ( (INCLUDE_IPSEC == NU_TRUE) && (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) )

    /* If in tunnel mode, change the source and destination address. */
    if (ipsec_tunnel_mode == NU_TRUE)
    {
        ip6_options.tx_source_address =
            policy_ptr->ipsec_security->ipsec_tunnel_source;

        ip6_options.tx_dest_address =
            policy_ptr->ipsec_security->ipsec_tunnel_destination;
    }

    else
#endif

    {
        ip6_options.tx_source_address = ip_pkt->ip6_src;
        ip6_options.tx_dest_address = ip_pkt->ip6_dest;
    }

    /* Forward the packet.  Because this is a forward many of the parameters are
     * not required.  Specifically the length, the ttl, the protocol, and tos.
     * NU_NULL is used for all of those parameters that will not be needed.
     */
    stat = IP6_Send(buf_ptr, &ip6_options, NU_NULL, &IP6_Forward_Rt, 
                    IP_FORWARDING, NU_NULL);

    if (stat != NU_SUCCESS)
    {
        /* Send ICMP host unreachable message. */
        if (stat == NU_HOST_UNREACHABLE)
        {
            /* Put the IPv6 header length back on the packet length */
            if (buf_ptr->mem_flags & NET_NOHDR)
            {      
                /* Put the extension header length back on the packet length */
                buf_ptr->data_len += IP6_HEADER_LEN;

                buf_ptr->mem_total_data_len += IP6_HEADER_LEN;
            }

            /* Put the IPv6 header and extension header length back on 
             * the packet length and set the data pointer to the beginning
             * of the packet.
             */
            else
            {      
                /* Set the data pointer to the head of the packet */
                buf_ptr->data_ptr = (UINT8 HUGE*)ip_pkt;

                /* Put the extension header length back on the packet length */
                buf_ptr->data_len += 
                    (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));

                buf_ptr->mem_total_data_len += 
                    (IP6_HEADER_LEN + (payload_length - buf_ptr->mem_total_data_len));
            }

            ICMP6_Send_Error(ip_pkt, buf_ptr, ICMP6_DST_UNREACH, 
                             ICMP6_DST_UNREACH_NOROUTE, 0, buf_ptr->mem_buf_device);
        }

        MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
    }

    else
        MIB_ipv6IfStatsOutForDgram_Inc(IP6_Forward_Rt.rt_route->rt_device);

    return (stat);

} /* IP6_Forward */

/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Canforward                                                    
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function decides if an attempt should be made to forward    
*       an IPv6 datagram.                                                  
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *dest                   The destination IP address.                          
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       -1                      Don't attempt forward the packet.                    
*       NU_SUCCESS              Forward the packet.                                  
*                                                                       
*************************************************************************/
STATIC STATUS IP6_Canforward(UINT8 *dest)
{
    /* If the destination address is link-local or the loopback address,
     * do not forward the packet.
     */
    if ( (IPV6_IS_ADDR_LINKLOCAL(dest)) || (IPV6_IS_ADDR_LOOPBACK(dest)) )
        return (-1);
    else
        return (NU_SUCCESS);

} /* IP6_Canforward */
