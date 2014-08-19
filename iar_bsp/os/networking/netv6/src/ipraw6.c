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
*       ipraw6.c                                     
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       IPRAW Protocol routines for IPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*              
*       None                                                            
*                                                                          
*   FUNCTIONS                                                                
*                                                                          
*       IPRaw6_Interpret 
*       IPRaw6_Cache_Route  
*       IPRaw6_Free_Cached_Route                                                      
*                                                                          
*   DEPENDENCIES                                                             
*                                    
*       externs.h
*       ipraw.h                                      
*       ipraw6.h                                      
*       externs6.h
*       mld6.h    
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/ipraw.h"
#include "networking/ipraw6.h"
#include "networking/externs6.h"
#include "networking/mld6.h"

/***********************************************************************
*                                                                       
*   FUNCTIONS
*
*       IPRaw6_Interpret                                                  
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Process received IP Raw datagrams.                               
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *pkt                    A pointer to the IPv6 header.
*       buf_ptr                 A pointer to the incoming buffer.
*       *pseudoheader           A pointer to the data structure holding 
*                               the pseudoheader information on which to 
*                               compute the checksum. 
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The packet was successfully passed to 
*                               the upper layer.
*       -1                      The incoming packet is not destined for
*                               a port on the node.
*                                                                       
*************************************************************************/
STATUS IPRaw6_Interpret(IP6LAYER *pkt, NET_BUFFER *buf_ptr, 
                        struct pseudohdr *pseudoheader, UINT8 next_header)
{
    INT     i;
    UINT16  checksum;
    STATUS  status = -1;

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) &&             \
    (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY) )
    MULTI_SCK_STATE     *mld_sck_state;
    MULTI_SCK_OPTIONS   *moptions;    
#endif

    /* Pull the current buffer chain off of the receive list. */
    MEM_Buffer_Dequeue (&MEM_Buffer_List);

    /* Did we want this data ?  If not, then let it go, no comment
     * If we want it, copy the relevant information into our structure
     */
    for (i = 0; i < IPR_MAX_PORTS; i++)
    {
        /* Check to make sure this entry in the iportlist actually points 
         * to a port structure, and check to see if the destination 
         * protocol matches the local protocol.  Short circuit evaluation 
         * will cause the test to fail immediately if the pointer to the 
         * port structure is NULL.
         */ 
        if ( (IPR_Ports[i]) && 
             (SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_family == SK_FAM_IP6) &&
             ((next_header == (UINT8)IPR_Ports[i]->ip_protocol) ||
              (IPR_Ports[i]->ip_protocol == 0)) &&
              (((IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[IPR_Ports[i]->ip_socketd]->
                                         s_local_addr.ip_num.is_ip_addrs)) ||
               (memcmp(pkt->ip6_dest, 
                       SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_local_addr.
                       ip_num.is_ip_addrs, IP6_ADDR_LEN) == 0))
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
                       ||
              ((buf_ptr->mem_flags & NET_MCAST) && 
               (SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_moptions_v6 != NU_NULL)) 
#endif                           
                    ) )
        {
            /* Only perform the following once */
            if (status == -1)
            {
                /* If the IPV6_CHECKSUM socket option was set on this socket, 
                 * verify the checksum in the packet.  Only verify the checksum 
                 * once - not each time through the loop.
                 */
                if (SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_options & SO_IPV6_CHECKSUM)
                {
                    /* Verify the RAW Checksum */
                    checksum = GET16(buf_ptr->data_ptr, IPR_Ports[i]->ip_chk_off);

                    if ( (!checksum) || 
                         (TLS6_Prot_Check((UINT16 *)pseudoheader, buf_ptr)) )
                    {
                        /* The Checksum failed log an error and drop the packet. */
                        NLOG_Error_Log("RAW Checksum invalid for incoming packet", 
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                        return (-1);
                    }
                }
            }
#if (INCLUDE_IP_MULTICASTING == NU_TRUE) 
            /* Verify that the socket is actually a member of the 
             * multicast group to which this message was sent.
             */
            if ( (buf_ptr->mem_flags & NET_MCAST) && 
                (SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_moptions_v6) )
            {
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
                moptions = SCK_Sockets[IPR_Ports[i]->ip_socketd]->s_moptions_v6;

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
                                           
                        return (-1);
                    }
                }
#endif
            }
#endif

            /* Set the data pointer to the beginning of the IP header so
             * the source and destination addresses can be extracted later
             * and a pointer to the IP header can be saved if ancillary
             * data has been requested by the application.
             */
            buf_ptr->data_ptr = (UINT8 HUGE*)pkt;

            /* Add the header length back onto the length of the buffer chain */
            buf_ptr->data_len += 
                IP6_HEADER_LEN + 
                (GET16(pkt, IP6_PAYLEN_OFFSET) - buf_ptr->mem_total_data_len);

            /* Deliver a copy of the data to this socket */
            status = IPRaw_Interpret(buf_ptr, i);
        }
    }  /* end for i =0 to IPR_MAX_PORTS */

    /* Free the original buffers held by this packet since IPRaw_Interpret
     * made a copy of the buffer to return to each socket waiting for the
     * data.
     */
    MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

    return (status);

} /* IPRaw6_Interpret */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IPRaw6_Cache_Route                                                
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Cache a route for an IPv6 socket.                                  
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *iprt                   A pointer to the IPRAW port structure.
*       *ip_addr                A pointer to the destination IP address
*                               for which to find the route.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              A route was successfully found.
*       NU_HOST_UNREACHABLE     A route does not exist.
*                                                                       
*************************************************************************/
STATUS IPRaw6_Cache_Route(struct iport *iprt, const UINT8 *ip_addr)
{
    RTAB6_ROUTE *ro;
    STATUS      status;

    ro = &iprt->ipraw_routev6;

    /* If there is already a route cached but the destination IP addresses 
     * don't match then free the route. This comparison is done to the real 
     * IP address not the search address. 
     */
    if  ( (ro->rt_route) && 
          ((memcmp(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr, ip_addr, 
                   IP6_ADDR_LEN) != 0) || 
           (!(ro->rt_route->rt_entry_parms.rt_parm_flags & RT_UP)) ||
           ((!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_RUNNING)) &&
           (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_UP)) &&
           (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags2 & DV6_UP)))) )
    {
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);
        ro->rt_route = NU_NULL;
    }

    /* If there is no cached route then try to find one. */
    if (ro->rt_route == NU_NULL)
    {
        NU_BLOCK_COPY(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr, ip_addr, 
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

} /* IPRaw6_Cache_Route */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IPRaw6_Free_Cached_Route                                                     
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
VOID IPRaw6_Free_Cached_Route(RTAB6_ROUTE_ENTRY *rt_entry)
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
            if (IPR_Ports[i]->ipraw_routev6.rt_route == rt_entry)
            {
                /* Free the route. */
                RTAB_Free((ROUTE_ENTRY*)IPR_Ports[i]->ipraw_routev6.rt_route, 
                          NU_FAMILY_IP6);

                /* Set the cached route to NU_NULL.  The next time data
                 * is transmitted using this port, a new route will be
                 * found.
                 */
                IPR_Ports[i]->ipraw_routev6.rt_route = NU_NULL;
            }
        }
    }

} /* IPRaw6_Free_Cached_Route */
