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
*   FILENAME
*
*       udp4.c
*
*   DESCRIPTION
*
*       UDP Protocol routines for IPv4 packets.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       UDP4_Input
*       UDP4_Cache_Route
*       UDP4_Find_Matching_UDP_Port
*       UDP4_Free_Cached_Route
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)
extern INT SCK_ReuseAddr_Set;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       UDP4_Input
*
*   DESCRIPTION
*
*       This function verifies the UDP checksum of an IPv4 UDP packet,
*       and forwards the packet to the UDP_Interpret routine.
*
*   INPUTS
*
*       *pkt                    A pointer to the IPv4 header.
*       *buf_ptr                A pointer to the NET Buffer.
*       *tcp_chk                A pointer to the TCP pseudo structure.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       2
*       -1
*
*************************************************************************/
INT16 UDP4_Input(IPLAYER *pkt, NET_BUFFER *buf_ptr, struct pseudotcp *tcp_chk)
{
    UINT16              hischeck, mycheck;
    UINT16              dest_port, source_port;
    UINT16              i;
    INT                 saved_i = -1;
    UINT32              local_addr, foreign_addr;
    UDP_PORT            *uptr;
    UINT32              dest_addr;
    struct sock_struct  *sockptr;
    UINT16              *src;
    UINT8               *saved_ptr;

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) && \
      (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
    MULTI_SCK_OPTIONS       *moptions;
    MULTI_SCK_STATE         *igmp_state;
    UINT8                   multi_dest[IP_ADDR_LEN];
    UINT8                   multi_src[IP_ADDR_LEN];
#endif

#if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE))
    UDPLAYER    *udp_info;
#endif

    /* If the checksum was computed in the controller bypass the software
     * checksum efforts.
     */
#if (HARDWARE_OFFLOAD == NU_TRUE)
    if (!(buf_ptr->hw_options & HW_RX_UDP_CHKSUM))
#endif
    {
        /* Was a checksum computed by the foreign host?  If so, verify the
         * checksum.
         */
        hischeck = GET16(buf_ptr->data_ptr, UDP_CHECK_OFFSET);

        if (hischeck != 0)
        {
            /* If the buffer includes the running total of the data to
             * be used for the checksum.
             */
            if (buf_ptr->mem_flags & NET_BUF_SUM)
            {
                /* Get a pointer to the 2-bytes of checksum data in the UDP
                 * header.
                 */
                src = (UINT16*)(&buf_ptr->data_ptr[UDP_CHECK_OFFSET]);

                /* Subtract the checksum from the running data total since
                 * the checksum for this packet was computed with a value of
                 * zero in this field by the sender.
                 */
                buf_ptr->chk_sum -= src[0];
            }

            /* Zero out the checksum field. */
            PUT16(buf_ptr->data_ptr, UDP_CHECK_OFFSET, 0);

            /* Perform the checksum. */
            mycheck = TLS_TCP_Check((UINT16 *)tcp_chk, buf_ptr);

            /* If a checksum of zero is computed it should be replaced with
             * 0xffff.
             */
            if (mycheck == 0)
                mycheck = 0xFFFF;

            if (hischeck != mycheck)
            {
                /* The Checksum failed log an error and drop the packet. */
                NLOG_Error_Log("UDP checksum error", NERR_RECOVERABLE,
                               __FILE__, __LINE__);

                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Increment the number of datagrams received with errors. */
                MIB2_udpInErrors_Inc;

                return (2);
            }

            /* put it back */
            PUT16(buf_ptr->data_ptr, UDP_CHECK_OFFSET, hischeck);
        }
    }

    /* Did we want this data ?  If not, then let it go, no comment
     * If we want it, copy the relevant information into our structure
     */
    uptr = NU_NULL;
    dest_port = GET16(buf_ptr->data_ptr, UDP_DEST_OFFSET);
    source_port = GET16(buf_ptr->data_ptr, UDP_SRC_OFFSET);

    /* LONGSWAP the destination address and store it in a local so we
     * don't have to perform the LONGSWAP every time through the loop.
     */
    dest_addr = LONGSWAP(tcp_chk->dest);

#if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE))
    /* Set a UDPLAYER ptr to the UDP header info */
    udp_info = (UDPLAYER *)(buf_ptr->data_ptr);

    /* Print the UDP header info */
    NLOG_UDP_Info(udp_info, NLOG_RX_PACK);
#endif

    /* Save the original buffer pointer if an error occurs. */
    saved_ptr = buf_ptr->data_ptr;

    /* Set the data pointer to the beginning of the IP header so
     * the source and destination addresses can be extracted later
     * and a pointer to the IP header can be saved if ancillary
     * data has been requested by the application.
     */
    buf_ptr->data_ptr = (UINT8 HUGE*)pkt;

    for (i = 0; i < UDP_MAX_PORTS; i++)
    {
        if ( (UDP_Ports[i]) && (dest_port == UDP_Ports[i]->up_lport) )
        {
            /* Store off a pointer to the socket to simplify the below
             * complex conditional statement.
             */
            sockptr = SCK_Sockets[UDP_Ports[i]->up_socketd];

            local_addr = IP_ADDR(sockptr->s_local_addr.ip_num.is_ip_addrs);

            foreign_addr = IP_ADDR(sockptr->s_foreign_addr.ip_num.is_ip_addrs);

            /* Check to make sure this entry in the UDP_Ports actually points to a
             * port structure, and check for the destination port matching the local
             * port.  Short circuit evaluation will cause the test to fail
             * immediately if the pointer to the port structure is NULL.
             */
            if ( ((sockptr->s_family == SK_FAM_IP) &&

                  ( ((!(sockptr->s_state & SS_ISCONNECTED)) &&
                   ((local_addr == dest_addr) || (local_addr == IP_ADDR_ANY))) ||

                  ((sockptr->s_state & SS_ISCONNECTED) &&
                   (source_port == UDP_Ports[i]->up_fport) &&
                   ((local_addr == dest_addr) || (local_addr == IP_ADDR_ANY)) &&
                   (foreign_addr == LONGSWAP(tcp_chk->source)))
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
                   ||
                   ((buf_ptr->mem_flags & NET_MCAST) &&
                    (sockptr->s_moptions_v4 != NU_NULL))
#endif
                        ) )

#if (INCLUDE_IPV6 == NU_TRUE)
                  ||
                  ((sockptr->s_family == SK_FAM_IP6) &&
                   (!(sockptr->s_options & SO_IPV6_V6ONLY)) &&

                   (((!(sockptr->s_state & SS_ISCONNECTED)) &&
                     ((IPV6_IS_ADDR_UNSPECIFIED(sockptr->
                                                s_local_addr.ip_num.is_ip_addrs)) ||
                      ((IPV6_IS_ADDR_V4MAPPED(sockptr->
                                              s_local_addr.ip_num.is_ip_addrs)) &&
                       (IP_ADDR(&(sockptr->
                                s_local_addr.ip_num.is_ip_addrs[12])) == dest_addr)))) ||

                   ((sockptr->s_state & SS_ISCONNECTED) &&
                    (source_port == UDP_Ports[i]->up_fport) &&
                    ((IPV6_IS_ADDR_V4MAPPED(sockptr->
                                            s_local_addr.ip_num.is_ip_addrs)) &&
                     ((IP_ADDR(&(sockptr->
                               s_local_addr.ip_num.is_ip_addrs[12])) == dest_addr) &&
                      (IP_ADDR(&(sockptr->
                               s_foreign_addr.ip_num.is_ip_addrs[12])) ==
                                            LONGSWAP(tcp_chk->source))))))

                     ) )
#else
                              )
#endif
            {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
                if (!(buf_ptr->mem_flags & NET_MCAST))
#endif
                {
                    /* If the packet is destined for a broadcast address, deliver a
                     * copy to all tasks.
                     */
                    if (buf_ptr->mem_flags & NET_BCAST)
                    {
                        uptr = UDP_Ports[i];
                        UDP_Interpret(buf_ptr, UDP_Ports[i], (INT)i, NU_TRUE);
                    }

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

                    /* If the SO_REUSEADDR socket option has been set on a socket,
                     * check for a specific match before accepting a WILDCARD match.
                     */
                    else if ( (SCK_ReuseAddr_Set) && (
#if (INCLUDE_IPV4 == NU_TRUE)
                         ((sockptr->s_family == SK_FAM_IP) &&
                          (local_addr == IP_ADDR_ANY))
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                          ||
                         ((sockptr->s_family == SK_FAM_IP6) &&
                          (IPV6_IS_ADDR_UNSPECIFIED(sockptr->
                                                    s_local_addr.ip_num.is_ip_addrs)))
#endif
                       ) )
                    {
                        saved_i = (INT)i;
                    }
#endif

                    /* When an exact match is found our search is over. */
                    else
                    {
                        uptr = UDP_Ports[i];

                        /* When a unicast message is received, the message should
                         * be delivered to the best matching socket, not all
                         * sockets.  That is why only the unicast task receives
                         * this message.
                         */
                        UDP_Interpret(buf_ptr, UDP_Ports[i], (INT)i, NU_FALSE);

                        break;
                    }
                }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
                /* Verify that the socket is actually a member of the
                 * multicast group to which this message was sent.
                 */
                else if (sockptr->s_moptions_v4)
                {
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
                    moptions = sockptr->s_moptions_v4;

                    /* Convert the multicast address from UINT32 into
                     * an array of UINT8's
                     */
                    PUT32(multi_dest, 0, dest_addr);

                    /* Get the device state for the multicast address
                     * that the message was sent to
                     */
                    igmp_state =
                        Multi_Get_Sck_State_Struct(multi_dest,
                                                   moptions,
                                                   buf_ptr->mem_buf_device,
                                                   UDP_Ports[i]->up_socketd,
                                                   NU_FAMILY_IP);

                    if (igmp_state != NU_NULL)
                    {
                        /* Get the source address in a UINT8 array
                         * format
                         */
                        PUT32(multi_src, 0, GET32(pkt, IP_SRC_OFFSET));

                        /* Now, see if we should accept the message
                         * coming from this source
                         */
                        if (Multi_Verify_Src_By_Filter(multi_src, igmp_state,
                                                       MULTICAST_VERIFY_SOCKET,
                                                       IP_ADDR_LEN) != MULTICAST_ACCEPT_SOURCE)
                        {
                            /* Drop the packet.  We do not want to
                             * receive anything from this src address.
                             */
                            MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                                  &MEM_Buffer_Freelist);

                            MIB2_ipInAddrErrors_Inc;

                            return (2);
                        }

                        else
                        {
                            uptr = UDP_Ports[i];

                            /* Deliver a copy of this packet to the UDP layer. */
                            UDP_Interpret(buf_ptr, UDP_Ports[i], (INT)i, NU_TRUE);
                        }
                    }
#else
                    uptr = UDP_Ports[i];

                    /* Deliver a copy of this packet to the UDP layer. */
                    UDP_Interpret(buf_ptr, UDP_Ports[i], (INT)i, NU_TRUE);
#endif
                }
#endif
            }
        }
    }  /* end for i =0 to UDP_MAX_PORTS*/

    /*  If we did not find a port then we are not waiting for this
     *  so return.
     */
    if (uptr == NU_NULL)
    {
        /* If a WILDCARD match was found */
        if (saved_i != -1)
        {
            /* Deliver this packet to the UDP layer. */
            return (UDP_Interpret(buf_ptr, UDP_Ports[saved_i], (INT)saved_i,
                                  NU_FALSE));
        }

        /* Increment the number of datagrams received for which there was no
         * port.
         */
        else
        {
            MIB2_udpNoPorts_Inc;

            /* Restore the buffer pointer to the head of the UDP header. */
            buf_ptr->data_ptr = saved_ptr;

            return (NU_DEST_UNREACH_PORT);
        }
    }

    else
    {
        /* Free the original buffers held by this packet since UDP_Interpret
         * made a copy of the buffer to return to each socket waiting for the
         * data.
         */
        if (i == UDP_MAX_PORTS)
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        return (NU_SUCCESS);
    }

} /* UDP4_Input */

