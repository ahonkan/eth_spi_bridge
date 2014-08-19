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
*       nd6.c                                        
*                                                                               
*   COMPONENT                                                             
*                                                                       
*       Neighbor Discovery
*                                                                       
*   DESCRIPTION                                                           
*                                                                    
*       This file contains those functions necessary to process Neighbor
*       Discovery packets.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       ND6_Validate_Options
*       ND6_Validate_Message
*       ND6_Build_Link_Layer_Opt
*       ND6_Process_Link_Layer_Option
*       ND6_Transmit_Queued_Data
*       ND6_Compute_Random_Timeout
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       nd6opts.h
*       nc6.h
*       nd6.h
*       defrtr6.h
*       nud6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/nd6opts.h"
#include "networking/nc6.h"
#include "networking/nd6.h"
#include "networking/defrtr6.h"
#include "networking/nud6.h"

STATIC  INT         ND6_Validate_Options(UINT16, union nd_opts *, VOID *);

extern  UINT8       IP6_Solicited_Node_Multi[];
extern  TQ_EVENT    IP6_Resolve6_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6_Validate_Options
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function validates the length of the options included in
*       a Neighbor Discovery packet.
*                                                                         
*   INPUTS                                                                
*                   
*       options_length          The length of all options in the packet.
*       *ndopts                 A pointer to the options validation 
*                               structure.
*       *nd_struct              A pointer to the options.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0                       The length of all options is valid.
*       -1                      At least one option is of invalid length.
*
*************************************************************************/
STATIC INT ND6_Validate_Options(UINT16 options_length, union nd_opts *ndopts, 
                                VOID *nd_struct)
{
    /* Initialize the options data structure */
    nd6_option_init(nd_struct, (int)options_length, ndopts);

    /* Extract the options and store them in ndopts */
    if (nd6_options(ndopts) < 0)
        return (-1);
    else
        return (0);

} /* ND6_Validate_Options */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6_Validate_Message
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function validates a Neighbor Discovery packet.
*                                                                         
*   INPUTS                                                                
*                   
*       type                    The type of Neighbor Discovery packet 
*                               to validate.
*       *src_addr               The source address of the incoming 
*                               packet.
*       *dest_addr              The destination address of the incoming 
*                               packet.
*       *target_addr            The Target Address if appropriate for
*                               the packet type.
*       *pkt                    A pointer to the IP header of the packet.
*       opt_length              The length of the option.
*       icmp_length             The length of the ICMP portion of the 
*                               packet.
*       *ndopts                 A pointer to the options validation data 
*                               structure.
*       *buf_ptr                A pointer to the received packet.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0                       The packet is valid.
*       -1                      The packet is invalid.
*
*************************************************************************/
INT ND6_Validate_Message(UINT8 type, UINT8 *src_addr, const UINT8 *dest_addr,
                         const UINT8 *target_addr, const IP6LAYER *pkt, 
                         UINT32 opt_length, UINT32 icmp_length, 
                         union nd_opts *ndopts, const NET_BUFFER *buf_ptr)
{
    VOID                *nd_struct = &(buf_ptr->data_ptr[icmp_length]);
    RTAB6_ROUTE         ro;

    /* If the message includes an IP Authentication Header, the message
     * must authenticate correctly.
     */

