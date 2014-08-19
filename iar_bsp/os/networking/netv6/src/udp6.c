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
*       udp6.c                                       
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       UDP Protocol routines for IPv6 packets.
*                                                                         
*   DATA STRUCTURES                                                       
*                                                                         
*       None
*                                                                         
*   FUNCTIONS                                                             
*                                                                         
*       UDP6_Input
*       UDP6_Cache_Route
*       UDP6_Find_Matching_UDP_Port
*       UDP6_Free_Cached_Route
*                                                                         
*   DEPENDENCIES                                                          
*                                                                         
*       target.h
*       mem_defs.h
*       udp.h
*       udp6.h
*       externs6.h
*       mld6.h                                                         
*                                                                         
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/udp6.h"
#include "networking/externs6.h"

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
#include "networking/mld6.h"
#endif

extern INT SCK_ReuseAddr_Set;

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UDP6_Cache_Route                                                  
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Cache a route for a UDPv6 socket.                                  
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *uprt                   A pointer to the port structure.
*       *dest_addr              A pointer to the destination address for
*                               which to find a route.
*                                                                       
*   OUTPUTS                                                                
*                                                                       
*       NU_SUCCESS              A route was found.
*       NU_HOST_UNREACHABLE     No route exists.
*                                                                       
*************************************************************************/
STATUS UDP6_Cache_Route(UDP_PORT *uprt, UINT8 *dest_addr)
{
    RTAB6_ROUTE     *ro;
    UINT8           *search_addr = dest_addr;
    STATUS          status;

    /* Set a pointer to the cached route */
    ro = &uprt->up_routev6;

    /* If this is a multicast address, check if the application layer set
     * the multicast interface on this socket.
     */
    if (IPV6_IS_ADDR_MULTICAST(search_addr))
    {
        /* If a multicast interface was specified, find a route through
         * that interface. 
         */
        if ( (uprt->up_socketd >= 0) &&
             (SCK_Sockets[uprt->up_socketd]->s_moptions_v6) &&
             (SCK_Sockets[uprt->up_socketd]->s_moptions_v6->multio_device) )
        {
            /* If the current cached route does not use the multicast 
             * interface set by the application, free the route and set a
             * new cached route.  Otherwise, continue to use the cached route.
             */
            if ( (!ro->rt_route) ||
                 (SCK_Sockets[uprt->up_socketd]->s_moptions_v6->multio_device !=
                  ro->rt_route->rt_entry_parms.rt_parm_device) )
            {
                /* Free the previously cached route */
                RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);

                /* Set the new cached route */
                ro->rt_route = 
                    (RTAB6_ROUTE_ENTRY*)
                    RTAB6_Find_Route_By_Device(SCK_Sockets[uprt->up_socketd]->
                        s_moptions_v6->multio_device->dev6_addr_list.dv_head->
                        dev6_ip_addr, SCK_Sockets[uprt->up_socketd]->s_moptions_v6->
                        multio_device);
            }

            /* Return the route or an error if a route could not be found. */
            if (ro->rt_route)
                return (NU_SUCCESS);
            else
                return (NU_HOST_UNREACHABLE);
        }
    }

    /* If there is already a route cached but the destination IP addresses
       don't match then free the route. This comparison is done to the real
       IP address not the search address, they will only be different when a
       route for the limited broadcast address is desired. */
    if  (ro->rt_route && 
        ((memcmp(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr, dest_addr, 
                 IP6_ADDR_LEN) != 0) ||
         (!(ro->rt_route->rt_entry_parms.rt_parm_flags & RT_UP)) ||
         ((!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_RUNNING)) &&
         (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_UP)) &&
         (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags2 & DV6_UP)))))
    {
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);
        ro->rt_route = NU_NULL;
    }

    /* If there is no cached route then try to find one. */
    if (ro->rt_route == NU_NULL)
    {
        NU_BLOCK_COPY(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr, search_addr, 
                      IP6_ADDR_LEN);

        ro->rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;
        IP6_Find_Route(ro);

        if (ro->rt_route)
            status = NU_SUCCESS;
        else
            status = NU_HOST_UNREACHABLE;
    }    
    else
        status = NU_SUCCESS;

    return (status);

} /* UDP6_Cache_Route */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UDP6_Input                                                  
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function verifies the UDP checksum of an IPv6 UDP packet,
*       and forwards the packet to the UDP_Interpret routine according
*       to the following table:
*
*       -------------------------------------------------------------------
*       INCOMING PACKET TYPE               |Multicast | Broadcast| Unicast
*       -------------------------------------------------------------------
*       TASK 1 - bound to IP_ADDR_ANY      | RECEIVES | RECEIVES |  ---
*       -------------------------------------------------------------------
*       TASK 2 - bound to IP_ADDR_ANY      | RECEIVES | RECEIVES |  ---
*       -------------------------------------------------------------------
*       TASK 3 - bound to unicast IP addr  | RECEIVES |  ---     | RECEIVES
*       -------------------------------------------------------------------
*                                                                       
*   INPUTS                                                                
*                                                         
*       *pkt                    A pointer to the IPv6 header.
*       *buf_ptr                A pointer to the packet.
*       *pseudoheader           A pointer to the data structure holding 
*                               the pseudoheader information on which to 
*                               compute the checksum.           
*                                                                       
*   OUTPUTS                                                                
*                                
*       2
*       -1
*       NU_SUCCESS                                       
*                                                                       
*************************************************************************/
STATUS UDP6_Input(IP6LAYER *pkt, NET_BUFFER *buf_ptr, 
                  struct pseudohdr *pseudoheader)
{
    UINT16              checksum;
    UINT16              dest_port, source_port;
    UINT16              i;
    INT                 saved_i = -1;
    UDP_PORT            *uptr;

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) &&             \
      (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY) )
    MULTI_SCK_STATE     *mld_sck_state;
    MULTI_SCK_OPTIONS   *moptions;    