/*************************************************************************
*
*   FUNCTION
*
*       UDP4_Cache_Route
*
*   DESCRIPTION
*
*       Cache a route for an IPv4 UDP socket.
*
*   INPUTS
*
*       *uprt                   A pointer to the UDP Port list entry.
*       ip_addr                 The destination IP address.
*
*   OUPUTS
*
*       NU_SUCCESS
*       NU_HOST_UNREACHABLE
*
*************************************************************************/
STATUS UDP4_Cache_Route(UDP_PORT *uprt, UINT32 ip_addr)
{
    RTAB_ROUTE      *ro;
    UINT32          search_addr = ip_addr;

    /* Point to the cached route */
    ro = &uprt->up_route;

    /* If the destination is the limited broadcast address, update it to the
     * broadcast address for the primary network interface. This is for the
     * purposes of route discovery below.
     */
    if (search_addr == IP_ADDR_BROADCAST)
    {
        /* default bcast route? */
        if ( (uprt->up_socketd >= 0) &&
             (SCK_Sockets[uprt->up_socketd]->s_bcast_if) )

            /* Set the search addr */
            search_addr = SCK_Sockets[uprt->up_socketd]->
                                s_bcast_if->dev_addr.dev_net;
        else
        {
            /* Use the primary interface, skip the loopback */
            if ( (DEV_Table.dv_head->dev_type == DVT_LOOP) &&
                 (DEV_Table.dv_head->dev_next != NU_NULL) )
                search_addr = DEV_Table.dv_head->dev_next->dev_addr.dev_net;
            else
                search_addr = DEV_Table.dv_head->dev_addr.dev_net;
        }

        /* Free the cached route. The default IF for sending a
         * bcast packet may have change.
         */
        if (ro->rt_route)
        {
            RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);
            ro->rt_route = NU_NULL;
        }

    }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    /* If this is a multicast address then check for a default route. */
    else if (IP_MULTICAST_ADDR(search_addr))
    {
        /* default mcast route? */
        if ( (uprt->up_socketd >= 0) &&
             (SCK_Sockets[uprt->up_socketd]->s_moptions_v4) &&
             (SCK_Sockets[uprt->up_socketd]->s_moptions_v4->multio_device) )
        {
            /* Set the search addr */
            search_addr = SCK_Sockets[uprt->up_socketd]->
                          s_moptions_v4->multio_device->dev_addr.dev_net;

            /* Is there a cached route? */
            if (ro->rt_route)
            {

                /* Check the IF used by the cached route. If it is different
                 * than the set default IF free it so a new route can be found.
                 */
                if (SCK_Sockets[uprt->up_socketd]->s_moptions_v4->multio_device !=
                    ro->rt_route->rt_entry_parms.rt_parm_device)
                {
                    RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);
                    ro->rt_route = NU_NULL;
                }
            }
        }
    }

