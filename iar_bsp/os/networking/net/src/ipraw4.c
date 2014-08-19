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

/**************************************************************************
*
*   FILENAME
*
*       ipraw4.c
*
*   DESCRIPTION
*
*       IPRAW Protocol routines for IPv4.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IPRaw4_Interpret
*       IPRaw4_Cache_Route
*       IPRaw4_Free_Cached_Route
*
*   DEPENDENCIES
*
*       externs.h
*       ipraw.h
*       igmp.h
*
****************************************************************************/

#include "networking/externs.h"
#include "networking/ipraw.h"
#include "networking/igmp.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw4_Interpret
*
*   DESCRIPTION
*
*       Process received IP Raw datagrams.
*
*   INPUTS
*
*       *pkt                    A pointer to the IPv4 header.
*       *buf_ptr                A pointer to the NET buffer
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       -1                      There is no listener for this packet.
*
*************************************************************************/
STATUS IPRaw4_Interpret(IPLAYER *pkt, NET_BUFFER *buf_ptr)
{
    INT             i;
    UINT32          local_addr;
    UINT16          hlen;
    STATUS          status = -1;

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) &&       \
      (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
    MULTI_SCK_STATE         *igmp_state;
    MULTI_SCK_OPTIONS       *moptions;
    UINT8                   multi_dest[IP_ADDR_LEN];
    UINT8                   multi_src[IP_ADDR_LEN];
#endif

    /* Pull the current buffer chain off of the receive list. */
    MEM_Buffer_Dequeue (&MEM_Buffer_List);

    /* The data pointer was advanced past the IP header prior to entering
     * this function, move it back to point at the IP header so that
     * we can peek into it and pass it up to the application if a matching
     * process is found.
     */
    buf_ptr->data_ptr = (UINT8 HUGE*)pkt;

    /* Extract the header length from the IP header */
    hlen = (UINT16)((GET8(buf_ptr->data_ptr, IP_VERSIONANDHDRLEN_OFFSET)
                    & 0x0f) << 2);

    /* Add the header length back onto the length of the buffer chain */
    buf_ptr->data_len += hlen;
    buf_ptr->mem_total_data_len += hlen;

    /* Did we want this data ?  If not, then let it go, no comment
     * If we want it, copy the relevant information into our structure
     */
    for (i = 0; i < IPR_MAX_PORTS; i++)
    {
        /* Check to make sure this entry in the IPR_Ports actually points to a
         * port structure, and check to see if the destination protocol
         * matches the local protocol.  Short circuit evaluation will cause
         * the test to fail immediately if the pointer to the port structure is NULL.
         */
        if ( (IPR_Ports[i]) &&
             ((GET8(pkt, IP_PROTOCOL_OFFSET) == (UINT8)(IPR_Ports[i]->ip_protocol)) ||
              (IPR_Ports[i]->ip_protocol == 0)) )
        {
            local_addr = IP_ADDR(SCK_Sockets[IPR_Ports[i]->ip_socketd]->
                                 s_local_addr.ip_num.is_ip_addrs);

            /* Compare local address with destination address */
            if  ( ((SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_family == SK_FAM_IP) &&
                   ((local_addr == IP_ADDR_ANY) ||
                    (GET32(pkt, IP_DEST_OFFSET) == local_addr)))

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) && (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
                    ||
                    ((buf_ptr->mem_flags & NET_MCAST) &&
                    (SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_moptions_v4 != NU_NULL))
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                    ||
                  ((SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_family == SK_FAM_IP6) &&
                   (!(SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_options & SO_IPV6_V6ONLY)) &&
                   ((IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[IPR_Ports[i]->ip_socketd]->
                                              s_local_addr.ip_num.is_ip_addrs)) ||
                    ((IPV6_IS_ADDR_V4MAPPED(SCK_Sockets[IPR_Ports[i]->ip_socketd]->
                                            s_local_addr.ip_num.is_ip_addrs)) &&
                     (IP_ADDR(&(SCK_Sockets[IPR_Ports[i]->ip_socketd]->
                              s_local_addr.ip_num.is_ip_addrs[12])) ==
                              GET32(pkt, IP_DEST_OFFSET)))))
#endif
                    )
            {
#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) && (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
                /* Verify that the socket is actually a member of the
                 * multicast group to which this message was sent.
                 */
                if ( (buf_ptr->mem_flags & NET_MCAST) &&
                     (SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_moptions_v4) )
                {
                    moptions = SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_moptions_v4;

                    /* Convert the multicast address from UINT32 into
                     * an array of UINT8's
                     */
                    PUT32(multi_dest, 0, pkt->ip_dest);

                    /* Get the device state for the multicast address
                     * that the message was sent to
                     */
                    igmp_state =
                        Multi_Get_Sck_State_Struct(multi_dest,
                                moptions, buf_ptr->mem_buf_device,
                                IPR_Ports[i]->ip_socketd,
                                NU_FAMILY_IP);

                    if (igmp_state != NU_NULL)
                    {
                        /* Get the source address in a UINT8 array
                         * format
                         */
                        PUT32(multi_src, 0, pkt->ip_src);

                        /* Now, see if we should accept the message
                         * coming from this source
                         */
                        if (Multi_Verify_Src_By_Filter(multi_src, igmp_state,
                                            MULTICAST_VERIFY_SOCKET,
                                            IP_ADDR_LEN) !=
                                            MULTICAST_ACCEPT_SOURCE)
                        {
                            /* Drop the packet.  We do not want to
                             * receive anything from this src address.
                             */
                            MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                                &MEM_Buffer_Freelist);
                            return (-1);
                        }
                    }
                }
#endif
                /* Deliver a copy of the data to this socket */
                status = IPRaw_Interpret(buf_ptr, i);
            }
        }
    }  /* end for i =0 to IPR_MAX_PORTS */

    /* Free the original buffers held by this packet since IPRaw_Interpret
     * made a copy of the buffer to return to each socket waiting for the
     * data.
     */
    MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

    return (status);

} /* IPRaw4_Interpret */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw4_Cache_Route
*
*   DESCRIPTION
*
*       Cache a route for an IPv4 socket.
*
*   INPUTS
*
*       *iprt                   A pointer to the IPRAW port structure.
*       ip_addr                 The destination IP address.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_HOST_UNREACHABLE     No route to the destination.
*
*************************************************************************/
STATUS IPRaw4_Cache_Route(struct iport *iprt, UINT32 ip_addr)
{
    RTAB_ROUTE      *ro;
    UINT32          search_addr = ip_addr;

    /* If the destination is the limited broadcast address, update it to the
     * broadcast address for the primary network interface. This is for the
     * purposes of route discovery below.
     */
    if (search_addr == IP_ADDR_BROADCAST)
        search_addr = DEV_Table.dv_head->dev_addr.dev_net_brdcast;

    ro = &iprt->ipraw_route;

    /* If there is already a route cached but the destination IP addresses
     * don't match then free the route. This comparison is done to the real
     * IP address not the search address, they will only be different when a
     * route for the limited broadcast address is desired.
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

    /* If it was the limited broadcast address (255.255.255.255) that a
     * route was desired for then the route address will be wrong. Go ahead
     * and update it for all cases.
     */
    ro->rt_ip_dest.sck_addr = ip_addr;

    if (ro->rt_route)
        return (NU_SUCCESS);
    else
        return (NU_HOST_UNREACHABLE);

} /* IPRaw4_Cache_Route */