#endif

    /* Verify the UDP Checksum */
    /* If the checksum was computed in the controller bypass the software 
     * checksum efforts.
     */
#if (HARDWARE_OFFLOAD == NU_TRUE)
    if (!(buf_ptr->hw_options & HW_RX_UDP6_CHKSUM))
#endif
    {
        checksum = GET16(buf_ptr->data_ptr, UDP_CHECK_OFFSET);

        if ( (!checksum) || (TLS6_Prot_Check((UINT16 *)pseudoheader, buf_ptr)) )
        {
            /* The Checksum failed log an error and drop the packet. */
            NLOG_Error_Log("UDP Checksum invalid for incoming packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (2);
        }       
    }

    /* Did we want this data ?  If not, then let it go, no comment
     * If we want it, copy the relevant information into our structure
     */
    uptr = NU_NULL;
    dest_port = GET16(buf_ptr->data_ptr, UDP_DEST_OFFSET);
    source_port = GET16(buf_ptr->data_ptr, UDP_SRC_OFFSET);

    /* Set the data pointer to the beginning of the IP header so
     * the source and destination addresses can be extracted later
     * and a pointer to the IP header can be saved if ancillary
     * data has been requested by the application.
     */
    buf_ptr->data_ptr = (UINT8 HUGE*)pkt;

    for (i = 0; i < UDP_MAX_PORTS; i++)
    {
        /* Check to make sure this entry in the UDP_Ports actually points to a
         * port structure, and check for the destination port matching the local
         * port.  Short circuit evaluation will cause the test to fail
         * immediately if the pointer to the port structure is NULL.
         */
        if ( (UDP_Ports[i]) && (dest_port == UDP_Ports[i]->up_lport) &&
             (SCK_Sockets[UDP_Ports[i]->up_socketd]->s_family == SK_FAM_IP6) &&

             (((!(SCK_Sockets[UDP_Ports[i]->up_socketd]->s_state & SS_ISCONNECTED)) &&
               ((IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[UDP_Ports[i]->up_socketd]->
                                          s_local_addr.ip_num.is_ip_addrs)) ||
                (memcmp(SCK_Sockets[UDP_Ports[i]->up_socketd]->
                        s_local_addr.ip_num.is_ip_addrs, pseudoheader->dest, 
                        IP6_ADDR_LEN) == 0))) ||

             ((SCK_Sockets[UDP_Ports[i]->up_socketd]->s_state & SS_ISCONNECTED) &&
              (source_port == UDP_Ports[i]->up_fport) &&
              ((IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[UDP_Ports[i]->up_socketd]->
                                          s_local_addr.ip_num.is_ip_addrs)) ||
               (memcmp(SCK_Sockets[UDP_Ports[i]->up_socketd]->
                       s_local_addr.ip_num.is_ip_addrs, pseudoheader->dest, 
                       IP6_ADDR_LEN) == 0)) &&
              (memcmp(SCK_Sockets[UDP_Ports[i]->up_socketd]->s_foreign_addr.ip_num.
                      is_ip_addrs, pseudoheader->source, IP6_ADDR_LEN) == 0)) 
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
                                  ||
              ((buf_ptr->mem_flags & NET_MCAST) && 
               (SCK_Sockets[UDP_Ports[i]->up_socketd]->s_moptions_v6 != NU_NULL)) 
#endif                                        
                      ))
        {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
            if (!(buf_ptr->mem_flags & NET_MCAST))
#endif
            {
#if (INCLUDE_SO_REUSEADDR == NU_TRUE)
                /* If the SO_REUSEADDR socket option has been set on a socket, 
                 * check for a specific match before accepting a WILDCARD match.
                 */
                if ( (SCK_ReuseAddr_Set) &&
                     (IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[UDP_Ports[i]->up_socketd]->
                                               s_local_addr.ip_num.is_ip_addrs)) )
                {
                    saved_i = (INT)i;
                }

                /* When an exact match is found our search is over. */
                else
#endif
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
            else if (SCK_Sockets[UDP_Ports[i]->up_socketd]->s_moptions_v6)
            {
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
                moptions = SCK_Sockets[UDP_Ports[i]->up_socketd]->s_moptions_v6;

                /* Get the device state for the multicast address that 
                 * the message was sent to 
                 */
                mld_sck_state = 
                    Multi_Get_Sck_State_Struct(pkt->ip6_dest, moptions, 
                                               buf_ptr->mem_buf_device,
                                               UDP_Ports[i]->up_socketd, 
                                               NU_FAMILY_IP6);
                
                if (mld_sck_state != NU_NULL)
                {
                    /* Now, see if we should accept the message coming 
                     * from this source 
                     */
                    if (Multi_Verify_Src_By_Filter(pkt->ip6_src, 
                                                   mld_sck_state, 
                                                   MULTICAST_VERIFY_SOCKET, 
                                                   IP6_ADDR_LEN) != 
                                                   MULTICAST_ACCEPT_SOURCE)
                    {
                        /* Drop the packet.  We do not want to receive 
                         * messages from this src address.
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
    }  /* end for i =0 to UDP_MAX_PORTS */

    /* If we did not find a port then we are not waiting for this, so return. */
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

            /* Free the buffer chain. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

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

} /* UDP6_Input */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UDP6_Find_Matching_UDP_Port                                                     
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function returns the UDP port associated with the parameters
*       provided.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *source_ip              A pointer to the Source Address of the 
*                               target entry.
*       *dest_ip                A pointer to the Destination Address of 
*                               the target entry.    
*       source_port             The source port of the target entry.
*       dest_port               The destination port of the target entry.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The index of the corresponding port.
*       -1 if no port found.
*                                                                       
*************************************************************************/
INT32 UDP6_Find_Matching_UDP_Port(const UINT8 *source_ip, const UINT8 *dest_ip, 
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
             (SCK_Sockets[prt->up_socketd]->s_family == SK_FAM_IP6) &&
             (prt->up_lport == source_port) && (prt->up_fport == dest_port) && 
             (memcmp(prt->up_laddrv6, source_ip, IP6_ADDR_LEN) == 0) && 
             (memcmp(prt->up_faddrv6, dest_ip, IP6_ADDR_LEN) == 0) )
        {
            status = i;
            break;
        }
    }

    return (status);

} /* UDP6_Find_Matching_UDP_Port */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UDP6_Free_Cached_Route                                                     
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
VOID UDP6_Free_Cached_Route(RTAB6_ROUTE_ENTRY *rt_entry)
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
            if (UDP_Ports[i]->up_routev6.rt_route == rt_entry)
            {
                /* Free the route. */
                RTAB_Free((ROUTE_ENTRY*)UDP_Ports[i]->up_routev6.rt_route, 
                          NU_FAMILY_IP6);

                /* Set the cached route to NU_NULL.  The next time data
                 * is transmitted using this port, a new route will be
                 * found.
                 */
                UDP_Ports[i]->up_routev6.rt_route = NU_NULL;
            }
        }
    }

} /* UDP6_Free_Cached_Route */