#endif

    /* If there is already a route cached but the destination IP addresses
     * don't match, the route is not UP, or the device associated with the
     * route is not UP and RUNNING, free the route.
     */
    if ( (ro->rt_route) &&
         ((ro->rt_ip_dest.sck_addr != ip_addr) ||
          (!(ro->rt_route->rt_entry_parms.rt_parm_flags & RT_UP)) ||
          ((!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_RUNNING)) &&
           (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_UP)))) )
    {
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);
        ro->rt_route = NU_NULL;
    }

    /* If there is no cached route then try to find one. */
    if (ro->rt_route == NU_NULL)
    {
        ro->rt_ip_dest.sck_addr = search_addr;
        ro->rt_ip_dest.sck_family = SK_FAM_IP;
        IP_Find_Route(ro);
    }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    /* If no route was found and this is a multicast address then
     * just use the primary interface.
     */
    if ( (!ro->rt_route) && (IP_MULTICAST_ADDR(search_addr)) )
    {
        if ( (DEV_Table.dv_head->dev_type == DVT_LOOP) &&
             (DEV_Table.dv_head->dev_next != NU_NULL) )
            ro->rt_ip_dest.sck_addr = DEV_Table.dv_head->dev_next->dev_addr.
                                                            dev_net_brdcast;
        else
            ro->rt_ip_dest.sck_addr = DEV_Table.dv_head->dev_addr.
                                                                dev_net_brdcast;

        IP_Find_Route(ro);

    }

