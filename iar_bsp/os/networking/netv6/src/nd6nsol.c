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
*       nd6nsol.c                                    
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process an 
*       incoming Neighbor Solicitation message and build and transmit an 
*       outgoing Neighbor Solicitation message.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6NSOL_Input
*       ND6NSOL_Output
*       ND6NSOL_Build
*                                                                          
*   DEPENDENCIES                                                             
*               
*       externs.h
*       externs6.h
*       dad6.h
*       nd6opts.h
*       nd6.h
*       nd6nsol.h
*       in6.h
*       nd6nadv.h
*       nc6.h
*       nud6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/externs6.h"
#include "networking/dad6.h"
#include "networking/nd6opts.h"
#include "networking/nd6.h"
#include "networking/nd6nsol.h"
#include "networking/in6.h"
#include "networking/nd6nadv.h"
#include "networking/nc6.h"
#include "networking/nud6.h"

extern UINT8        IP6_All_Nodes_Multi[];
extern TQ_EVENT     IP6_Resolve6_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6NSOL_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function processes a Neighbor Solicitation received on the
*       respective interface.  Nodes send Neighbor Solicitations to request
*       the link-layer address of a target node while also providing their
*       own link-layer address to the target.  Neighbor Solicitations are
*       multicast when the node needs to resolve an address and unicast
*       when the node seeks to verify the reachability of a neighbor.
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
*  |    Options ...
*  +-+-+-+-+-+-+-+-+-+-
*                              
*   OUTPUTS                                                               
*                                           
*       0                              
*
*************************************************************************/
STATUS ND6NSOL_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                     const NET_BUFFER *buf_ptr)
{
    UINT8                           *link_addr = NU_NULL;
    UINT8                           *target_addr;
    UINT8                           nadv_dest_addr[IP6_ADDR_LEN];
    DEV6_IF_ADDRESS                 *target_address;
    IP6_NEIGHBOR_CACHE_ENTRY        *nc_entry;
    UINT32                          icmp6len = buf_ptr->mem_total_data_len;
    union   nd_opts                 ndopts;
    UINT8                           flags = 0;

    /* Get a pointer to the Target Address */
    target_addr = (UINT8 *)(&buf_ptr->data_ptr[IP6_ICMP_NEIGH_ADV_TRGT_ADDRS_OFFSET]);

    /* Get the length of the options */
    icmp6len -= IP6_NEIGH_SOL_HDR_SIZE;

    /* Validate the Neighbor Solicitation - if invalid, silently discard the 
     * packet.
     */
    if (ND6_Validate_Message(ICMP6_NEIGH_SOL, pkt->ip6_src, pkt->ip6_dest, 
                             target_addr, pkt, icmp6len, IP6_NEIGH_SOL_HDR_SIZE,
                             &ndopts, buf_ptr) == -1)
    {
        MIB_ipv6IfIcmpInErrors_Inc(device);

        NLOG_Error_Log("Neighbor Solicitation silently discarded", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return (0);
    }

    /* If a Source Link-Local Address option is present and the packet was
     * not received on a virtual interface, extract the link-local address.
     */
    if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
         (ndopts.nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR]) )
        link_addr = (UINT8*)((UNSIGNED)(ndopts.nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR]) + 
                    IP6_ICMP_LL_OPTION_ADDRESS_OFFSET);

    /* Search through the device's addresses for a match on the Target Address */
    target_address = DEV6_Find_Target_Address(device, target_addr);

    /* If the Target Address is not a unicast or anycast address assigned
     * to the receiving interface, or the packet is from this node,
     * silently discard the packet.
     */
    if ( (target_address == NU_NULL) ||
         (memcmp(target_address->dev6_ip_addr, pkt->ip6_src, IP6_ADDR_LEN) == 0) )
        return (0);

    /* Receiving Neighbor Solicitations per RFC 2462 section 7.2.3 - the 
     * address is in the TENTATIVE state.  A node does not respond to a 
     * Neighbor Solicitation for a TENTATIVE address.
     */
    if (target_address->dev6_addr_state & DV6_TENTATIVE)
    {
        /* If the source address is not a unicast address and the packet is
         * not destined to the tentative unicast address.
         */
        if ( !(IPV6_IS_ADDR_LINKLOCAL(pkt->ip6_src)) && 
             !(IN6_IS_ADDR_SITELOCAL(pkt->ip6_src)) &&
             (memcmp(pkt->ip6_dest, target_addr, IP6_ADDR_LEN) != 0) )
        {
            /* If the Source Address is the unspecified address, and the
             * solicitation is from another node, the source node
             * is performing Address Resolution, and the address is not
             * unique.
             */
            if (IPV6_IS_ADDR_UNSPECIFIED(pkt->ip6_src))
            {
                NLOG_Error_Log("DAD failed", NERR_INFORMATIONAL, __FILE__, 
                               __LINE__);

#if (INCLUDE_DAD6 == NU_TRUE)

                /* The address is a duplicate - inform DAD */
                nd6_dad_ns_input(target_address);
#endif
                return (0);
            }
        }

        /* Otherwise, the Target Address is TENTATIVE, and the source address
         * is a unicast address (the sender is performing Address Resolution
         * on the target) or the packet is destined to the tentative address.
         * RFC 4862 - section 5.4 - Other packets addressed to the tentative
         * address should be silently discarded.  Note that the "other packets"
         * include Neighbor Solicitation and Advertisement messages that have
         * the tentative (i.e., unicast) address as the IP destination address
         * and contain the tentative address in the Target Address field.
         */
        else
            return(0);
    }

    /* Receiving Neighbor Solicitations per RFC 4861 section 7.2.3 - the
     * address is not in the TENTATIVE state.  Also want to check that the
     * address has not already been flagged as a duplicate.
     */
    else if (!(target_address->dev6_addr_state & DV6_DUPLICATED))
    {
        /* If the Source Address is not the Unspecified Address, and the
         * packet contains a Source Link-Layer option, create or update the
         * Neighbor Cache entry corresponding to the Source Address of the 
         * packet.
         */
        if (!IPV6_IS_ADDR_UNSPECIFIED(pkt->ip6_src))
        {
            if (link_addr)
            {
                /* If an entry does not already exist, create a new one, 
                 * and set its reachability state to STALE, and set the 
                 * Router flag to FALSE.
                 */
                nc_entry = device->dev6_fnd_neighcache_entry(device, 
                                                             pkt->ip6_src);

                if (nc_entry == NU_NULL)
                    device->dev6_add_neighcache_entry(device, pkt->ip6_src, 
                                                      link_addr, 0, NU_NULL, 
                                                      NC_NEIGH_STALE);
    
                /* If an entry already exists. */
                else
                {
                    /* If the state of the entry is INCOMPLETE. */
                    if (nc_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE)
                    {
                        /* Record the link-layer address per Appendix C of 
                         * RFC 4861.
                         */
                        device->dev6_update_neighcache_link_addr(nc_entry, 
                                                                 link_addr);

                        /* Set the state to STALE per Appendix C of RFC 4861. */
                        nc_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;

                        /* Unset the address resolution timer if it is running. */
                        if (TQ_Timerunset(IP6_Resolve6_Event, TQ_CLEAR_EXACT, 
                                          nc_entry->ip6_neigh_cache_resolve_id, 
                                          (UNSIGNED)device->dev_index) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to stop the Address Resolution timer", 
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        /* Transmit any data that is queued awaiting address
                         * resolution to complete.
                         */
                        ND6_Transmit_Queued_Data(nc_entry, device);
                    }

                    /* If the cached link-layer address differs from the one 
                     * in the received Source Link-Layer option, replace the 
                     * cached address with the received address, and set the 
                     * entry's reachability state to STALE.
                     */
                    else if (device->dev6_neighcache_entry_equal(link_addr, 
                                                                 nc_entry) != 0)
                    {
                        device->dev6_update_neighcache_link_addr(nc_entry, 
                                                                 link_addr);

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
            }

            /* If the source of the Solicitation is not the Unspecified Address,
             * the node must set the Solicited flag to TRUE and unicast the
             * advertisement to the Source Address of the solicitation.
             */
            flags |= IP6_NA_FLAG_SOLICITED;
            NU_BLOCK_COPY(nadv_dest_addr, pkt->ip6_src, IP6_ADDR_LEN);
        }

        /* If the source of the Solicitation is the Unspecified Address, the
         * node must set the solicited flag to FALSE and multicast the 
         * advertisement to the All-Nodes Multicast Address.
         */
        else
            NU_BLOCK_COPY(nadv_dest_addr, IP6_All_Nodes_Multi, IP6_ADDR_LEN);

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

        /* If the node is a router, set the isRouter flag to TRUE */
        if (device->dev_flags & DV6_ISROUTER)
            flags |= IP6_NA_FLAG_ROUTER;

#endif

        /* Since the Target Link-Layer option is included, set the override flag
         * to TRUE.
         */
        flags |= IP6_NA_FLAG_OVERRIDE;

        /* Either the Neighbor Cache has been updated, or the sender is performing
         * Duplicate Address Detection on one of our addresses that is not in 
         * the TENTATIVE state - transmit a Neighbor Advertisement to inform the
         * sender.  The Destination address of the Advertisement is the Source
         * address of the Solicitation, because the source of the Solicitation is
         * not the Unspecified address.
         */
        if (ND6NADV_Output(device, pkt->ip6_dest, nadv_dest_addr, target_addr, 
                           device->dev_mac_addr, flags) != NU_SUCCESS)
            NLOG_Error_Log("Failed to transmit the Neighbor Advertisement message", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    return (0);

} /* ND6NSOL_Input */

/*************************************************************************
*
*   FUNCTION
*
*       ND6NSOL_Output
*
*   DESCRIPTION
*
*       This function builds and transmit a Neighbor Solicitation 
*       message.
*
*   INPUTS
*
*       *device                 A pointer to the device out which to send 
*                               the packet.
*       *dest_address           A pointer to the Destination Address of 
*                               the packet.
*       *target_address         A pointer to the Target Address of the 
*                               packet.
*       *pkt_source_address     A pointer to the Source Address of the 
*                               packet.
*       dad                     A flag to indicate whether this is a DAD 
*                               packet.
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
STATUS ND6NSOL_Output(DV_DEVICE_ENTRY *device, UINT8 *dest_address, 
                      const UINT8 *target_address, 
                      const UINT8 *pkt_source_address, UINT8 dad)
{
    NET_BUFFER          *buf_ptr;
    UINT8               *link_addr;
    DEV6_IF_ADDRESS     *current_address;
    UINT16              checksum;
    STATUS              status;
    MULTI_SCK_OPTIONS   multi_opts;
    IP6_S_OPTIONS       ip6_options;
    RTAB6_ROUTE         dest_route;

    memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

    /* If DAD is not running, select the source IP address.
     */
    if (dad == NU_FALSE)
    {
        /* If the source address of the packet prompting the Neighbor
         * Solicitation is the same as one of the addresses assigned to the
         * outgoing interface, that address is placed in the IP Source 
         * Address of the outgoing Neighbor Solicitation.
         */
        current_address = DEV6_Find_Target_Address(device, pkt_source_address);

        /* If a match was not found, find the longest prefix match on
         * this interface.
         */
        if (current_address == NU_NULL)
        {
            ip6_options.tx_source_address = in6_ifawithifp(device, dest_address);

            if (ip6_options.tx_source_address == NU_NULL)
                return (NU_INVALID_ADDRESS);
        }

        /* Otherwise, use the IP address found */
        else
            ip6_options.tx_source_address = current_address->dev6_ip_addr;

        link_addr = device->dev_mac_addr;
    }

    /* Zero out the source address.  If DAD is being run on the address,
     * the source address is the Unspecified Address.
     */
    else
    {
        ip6_options.tx_source_address = IP6_ADDR_ANY.is_ip_addrs;

        /* RFC 4861 section 4.3 - Source Link-Layer address option must not
         * be included when the source IP Address is the unspecified address.
         */
        link_addr = NU_NULL;
    }

    /* Build the ICMP Neighbor Advertisement portion of the packet */
    buf_ptr = ND6NSOL_Build(device, target_address, link_addr);

    if (buf_ptr != NU_NULL)
    {
        /* Compute the Checksum */
        checksum = UTL6_Checksum(buf_ptr, ip6_options.tx_source_address, dest_address, 
                                 buf_ptr->mem_total_data_len, IPPROTO_ICMPV6, 
                                 IPPROTO_ICMPV6);

        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);

        UTL_Zero(&multi_opts, sizeof(MULTI_SCK_OPTIONS));

        /* If the Destination Address is a multicast address, set the multicast
         * option for the device.
         */
        if (IPV6_IS_ADDR_MULTICAST(dest_address))
        {
            dest_route.rt_route = NU_NULL;
            multi_opts.multio_device = device;
        }

        else
        {
            /* Find a route to the destination using the correct interface. */
            dest_route.rt_route = 
                (RTAB6_ROUTE_ENTRY*)(RTAB6_Find_Route_By_Device(dest_address, device));

            if (dest_route.rt_route)
            {
                NU_BLOCK_COPY(dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                              dest_address, IP6_ADDR_LEN);

                dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;
                dest_route.rt_ip_dest.rtab6_rt_device = device;
            }
        }

        ip6_options.tx_dest_address = dest_address;
        ip6_options.tx_hop_limit = ICMP6_VALID_HOP_LIMIT;

        /* Increment the number of ICMP messages sent. */
        MIB_ipv6IfIcmpOutMsgs_Inc(device);

        status = IP6_Send(buf_ptr, &ip6_options, IP_ICMPV6_PROT, &dest_route, 
                          IP6_DONTROUTE, &multi_opts);

        if (status != NU_SUCCESS)
        {
            /* Increment the number of send errors. */
            MIB_ipv6IfIcmpOutErrors_Inc(device);

            NLOG_Error_Log("Neighbor Solicitation not sent", NERR_SEVERE, 
                           __FILE__, __LINE__);

            /* The packet was not sent.  Deallocate the buffer.  If the packet was
             * transmitted it will be deallocated when the transmit complete
             * interrupt occurs. 
             */
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
        }

        else
            MIB_ipv6IfIcmpOutNeigSolic_Inc(device);

        /* If a route was found, free it. */
        if (dest_route.rt_route)
            RTAB_Free((ROUTE_ENTRY*)dest_route.rt_route, NU_FAMILY_IP6);
    }
    else
    {
        /* Increment the number of send errors. */
        MIB_ipv6IfIcmpOutErrors_Inc(device);

        NLOG_Error_Log("Neighbor Solicitation not sent", NERR_SEVERE, 
                       __FILE__, __LINE__);

        status = NU_NO_BUFFERS;
    }

    return (status);

} /* ND6NSOL_Output */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6NSOL_Build
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function builds a Neighbor Solicitation Message.                                                               
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
*                                                                         
*   OUTPUTS                                                               
*                                           
*       A pointer to the buffer in which the message was built or NU_NULL
*       if the message could not be built.
*
*************************************************************************/
NET_BUFFER *ND6NSOL_Build(const DV_DEVICE_ENTRY *device,
                          const UINT8 *target_address, 
                          const UINT8 *source_link_addr)
{
    UINT32      message_size;
    NET_BUFFER  *buf_ptr;

    /* If the source_link_addr is not NULL and the device is not a 
     * virtual device, build a Source Link-Layer option.  Otherwise, do 
     * not build a Source Link-Layer option.
     */
    if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
         (source_link_addr != NU_NULL) )
        message_size = IP6_NEIGH_SOL_HDR_SIZE + IP6_LINK_LAYER_OPT_SIZE +
                       device->dev_addrlen;
    else
        message_size = IP6_NEIGH_SOL_HDR_SIZE;

    /* Build the common fields of the ICMP header */
    buf_ptr = ICMP6_Header_Init(ICMP6_NEIGH_SOL, 0, message_size);

    if (buf_ptr != NU_NULL)
    {
        /* Initialize the checksum to 0 */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);

        PUT32(buf_ptr->data_ptr, IP6_ICMP_NEIGH_SOL_RES_OFFSET, 0);

        /* Put the target address into the Target Address field of the packet */
        NU_BLOCK_COPY(&buf_ptr->data_ptr[IP6_ICMP_NEIGH_SOL_TRGT_ADDRS_OFFSET],
                      target_address, IP6_ADDR_LEN);
       
        /* If necessary, build the Source Link-Layer option */
        if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
             (source_link_addr != NU_NULL) )
             
        {
            buf_ptr->data_ptr += IP6_NEIGH_SOL_HDR_SIZE;

            ND6_Build_Link_Layer_Opt(IP6_ICMP_OPTION_SRC_ADDR, source_link_addr, 
                                     buf_ptr, device->dev_addrlen);

            buf_ptr->data_ptr -= IP6_NEIGH_SOL_HDR_SIZE;
        }        
    }
    else
        NLOG_Error_Log("No buffers available", NERR_SEVERE, __FILE__, 
                       __LINE__);

    return (buf_ptr);

} /* ND6NSOL_Build */
