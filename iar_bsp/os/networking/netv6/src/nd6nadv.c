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
*       nd6nadv.c                                    
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process an 
*       incoming Neighbor Advertisement message and build and transmit 
*       an outgoing Neighbor Advertisement message.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6NADV_Input
*       ND6NADV_Output
*       ND6NADV_Build
*                                                                          
*   DEPENDENCIES                                                             
*               
*       externs.h
*       externs6.h
*       dad6.h
*       nd6opts.h
*       nd6.h
*       nd6nadv.h
*       in6.h
*       defrtr6.h
*       nc6.h
*       nud6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/externs6.h"
#include "networking/dad6.h"
#include "networking/nd6opts.h"
#include "networking/nd6.h"
#include "networking/nd6nadv.h"
#include "networking/in6.h"
#include "networking/defrtr6.h"
#include "networking/nc6.h"
#include "networking/nud6.h"

extern TQ_EVENT     IP6_Resolve6_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6NADV_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function process a Neighbor Advertisement received on the
*       respective interface.  A node sends Neighbor Advertisements in
*       response to Neighbor Solicitations and sends unsolicited
*       Neighbor Advertisements in order to propagate new information
*       quickly.  
*
*       If the Neighbor Advertisement is unsolicited, and
*       the receiving node does not already have an entry for the
*       sender in its Neighbor Cache, the packet is discarded.
*
*       If the Neighbor Advertisement is unsolicited, and
*       the receiving node does have an entry for the sender in its 
*       Neighbor Cache, the Neighbor Cache is updated accordingly.
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
*  |R|S|O|                       Reserved                          |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +                        Target Address                         +
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
STATUS ND6NADV_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                     const NET_BUFFER *buf_ptr)
{
    UINT8                           *link_addr = NU_NULL;
    UINT8                           *target_addr;
    IP6_NEIGHBOR_CACHE_ENTRY        *current_entry = NU_NULL;
    union   nd_opts                 ndopts;
    UINT32                          icmp6len = buf_ptr->mem_total_data_len;
    UINT8                           flags;
    DEV6_IF_ADDRESS                 *target_address;

    /* If the Neighbor Advertisement was received on a virtual device,
     * silently ignore the packet.
     */
    if (device->dev_flags & DV6_VIRTUAL_DEV)
    {
        NLOG_Error_Log("Received a Neighbor Advertisement on a virtual device", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (0);
    }

    /* Get a pointer to the Target Address */
    target_addr = (UINT8 *)(&buf_ptr->data_ptr[IP6_ICMP_NEIGH_ADV_TRGT_ADDRS_OFFSET]);

    /* Get the length of the options */
    icmp6len -= IP6_NEIGH_ADV_HDR_SIZE;

    /* Validate the Neighbor Advertisement - if invalid, silently discard
     * the packet.
     */
    if (ND6_Validate_Message(ICMP6_NEIGH_ADV, pkt->ip6_src, pkt->ip6_dest, 
                             target_addr, pkt, icmp6len, IP6_NEIGH_ADV_HDR_SIZE,
                             &ndopts, buf_ptr) == -1)
    {
        MIB_ipv6IfIcmpInErrors_Inc(device);

        NLOG_Error_Log("Neighbor Advertisement packet silently discarded", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return (0);
    }

    /* Search through the device's addresses for a match on the Target Address */
    target_address = DEV6_Find_Target_Address(device, target_addr);

    /* If the address in the packet matches one of the addresses of my 
     * interfaces.
     */
    if (target_address)
    {
        /* If the address of the interface is in the TENTATIVE state, 
         * someone else is using my address, so Duplicate Address 
         * Detection has failed.
         */
        if (target_address->dev6_addr_state & DV6_TENTATIVE)
        {
           /* RFC 4862 - section 5.4 - Other packets addressed to the tentative
            * address should be silently discarded.  Note that the "other packets"
            * include Neighbor Solicitation and Advertisement messages that have
            * the tentative (i.e., unicast) address as the IP destination address
            * and contain the tentative address in the Target Address field.
            */
        	if (memcmp(pkt->ip6_dest, target_addr, IP6_ADDR_LEN) != 0)
        	{
				NLOG_Error_Log("DAD has failed", NERR_INFORMATIONAL, __FILE__,
							   __LINE__);

#if (INCLUDE_DAD6 == NU_TRUE)

				/* There is a problem - we must flag this address as invalid */
				nd6_dad_na_input(target_address);
        	}
#endif

            return (0);
        }
    }

    else
    {
        /* Search the Neighbor Cache for a matching entry. */
        current_entry = device->dev6_fnd_neighcache_entry(device, 
                                                          target_addr);
    }

    /* If no entry exists, silently discard the packet. */
    if (current_entry == NU_NULL)
    {
        NLOG_Error_Log("Neighbor Advertisement discarded because no matching entry exists", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return (0);
    }

    /* If a Target Link-Local Address option is present, extract the
     * link-local address and the length.
     */
    if (ndopts.nd_opt_array[IP6_ICMP_OPTION_TAR_ADDR])
        link_addr = (UINT8*)((UNSIGNED)(ndopts.nd_opt_array[IP6_ICMP_OPTION_TAR_ADDR]) +
                    IP6_ICMP_LL_OPTION_ADDRESS_OFFSET);

    flags = buf_ptr->data_ptr[IP6_ICMP_NEIGH_ADV_FLAG_OFFSET];

    /* If the Neighbor Cache entry is in the INCOMPLETE state */
    if (current_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE)
    {
        /* If no Target Link-Layer address option is included, silently
         * discard the packet.
         */
        if (!link_addr)
        {
            NLOG_Error_Log("Neighbor Advertisement discarded due to no Target Link-Layer address option", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (0);
        }

        if (TQ_Timerunset(IP6_Resolve6_Event, TQ_CLEAR_EXACT, 
                          current_entry->ip6_neigh_cache_resolve_id, 
                          (UNSIGNED)device->dev_index) != NU_SUCCESS)
            NLOG_Error_Log("Failed to stop the Address Resolution timer", 
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Record the Link-Layer address in the Neighbor Cache entry */
        device->dev6_update_neighcache_link_addr(current_entry, link_addr);

        /* If the advertisement's Solicited Flag is set, set the state of
         * the entry to REACHABLE.
         */
        if (flags & IP6_NA_FLAG_SOLICITED)
            NUD6_Confirm_Reachability(current_entry);

        /* If the advertisement's Solicited Flag is not set, set the state
         * of the entry to STALE.
         */
        else
            current_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;

        /* Set the IsRouter flag of the entry based on the IsRouter flag
         * of the advertisement.
         */
        if (flags & IP6_NA_FLAG_ROUTER)
            NC6_Transition_To_Router(current_entry);

        /* Transmit the data awaiting address resolution to complete. */
        ND6_Transmit_Queued_Data(current_entry, device);
    }

    /* If the Neighbor Cache entry is in any other state than the INCOMPLETE
     * state. 
     */
    else
    {
        /* If the Override Flag is not set, and the link-layer address in
         * the packet differs from the link-layer address in the Neighbor
         * Cache entry.
         */
        if (!(flags & IP6_NA_FLAG_OVERRIDE) && 
             (link_addr) && 
             (device->dev6_neighcache_entry_equal(link_addr, current_entry) != 0) )
        {
            /* If the state of the entry is REACHABLE, set the state to STALE. */
            if (current_entry->ip6_neigh_cache_state == NC_NEIGH_REACHABLE)
                current_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;
        }

        /* If the Override Flag is set, or the Override Flag is not set and 
         * the supplied link-layer address is the same as that in the Neighbor
         * Cache entry, or no Target Link-Layer address option was supplied.
         */
        else if ( (!link_addr) ||
                  (flags & IP6_NA_FLAG_OVERRIDE) ||
                  ((!(flags & IP6_NA_FLAG_OVERRIDE)) &&
                  (device->dev6_neighcache_entry_equal(link_addr, current_entry) == 0)) )
        {
            /* If the Solicited Flag is set, set the state of the entry to
             * REACHABLE.
             */
            if (flags & IP6_NA_FLAG_SOLICITED)
                NUD6_Confirm_Reachability(current_entry);

            /* If a link-layer address is provided */
            if ( (link_addr) &&
                 (device->dev6_neighcache_entry_equal(link_addr, current_entry) != 0) )
            {
                /* Update the Neighbor Cache entry with the link-layer address in
                 * the link-layer address option.
                 */
                device->dev6_update_neighcache_link_addr(current_entry, link_addr);

                /* If the Solicited Flag is not set, and the link-layer address 
                 * in the packet differs from the link-layer address in the entry,
                 * set the state of the entry to STALE.
                 */
                if (!(flags & IP6_NA_FLAG_SOLICITED))
                {
                    /* If we are currently probing the neighbor, cancel probing
                     * now.
                     */
                    if ( (current_entry->ip6_neigh_cache_state == NC_NEIGH_PROBE) ||
                         (current_entry->ip6_neigh_cache_state == NC_NEIGH_DELAY) )
                    {
                        NUD6_Stop_Probing(current_entry, device->dev_index);
                    }

                    current_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;
                }
            }

            /* The IsRouter flag in the cache entry must be set based on the
             * Router flag in the received advertisement.
             */
            if (!(flags & IP6_NA_FLAG_ROUTER))
            {
                /* If the state of the IsRouter Flag changes from TRUE to
                 * FALSE, remove the router from the Default Router List, and
                 * update the route entries for all destinations using that 
                 * Neighbor as a router.
                 */
                if (current_entry->ip6_neigh_cache_flags & NC_ISROUTER)
                {
                    DEFRTR6_Delete_Default_Router(current_entry->
                                                  ip6_neigh_cache_ip_addr);

                    /* Delete all the routes that use this node as the 
                     * gateway. 
                     */
                    RTAB6_Delete_Route_By_Gateway(current_entry->
                                                  ip6_neigh_cache_ip_addr);
                }

                current_entry->ip6_neigh_cache_flags &= ~NC_ISROUTER;
            }

            else
            {
                NC6_Transition_To_Router(current_entry);
            }
        }
    }

    return (0);

} /* ND6NADV_Input */

/*************************************************************************
*
*   FUNCTION
*
*       ND6NADV_Output
*
*   DESCRIPTION
*
*       This function builds and outputs a Neighbor Advertisement
*       message.
*
*   INPUTS
*
*       *device                 The device out which to transmit the 
*                               packet.
*       *dest_addrs             The Destination Address of the packet.
*       *target_address         The Target Address of the packet.
*       *link_addr              The link-layer address to place in the 
*                               link-layer option of the packet.
*       flags                   The solicited, router or override flag.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       NU_INVALID_ADDRESS      There is no address of appropriate scope
*                               on the device to communicate with the
*                               destination.
*       NU_NO_BUFFERS           Insufficient NET Buffers.
*       NU_HOST_UNREACHABLE     A route to the host does not exist.
*       NU_MSGSIZE              The size of the message is wrong.
*
*************************************************************************/
STATUS ND6NADV_Output(DV_DEVICE_ENTRY *device, UINT8 *source_addrs,
                      UINT8 *dest_addrs, const UINT8 *target_address, 
                      const UINT8 *link_addr, UINT8 flags)
{
    NET_BUFFER          *buf_ptr;
    UINT16              checksum;
    STATUS              status;
    MULTI_SCK_OPTIONS       multi_opts;
    IP6_S_OPTIONS       ip6_options;
    RTAB6_ROUTE         dest_route;

    memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

    /* If the address was sent to a multicast address, select a source
     * address to use.
     */
    if (IPV6_IS_ADDR_MULTICAST(source_addrs))
        ip6_options.tx_source_address = in6_ifawithifp(device, dest_addrs);

    /* Otherwise, use the address to which the packet was transmitted */
    else
        ip6_options.tx_source_address = source_addrs;

    /* If there are no addresses on the interface, return */
    if (ip6_options.tx_source_address == NU_NULL)
    {
        NLOG_Error_Log("Neighbor Advertisement cannot be sent due to no address for outgoing interface", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (NU_INVALID_ADDRESS);
    }

    /* Build the ICMP Neighbor Advertisement portion of the packet */
    buf_ptr = ND6NADV_Build(device, target_address, link_addr, flags);

    if (buf_ptr != NU_NULL)
    {
        /* Compute the Checksum */
        checksum = UTL6_Checksum(buf_ptr, ip6_options.tx_source_address, dest_addrs, 
                                 buf_ptr->mem_total_data_len, IPPROTO_ICMPV6, 
                                 IPPROTO_ICMPV6);

        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);

        UTL_Zero(&multi_opts, sizeof(MULTI_SCK_OPTIONS));

        /* If the Destination Address is a multicast address, set the multicast
         * option for the device.
         */
        if (IPV6_IS_ADDR_MULTICAST(dest_addrs))
        {
            dest_route.rt_route = NU_NULL;
            multi_opts.multio_device = device;
        }

        else
        {
            /* Find a route to the destination using the same interface as the
             * Neighbor Solicitation was received on.
             */
            dest_route.rt_route = 
                (RTAB6_ROUTE_ENTRY*)(RTAB6_Find_Route_By_Device(dest_addrs, device));

            if (dest_route.rt_route)
            {
                NU_BLOCK_COPY(dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                              dest_addrs, IP6_ADDR_LEN);

                dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;
                dest_route.rt_ip_dest.rtab6_rt_device = device;
            }
        }
       
        ip6_options.tx_dest_address = dest_addrs;
        ip6_options.tx_hop_limit = ICMP6_VALID_HOP_LIMIT;

        /* Increment the number of ICMP messages sent. */
        MIB_ipv6IfIcmpOutMsgs_Inc(device);

        status = IP6_Send(buf_ptr, &ip6_options, IP_ICMPV6_PROT, &dest_route, 
                          IP6_DONTROUTE, &multi_opts);
        
        if (status != NU_SUCCESS)
        {
            /* Increment the number of send errors. */
            MIB_ipv6IfIcmpOutErrors_Inc(device);

            NLOG_Error_Log("Neighbor Advertisement not sent", 
                           NERR_SEVERE, __FILE__, __LINE__);

            /* The packet was not sent.  Deallocate the buffer.  If the packet was
             * transmitted it will be deallocated when the transmit complete
             * interrupt occurs. 
             */
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
        }

        else
            MIB_ipv6IfIcmpOutNeigAdv_Inc(device);

        /* If a route was found, free it. */
        if (dest_route.rt_route)
            RTAB_Free((ROUTE_ENTRY*)dest_route.rt_route, NU_FAMILY_IP6);
    }
    else
    {
        /* Increment the number of send errors. */
        MIB_ipv6IfIcmpOutErrors_Inc(device);

        NLOG_Error_Log("Neighbor Advertisement not sent because of no buffers", 
                       NERR_SEVERE, __FILE__, __LINE__);

        status = NU_NO_BUFFERS;
    }

    return (status);

} /* ND6NADV_Output */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6NADV_Build
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function builds a Neighbor Advertisement Message.                                                               
*                                                                         
*   INPUTS                                                                
*             
*       *device                 A pointer to the device.                                         
*       *target_address         The address to put in the Target Address 
*                               field of the Neighbor Solicitation 
*                               Message.
*       *source_link_addr       The address to put in the Source 
*                               Link-Layer option.  If this address is 
*                               NULL, the Source Link-Layer option is not 
*                               built.
*       flags                   The solicited flag, router flag, or 
*                               override flag.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       A pointer to the buffer in which the message was built or NU_NULL
*       if the message could not be built.
*
*************************************************************************/
NET_BUFFER *ND6NADV_Build(const DV_DEVICE_ENTRY *device, 
                          const UINT8 *target_address, 
                          const UINT8 *source_link_addr, UINT8 flags)
{
    UINT32      message_size;
    NET_BUFFER  *buf_ptr;

    /* If the source_link_addr is not NULL and the interface is not a
     * virtual interface, build a Source Link-Layer option.  Otherwise, 
     * do not build a Source Link-Layer option.
     */
    if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
         (source_link_addr != NU_NULL) )
        message_size = IP6_NEIGH_ADV_HDR_SIZE + IP6_LINK_LAYER_OPT_SIZE +
                       device->dev_addrlen;
    else
        message_size = IP6_NEIGH_ADV_HDR_SIZE;

    /* Build the common fields of the ICMP header */
    buf_ptr = ICMP6_Header_Init(ICMP6_NEIGH_ADV, 0, message_size);

    /* If a net buffer was created */
    if (buf_ptr != NU_NULL)
    {
        /* Initialize the ICMP checksum to 0 */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);

        /* Zero out the 32 bytes */
        PUT32(buf_ptr->data_ptr, IP6_ICMP_NEIGH_ADV_FLAG_OFFSET, 0);

        /* Put the flags in the packet */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_NEIGH_ADV_FLAG_OFFSET, flags);

        /* Put the target address into the Target Address field of the packet */
        NU_BLOCK_COPY(&buf_ptr->data_ptr[IP6_ICMP_NEIGH_ADV_TRGT_ADDRS_OFFSET],
                      target_address, IP6_ADDR_LEN);
        
        /* If necessary, build the Source Link-Layer option */
        if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
             (source_link_addr != NU_NULL) )
             
        {
            buf_ptr->data_ptr += IP6_NEIGH_ADV_HDR_SIZE;

            ND6_Build_Link_Layer_Opt(IP6_ICMP_OPTION_TAR_ADDR, source_link_addr, 
                                     buf_ptr, device->dev_addrlen);

            buf_ptr->data_ptr -= IP6_NEIGH_ADV_HDR_SIZE;
        }        
    }
    else
        NLOG_Error_Log("Cannot build Neighbor Advertisement",                   
                       NERR_SEVERE, __FILE__, __LINE__);

    return (buf_ptr);

} /* ND6NADV_Build */
