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
*       nd6rdrct.c                                   
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process and send
*       an ICMPv6 Redirect Message.
*                                                                          
*   DATA STRUCTURES                                                          
*                                  
*       None    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6RDRCT_Input
*                                                                          
*   DEPENDENCIES                                                             
*           
*       externs.h
*       nd6opts.h
*       nd6.h
*       nc6.h    
*       nud6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/nd6opts.h"
#include "networking/nd6.h"
#include "networking/nc6.h"
#include "networking/nud6.h"

extern  TQ_EVENT    IP6_Resolve6_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RDRCT_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function processes an incoming ICMPv6 Redirect according
*       to the specifications in RFC 4861 section 8.
*                                                                         
*   INPUTS                                                                
*                        
*       *pkt                    A pointer to the IP header.    
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       *buf_ptr                A pointer to the buffer.
*
*   PACKET FORMAT
*
*   0                   1                   2
*   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |      Type     |     Code      |            Checksum           |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                           Reserved                            |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +                        Target Address                         +
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +                      Destination Address                      +
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |    Options ...
*  +-+-+-+-+-+-+-+-+-+-
*                              
*   OUTPUTS                                                               
*                                           
*       0                              
*
*************************************************************************/
STATUS ND6RDRCT_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                      const NET_BUFFER *buf_ptr)
{
    UINT8                           *link_addr = NU_NULL;
    UINT8                           *target_addr, *dest_addr;
    union   nd_opts                 ndopts;
    UINT32                          icmp6len = buf_ptr->mem_total_data_len;
    RTAB6_ROUTE_ENTRY               *rt_entry;
    IP6_NEIGHBOR_CACHE_ENTRY        *nc_entry;
    UINT32                          flags = 0;
    UINT32                          route_flags;

    /* Get the length of the options */
    icmp6len -= IP6_REDIRECT_SIZE;

    /* Validate the Neighbor Advertisement - if invalid, silently discard
     * the packet.
     */
    if (ND6_Validate_Message(ICMP6_REDIRECT, pkt->ip6_src, NU_NULL, 
                             NU_NULL, pkt, icmp6len, IP6_REDIRECT_SIZE,
                             &ndopts, buf_ptr) == -1)
    {
        MIB_ipv6IfIcmpInErrors_Inc(device);

        NLOG_Error_Log("Redirect packet silently discarded", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (0);
    }

    /* Get a pointer to the Target Address */
    target_addr = (UINT8 *)(&buf_ptr->data_ptr[IP6_ICMP_REDIRECT_TRGT_ADDRS_OFFSET]);

    /* RFC 4861 section 8.3 - If the Target Address is not the same as
     * the Destination Address, the host must set IsRouter to TRUE for 
     * the target.
     */
    if (memcmp(target_addr, 
               &buf_ptr->data_ptr[IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET], 
               IP6_ADDR_LEN) != 0)
        flags = NC_ISROUTER;

    /* If a Target Link-Local Address option is present, extract the
     * link-local address and the length.
     */
    if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
         (ndopts.nd_opt_array[IP6_ICMP_OPTION_TAR_ADDR]) )         
        link_addr = (UINT8*)((UNSIGNED)(ndopts.nd_opt_array[IP6_ICMP_OPTION_TAR_ADDR]) +
                    IP6_ICMP_LL_OPTION_ADDRESS_OFFSET);

    /* Get a pointer to the Neighbor Cache entry associated with the
     * Target Address.
     */
    nc_entry = device->dev6_fnd_neighcache_entry(device, target_addr);

    /* RFC 4861 section 8.3 - If the redirect contains a Target Link-Layer address
     * option, the host either creates or updates the Neighbor Cache entry for
     * the target.  RFC 4861 - section 7.2 - It is possible that a host may
     * receive a ... Redirect message without a link-layer address option
     * included.  These messages MUST NOT create or update neighbor cache entries.
     */
    if (link_addr)
    {
        /* If a Neighbor Cache entry exists for the Target address, update its
         * parameters accordingly.
         */        
        if (nc_entry != NU_NULL)
        {
            /* If the isRouter flag is not set in the entry, and the new isRouter flag
             * is TRUE, set the entry to TRUE.
             */
            if (flags & NC_ISROUTER)
                NC6_Transition_To_Router(nc_entry);

            /* If the Neighbor Cache entry is in the INCOMPLETE state */
            if (nc_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE)
            {
                if (TQ_Timerunset(IP6_Resolve6_Event, TQ_CLEAR_EXACT, 
                                  nc_entry->ip6_neigh_cache_resolve_id, 
                                  (UNSIGNED)device->dev_index) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to stop the Address Resolution timer", 
                                   NERR_SEVERE, __FILE__, __LINE__);

                /* Record the Link-Layer address in the Neighbor Cache entry */
                device->dev6_update_neighcache_link_addr(nc_entry, link_addr);

                nc_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;

                /* Transmit the queued data awaiting address resolution. */
                ND6_Transmit_Queued_Data(nc_entry, device);
            }
        
            /* If the link-layer address in the entry is different from the
             * link-layer address in the packet, update the link-layer address in the
             * entry and set its state to STALE.
             */
            else if (device->dev6_neighcache_entry_equal(link_addr, nc_entry) != 0)
            {
                device->dev6_update_neighcache_link_addr(nc_entry, link_addr);

                /* If we are currently probing the neighbor, cancel probing
                 * now.
                 */
                if ( (nc_entry->ip6_neigh_cache_state == NC_NEIGH_PROBE) ||
                     (nc_entry->ip6_neigh_cache_state == NC_NEIGH_DELAY) )
                {
                    NUD6_Stop_Probing(nc_entry, device->dev_index);
                }

                nc_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;
            }
        }

        /* Otherwise, add the entry */
        else
            nc_entry = device->dev6_add_neighcache_entry(device, target_addr, 
                                                         link_addr, flags, NU_NULL, 
                                                         NC_NEIGH_STALE);
    }

    /* If a Neighbor Cache entry does not exist for the target address, create
     * a new one with its state set to INCOMPLETE.
     */
    if (!nc_entry)
        nc_entry = device->dev6_add_neighcache_entry(device, target_addr, NU_NULL,
                                                     0, NU_NULL, NC_NEIGH_INCOMPLETE);

    /* Get a pointer to the Destination Address */
    dest_addr = (UINT8 *)(&buf_ptr->data_ptr[IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET]);

    /* RFC 4861 section 8.3 - If no route exists for the destination, an
     * implementation SHOULD create such an entry.  Or if the Target 
     * Address is not a next-hop for the route, add the new next-hop.
     */
    if ( ((rt_entry = RTAB6_Find_Route(dest_addr, 
                                       RT_HOST_MATCH | RT_BEST_METRIC)) == NU_NULL) ||
         (RTAB6_Find_Next_Hop_Entry(rt_entry, nc_entry) == NU_NULL) )
    {
        /* If the next-hop is a router, set the RT_GATEWAY flag in the route. */
        if (flags & NC_ISROUTER)
            route_flags = (RT_UP | RT_GATEWAY);
        else
            route_flags = RT_UP;

        /* Cause the route to be updated to use this next-hop as the best
         * route if a route already exists.
         */
        route_flags |= (RT_LOCAL | RT_HOST | RT_BEST_METRIC);

        /* Adding a route will update the Next-Hop list of the Destination 
         * Cache entry and the Destination List of the Neighbor Cache entry.
         */
        if (RTAB6_Add_Route(device, dest_addr, nc_entry->ip6_neigh_cache_ip_addr, 128,
                            route_flags) != NU_SUCCESS)
            NLOG_Error_Log("Failed to add the route", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If a route was found, free it */
    if (rt_entry)
        RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP6);

    return (0);

} /* ND6RDRCT_Input */