/*************************************************************************
*
*   FUNCTION
*
*       IPRaw4_Free_Cached_Route
*
*   DESCRIPTION
*
*       This function invalidates the cached route for all sockets
*       using the specified route.
*
*   INPUTS
*
*       rt_entry                The route that is being deleted and
*                               therefore must not be used by any RAW IP
*                               ports in the future.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IPRaw4_Free_Cached_Route(RTAB4_ROUTE_ENTRY *rt_entry)
{
    UINT16  i;

    /* Invalidate all cached routes using this interface on a RAW IP
     * socket
     */
    for (i = 0; i < IPR_MAX_PORTS; i++)
    {
        /* If this port is in use by a socket. */
        if (IPR_Ports[i])
        {
            /* If the cached route for this port is the route that is
             * being deleted.
             */
            if (IPR_Ports[i]->ipraw_route.rt_route == rt_entry)
            {
                /* Free the route. */
                RTAB_Free((ROUTE_ENTRY*)IPR_Ports[i]->ipraw_route.rt_route,
                          NU_FAMILY_IP);

                /* Set the cached route to NU_NULL.  The next time data
                 * is transmitted using this port, a new route will be
                 * found.
                 */
                IPR_Ports[i]->ipraw_route.rt_route = NU_NULL;
            }
        }
    }

} /* IPRaw4_Free_Cached_Route */

#endif