#endif

    /* If it was the limited broadcast address (255.255.255.255) or a
     * multicast address that a route was desired for then the route
     * address will be wrong. Go ahead and update it for all cases.
     */
    ro->rt_ip_dest.sck_addr = ip_addr;

    if (ro->rt_route)
    {
        return NU_SUCCESS;
    }
    else
        return (NU_HOST_UNREACHABLE);

} /* UDP4_Cache_Route */

/*************************************************************************
*
*   FUNCTION
*
*       UDP4_Find_Matching_UDP_Port
*
*   DESCRIPTION
*
*       This function returns the UDP port associated with the parameters
*       provided.
*
*   INPUTS
*
*       source_ip               The Source Address of the target entry.
*       dest_ip                 The Destination Address of the target
*                               entry.
*       source_port             The source port of the target entry.
*       dest_port               The destination port of the target entry.
*
*   OUTPUTS
*
*       The index of the corresponding port.
*       -1 if no port found.
*
*************************************************************************/
INT32 UDP4_Find_Matching_UDP_Port(UINT32 source_ip, UINT32 dest_ip,
                                  UINT16 source_port, UINT16 dest_port)
{
    INT         i;
    UDP_PORT    *prt;
    STATUS      status = -1;

    /* Search through the UDP_Ports for the port matching our Source and
     * Destination port and IP address.
     */
    for (i = 0; i < UDP_MAX_PORTS; i++)
    {
        prt = UDP_Ports[i];

        /* If this is the port we want, assign an error code */
        if ( (prt != NU_NULL) &&
             (SCK_Sockets[prt->up_socketd]->s_family == SK_FAM_IP) &&
             (prt->up_lport == source_port) && (prt->up_fport == dest_port) &&
             (prt->up_laddr == source_ip) && (prt->up_faddr == dest_ip) )
        {
            status = i;
            break;
        }
    }

    return (status);

} /* UDP4_Find_Matching_UDP_Port */