    /* The hop limit must be ICMP6_VALID_HOP_LIMIT */
    if (pkt->ip6_hop != ICMP6_VALID_HOP_LIMIT)
    {
        NLOG_Error_Log("Hop Limit invalid for Neighbor Discovery packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (-1);
    }

    /* The ICMP code must be ICMP6_VALID_CODE */
    if (buf_ptr->data_ptr[IP6_ICMP_CODE_OFFSET] != ICMP6_VALID_CODE)
    {
        NLOG_Error_Log("ICMP code invalid for Neighbor Discovery packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (-1);
    }

    /* All included options must have a length greater than 0 */
    if (ND6_Validate_Options((UINT16)opt_length, ndopts, nd_struct) == -1)
    {
        NLOG_Error_Log("Options Length invalid for Neighbor Discovery packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (-1);
    }

    switch (type)
    {
    case ICMP6_REDIRECT:
        
        /* The IP Source address must be a link-local address. */
        if (!(IPV6_IS_ADDR_LINKLOCAL(src_addr)))
        {
            NLOG_Error_Log("Source Address invalid for Redirect packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* ICMP length must be 40 or more bytes */
        if (icmp_length < ICMP6_REDIRECT_MIN_LENGTH)
        {
            NLOG_Error_Log("Length invalid for Redirect packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        NU_BLOCK_COPY(ro.rt_ip_dest.rtab6_rt_ip_dest.sck_addr, 
                      (UINT8 *)(&buf_ptr->data_ptr[IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET]), 
                      IP6_ADDR_LEN);

        ro.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;

        /* Find a route to the specified ICMP Destination Address in the
         * Redirect message.
         */
        IP6_Find_Route(&ro);

        /* If a route was found. */
        if (ro.rt_route)
        {
            /* The IP source address of the Redirect must be the same as the 
             * current first-hop router for the specified ICMP Destination 
             * Address.
             */
            if (memcmp(ro.rt_route->rt_next_hop.sck_addr, src_addr, 
                       IP6_ADDR_LEN) != 0)
            {
                NLOG_Error_Log("Source Address in Redirect packet does not match the current first-hop for the Destination address", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                return (-1);
            }

            /* Free the route. */
            RTAB_Free((ROUTE_ENTRY*)(ro.rt_route), NU_FAMILY_IP6);
        }

        /* The route does not exist */
        else
        {
            NLOG_Error_Log("Source Address in Redirect packet does not match the current first-hop for the Destination address", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* The ICMP Destination address in the Redirect must not be a 
         * multicast address. 
         */
        if (IPV6_IS_ADDR_MULTICAST(&buf_ptr->data_ptr[IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET]))
        {
            NLOG_Error_Log("Destination Address is multicast in Redirect packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* The ICMP Target address is either a link-local address or the 
         * same as the ICMP Destination address.
         */
        if ((!IPV6_IS_ADDR_LINKLOCAL(&buf_ptr->data_ptr[IP6_ICMP_REDIRECT_TRGT_ADDRS_OFFSET]))
            && (memcmp(&buf_ptr->data_ptr[IP6_ICMP_REDIRECT_TRGT_ADDRS_OFFSET],
                       &buf_ptr->data_ptr[IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET],
                       IP6_ADDR_LEN) != 0) )
        {
            NLOG_Error_Log("Target Address invalid for Redirect packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        break;

    case ICMP6_RTR_ADV:

        /* The source address must be the link-local address */
        if (!(IPV6_IS_ADDR_LINKLOCAL(src_addr)))
        {
            NLOG_Error_Log("Source Address invalid for Router Advertisement packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* The ICMP length must be at least ICMP6_RTRADV_MIN_LENGTH bytes */
        if (icmp_length < ICMP6_RTRADV_MIN_LENGTH)
        {
            NLOG_Error_Log("Length invalid for Router Advertisement packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        break;

    case ICMP6_RTR_SOL:

        /* The ICMP length must be at least ICMP6_RTRSOL_MIN_LENGTH bytes */
        if (icmp_length < ICMP6_RTRSOL_MIN_LENGTH)
        {
            NLOG_Error_Log("Length invalid for Router Solicitation packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* If the IP Source Address is the Unspecified Address, there must not
         * be a Source Link-Layer Address option in the message.
         */
        if (IPV6_IS_ADDR_UNSPECIFIED(src_addr))
        {
            if (ndopts->nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR])
            {
                NLOG_Error_Log("Source Address is the Unspecified Address for Router Solicitation, and a Source Link-Layer option is included", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                return (-1);
            }
        }

        break;

    case ICMP6_NEIGH_ADV:

        /* The ICMP length must be at least ICMP6_NEIGHADV_MIN_LENGTH bytes */
        if (icmp_length < ICMP6_NEIGHADV_MIN_LENGTH)
        {
            NLOG_Error_Log("Length invalid for Neighbor Advertisement packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* The target address must not be a multicast address */
        if (IPV6_IS_ADDR_MULTICAST(target_addr))
        {
            NLOG_Error_Log("Target Address invalid for Neighbor Advertisement packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* If the destination address is a multicast address, the Solicited
         * flag must be zero.
         */
        if (IPV6_IS_ADDR_MULTICAST(dest_addr))
        {
            if ((GET8(buf_ptr->data_ptr, IP6_ICMP_NEIGH_ADV_FLAG_OFFSET)) & 
                 IP6_NA_FLAG_SOLICITED)
            {
                NLOG_Error_Log("Destination Address is multicast in Neighbor Advertisement packet, and the Solicited flag is set.", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                return (-1);
            }
        }

        break;

    case ICMP6_NEIGH_SOL:

        /* The ICMP length must be at least ICMP6_NEIGHSOL_MIN_LENGTH bytes */
        if (icmp_length < ICMP6_NEIGHSOL_MIN_LENGTH)
        {
            NLOG_Error_Log("Length invalid for Neighbor Solicitation packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* The target address must not be a multicast address */
        if (IPV6_IS_ADDR_MULTICAST(target_addr))
        {
            NLOG_Error_Log("Target address invalid for Neighbor Solicitation packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        /* If the source address is the unspecified address, the destination
         * address must be a solicited-node multicast address and there must
         * be no source link-layer address option in the message.
         */
        if (IPV6_IS_ADDR_UNSPECIFIED(src_addr))
        {   
            if ( (memcmp(dest_addr, IP6_Solicited_Node_Multi, 
                         IP6_SOL_NODE_MULTICAST_LENGTH) != 0) ||
                 (ndopts->nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR]) )
            {
                NLOG_Error_Log("Invalid Neighbor Solicitation Received", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                return (-1);
            }
        }

        /* On link layers that have addresses the source link-layer address 
         * option MUST be included in multicast solicitations and SHOULD 
         * be included in unicast solicitations.
         */
        else if ( (IPV6_IS_ADDR_MULTICAST(dest_addr)) && 
                  (buf_ptr->mem_buf_device->dev_type == DVT_ETHER) &&
                  (!(ndopts->nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR])) )
        {
            NLOG_Error_Log("Invalid Neighbor Solicitation Received - no SLL option present", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }

        break;

    default:

        break;
    }

    return (0);

} /* ND6_Validate_Message */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6_Build_Link_Layer_Opt
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function will place the link-layer address of the sending
*       node in a link-layer address option header.
*                                                                         
*   INPUTS                                                                
*                 
*       type                    The type of packet.
*       *link_addr              The link-layer address of the sending 
*                               node.
*       *buf_ptr                A pointer to the buffer in which to place
*                               the option.
*       link_addr_length        The length of the link-layer address.
*                                                 
*   OUTPUTS                                                               
*                                                                         
*       None
*
*************************************************************************/
VOID ND6_Build_Link_Layer_Opt(UINT8 type, const UINT8 *link_addr, 
                              const NET_BUFFER *buf_ptr, UINT8 link_addr_length)
{
    UINT8   option_length;

    /* Put the type in the packet. */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_OPTION_TYPE_OFFSET, type);
    
    /* Compute the length and insert it into the packet.  The length
     * field is measured in terms of 8 octets.
     */
    option_length = (UINT8)((link_addr_length + IP6_LINK_LAYER_OPT_SIZE) >> 3);

    PUT8(buf_ptr->data_ptr, IP6_ICMP_OPTION_LENGTH_OFFSET, option_length);

    /* Put the link address in the packet. */
    memcpy(buf_ptr->data_ptr + IP6_ICMP_LL_OPTION_ADDRESS_OFFSET, 
           link_addr, link_addr_length);

} /* ND6_Build_Link_Layer_Opt */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6_Process_Link_Layer_Option
*                                                                         
*   DESCRIPTION                                                           
*                            
*       This function processes a received Link-Layer Option.                     
*
*       The Source Link-Layer option contains the link-layer address of
*       the sender of the packet.
*
*   INPUTS                                                                
*
*       *source_addr            A pointer to the Source Address of the 
*                               packet.                            
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       *link_addr              A pointer to the link-layer address 
*                               advertised in the packet.
*       flags                   Flags for the new entry.
*
*   PACKET FORMAT
*
*   0                   1                   2
*   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |      Type     |     Length    |      Link Layer Address ...   |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0                              
*
*************************************************************************/
STATUS ND6_Process_Link_Layer_Option(UINT8 *source_addr, 
                                     DV_DEVICE_ENTRY *device, 
                                     const UINT8 *link_addr, UINT32 flags)
{
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry;

    /* Find a corresponding Neighbor Cache entry */
    nc_entry = device->dev6_fnd_neighcache_entry(device, source_addr);

    if (nc_entry)
    {
        /* RFC 4861 - section 6.3.4 - ... the IsRouter flag in the Neighbor
         * Cache entry MUST be set to TRUE.
         */
        NC6_Transition_To_Router(nc_entry);

        /* If the Neighbor Cache entry is in the INCOMPLETE state. */
        if (nc_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE)
        {
            /* Unset the event to transmit Neighbor Solicitation messages. */
            if (TQ_Timerunset(IP6_Resolve6_Event, TQ_CLEAR_EXACT, 
                              nc_entry->ip6_neigh_cache_resolve_id, 
                              (UNSIGNED)device->dev_index) != NU_SUCCESS)
                NLOG_Error_Log("Failed to stop the Address Resolution timer", 
                               NERR_SEVERE, __FILE__, __LINE__);    

            /* Record the Link-Layer address in the Neighbor Cache entry */
            device->dev6_update_neighcache_link_addr(nc_entry, link_addr);

            /* Transition the Neighbor Cache entry to STALE per Appendix C
             * of RFC 4861.
             */
            nc_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;

            /* Transmit the queued data */
            ND6_Transmit_Queued_Data(nc_entry, device);
        }

        /* RFC 4861 section 6.3.4 - If a cache entry already exists and is
         * updated with a different link-layer address, the reachability state
         * MUST be set to STALE.
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

            /* Set the state of the entry to stale. */
            nc_entry->ip6_neigh_cache_state = NC_NEIGH_STALE;
        }
    }

    /* RFC 4861 section 6.3.4 - If the advertisement contains a Source
     * Link-Layer Address option, the link-layer address SHOULD be recorded
     * in the Neighbor Cache entry for the router (creating an entry if
     * necessary) and the IsRouter flag in the Neighbor Cache entry MUST
     * be set to TRUE.
     */
    else
        device->dev6_add_neighcache_entry(device, source_addr, link_addr, 
                                          (flags | NC_ISROUTER), NU_NULL, 
                                          NC_NEIGH_STALE);

    return (0);

} /* ND6_Process_Link_Layer_Option */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6_Transmit_Queued_Data
*                                                                         
*   DESCRIPTION                                                           
*                            
*       This function transmits queued up data awaiting address 
*       resolution.
*
*   INPUTS                                                                
*
*       *nc_entry               A pointer to the Neighbor Cache entry
*                               holding the queued data to transmit.
*                                                        
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
VOID ND6_Transmit_Queued_Data(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry, 
                              DV_DEVICE_ENTRY *device)
{
    NET_BUFFER                  *current_buffer;
    RTAB6_ROUTE                 iproute;
    SCK6_SOCKADDR_IP            *dest;

    /* Get the first packet from the queue */
    current_buffer = 
        MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);

    if (current_buffer)
    {
        UTL_Zero((CHAR*)&iproute, sizeof(RTAB6_ROUTE));

        /* Point to the destination. */
        dest = &iproute.rt_ip_dest.rtab6_rt_ip_dest;

        /* Send any packets queued for the neighbor awaiting address
         * resolution.
         */
        while (current_buffer)
        {
            dest->sck_family = SK_FAM_IP6;
            dest->sck_len = sizeof(SCK6_SOCKADDR_IP);

            NU_BLOCK_COPY(dest->sck_addr, 
                          &current_buffer->data_ptr[IP6_DESTADDR_OFFSET],
                          IP6_ADDR_LEN);

            IP6_Find_Route(&iproute);

            if (iproute.rt_route)
            {
                /* If the next hop is a gateway then set the destination 
                 * IP address to the gateway. 
                 */
                if (iproute.rt_route->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
                    dest = &(iproute.rt_route->rt_next_hop);

                RTAB_Free((ROUTE_ENTRY*)iproute.rt_route, NU_FAMILY_IP6);
            }

            nc_entry->ip6_neigh_cache_qpkts_count--;

            /* Send the packet */
            if (device->dev_output(current_buffer, device, dest, 
                                   &iproute) != NU_SUCCESS)
            {   
                NLOG_Error_Log("Queued packet awaiting Address Resolution not sent", 
                               NERR_SEVERE, __FILE__, __LINE__);

                /* The packet was not sent. Free the buffers to the 
                 * appropriate list(s).
                 */
                MEM_Multiple_Buffer_Chain_Free(current_buffer);
            }

            /* Get the next packet on the list. */
            current_buffer = 
                MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);
        }

        nc_entry->ip6_neigh_cache_resolve_id = 0;
    }

} /* ND6_Transmit_Queued_Data */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6_Compute_Random_Timeout
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function computes a random value between min_delay
*       and max_delay.
*
*   INPUTS                                                                
*                         
*       min_delay               The minimum value to return.
*       max_delay               The maximum value to return.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       Random value.
*
*************************************************************************/
UINT32 ND6_Compute_Random_Timeout(const UINT32 min_delay, 
                                  const UINT32 max_delay)
{
    return (min_delay + (UTL_Rand() & (max_delay - min_delay)));

} /* ND6_Compute_Random_Timeout */