/*************************************************************************
*
*   FUNCTION
*
*       UDP4_Free_Cached_Route
*
*   DESCRIPTION
*
*       This function invalidates the cached route for all sockets
*       using the specified route.
*
*   INPUTS
*
*       rt_entry                The route that is being deleted and
*                               therefore must not be used by any UDP
*                               ports in the future.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID UDP4_Free_Cached_Route(RTAB4_ROUTE_ENTRY *rt_entry)
{
    UINT16  i;

    /* Invalidate all cached routes using this interface on a UDP
     * socket
     */
    for (i = 0; i < UDP_MAX_PORTS; i++)
    {
        /* If this port is in use by a socket. */
        if (UDP_Ports[i])
        {
            /* If the cached route for this port is the route that is
             * being deleted.
             */
            if (UDP_Ports[i]->up_route.rt_route == rt_entry)
            {
                /* Free the route. */
                RTAB_Free((ROUTE_ENTRY*)UDP_Ports[i]->up_route.rt_route,
                          NU_FAMILY_IP);

                /* Set the cached route to NU_NULL.  The next time data
                 * is transmitted using this port, a new route will be
                 * found.
                 */
                UDP_Ports[i]->up_route.rt_route = NU_NULL;
            }
        }
    }

} /* UDP4_Free_Cached_Route */

#endif
