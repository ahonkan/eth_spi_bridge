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
*       nd6radv.c                                    
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process an 
*       incoming Router Advertisement message and build and transmit an 
*       outgoing Router Advertisement message.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6RADV_Input
*       ND6RADV_Host_Input
*       ND6RADV_Process_Options
*       ND6RADV_Process_Prefixes
*       ND6RADV_Process_Prefix_Option
*       ND6RADV_Process_MTU_Option
*                                                                          
*   DEPENDENCIES                                                             
*               
*       externs.h
*       externs6.h
*       nd6opts.h
*       nd6.h
*       nd6radv.h
*       defrtr6.h
*       prefix6.h
*       nc6.h
*       nud6.h
*       ip6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/nu_net6.h"
#include "networking/nd6opts.h"
#include "networking/nd6.h"
#include "networking/nd6radv.h"
#include "networking/defrtr6.h"
#include "networking/nc6.h"
#include "networking/nud6.h"

VOID ND6RADV_Host_Input(IP6LAYER *, DV_DEVICE_ENTRY *, 
                        const NET_BUFFER *, union nd_opts *);

extern TQ_EVENT     ICMP6_RtrSol_Event;
extern TQ_EVENT     DEFRTR6_Expire_Entry_Event;
extern TQ_EVENT     PREFIX6_Expire_Entry_Event;
extern TQ_EVENT     NUD6_Check_Neighbors_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function process a Router Advertisement received on the
*       respective interface.  Routers send out Router Advertisement 
*       messages periodically, or in response to a Router Solicitation.
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
*  | Cur Hop Limit |M|O| Reserved  |        Router Lifetime        |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                         Reachable Time                        |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                          Retrans Timer                        |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |    Options ...
*  +-+-+-+-+-+-+-+-+-+-
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0                              
*
*************************************************************************/
STATUS ND6RADV_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                     const NET_BUFFER *buf_ptr)
{
    union   nd_opts             ndopts;
    UINT32                      icmp6len = buf_ptr->mem_total_data_len;

    /* Get the length of the options */
    icmp6len -= IP6_RTR_ADV_HDR_SIZE;

    /* Validate the Router Advertisement */
    if (ND6_Validate_Message(ICMP6_RTR_ADV, pkt->ip6_src, NU_NULL, NU_NULL, 
                             pkt, icmp6len, IP6_RTR_ADV_HDR_SIZE,
                             &ndopts, buf_ptr) == -1)
    {
        MIB_ipv6IfIcmpInErrors_Inc(device);

        NLOG_Error_Log("Router Advertisement silently discarded", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (0);
    }

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

    /* If the interface is a router, check the consistency of the Router
     * Advertisement on the link.
     */
    if (device->dev_flags & DV6_ISROUTER)
        ND6RADV_Router_Input(device, buf_ptr, &ndopts);

    /* Otherwise, process the Router Advertisement */
    else

#endif
        ND6RADV_Host_Input(pkt, device, buf_ptr, &ndopts);

    return (0);

} /* ND6RADV_Input */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Host_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function process a Router Advertisement received on the
*       respective interface for a host interface.
*                                                                         
*   INPUTS                                                                
*           
*       *pkt                    A pointer to the IP header.                 
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       *buf_ptr                A pointer to the buffer.
*       *ndopts                 A pointer to the options data
*                               structure.
*
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
VOID ND6RADV_Host_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                        const NET_BUFFER *buf_ptr, union nd_opts *ndopts)
{
    UINT8                       cur_hop_limit;
    UINT16                      router_lifetime;
    UINT32                      reach_time;
    UINT32                      retrans_timer;
    IP6_DEFAULT_ROUTER_ENTRY    *router_entry;

    /* Extract the Router Lifetime from the packet */
    router_lifetime = GET16(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RTR_LIFETIME_OFFSET);

    /* Search for the address in the Default Router List */
    router_entry = DEFRTR6_Find_Default_Router_Entry(pkt->ip6_src);

    /* If no entry exists and the Router Lifetime is non-zero, create a
     * new entry in the Default Router List.
     */
    if (router_entry == NU_NULL)
    {
        /* If the router lifetime is non-zero, create a new entry for the 
         * source address using the router lifetime value in the packet.
         */
        if (router_lifetime != 0)
        {
            if (DEFRTR6_Create_Default_Router_Entry(device, pkt->ip6_src, 
                                                    router_lifetime) == NU_NULL)
                NLOG_Error_Log("Failed to create a Default Router entry for the new Default Router", 
                               NERR_SEVERE, __FILE__, __LINE__);

#ifdef NET_5_3
            /* RFC 4861 section 6.3.7 - Once the hosts sends a Router Solicitation
             * and receives a valid Router Advertisement with a non-zero Router
             * Lifetime, the host MUST desist from sending additional solicitations
             * on that interface.  However, the node must send at least one 
             * Router Solicitation at start up.
             */
            if (device->dev6_rtr_sols != 0)
#endif
            {
                if (TQ_Timerunset(ICMP6_RtrSol_Event, TQ_CLEAR_ALL_EXTRA, 
                                  device->dev_index, 0) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to stop the timer to transmit another Router Solicitation", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        /* Routes may have been added through this node as a result of a redirect
         * message.  Delete those routes now.
         */
        else
        {
            /* Delete all the routes that use this node as the gateway */
            RTAB6_Delete_Route_By_Gateway(pkt->ip6_src);
        }
    }

    /* Otherwise, if an entry exists, either update the Router Lifetime or 
     * timeout the entry.
     */
    else
    {
        /* If the router lifetime is not zero, update the entry. */
        if (router_lifetime != 0)
        {
            /* Delete the current timer to expire the entry. */
            if (TQ_Timerunset(DEFRTR6_Expire_Entry_Event, TQ_CLEAR_EXACT, 
                              router_entry->ip6_def_rtr_index, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to stop the timer to expire the Default Router entry", 
                               NERR_SEVERE, __FILE__, __LINE__);

            router_entry->ip6_def_rtr_inval_timer = router_lifetime;

            /* Create a new timer to expire the entry */
            if (TQ_Timerset(DEFRTR6_Expire_Entry_Event, 
                            router_entry->ip6_def_rtr_index, 
                            router_entry->ip6_def_rtr_inval_timer * TICKS_PER_SECOND, 
                            0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set the timer to expire the Default Router entry", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        /* If the router lifetime is zero, timeout the entry */
        else
        {
            /* Delete the timer to expire the entry */
            if (TQ_Timerunset(DEFRTR6_Expire_Entry_Event, TQ_CLEAR_EXACT, 
                              router_entry->ip6_def_rtr_index, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to stop the timer to expire the Default Router entry", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* Timeout the entry and return - RFC 4861 section 6.3.5 */
            DEFRTR6_Delete_Entry(router_entry);

            return;
        }
    }

    /* If the Current Hop Limit in the packet is not zero, set the Current
     * Hop Limit of the device to it.
     */
    cur_hop_limit = buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_CURHOPLMT_OFFSET];

    if (cur_hop_limit != 0)
        device->dev6_cur_hop_limit = cur_hop_limit;

    /* If the Reachable Time value in the packet is not zero, set the Base 
     * Reachable Time of the device to it.
     */
    reach_time = GET32(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RCHBL_TIME_OFFSET);

    if (reach_time != 0)
    {
        /* If the Reachable Time value in the packet differs from the device's 
         * Base Reachable Time, set the device's Base Reachable Time according 
         * to the one in the packet and recompute the Reachable Time value 
         * for the device.
         */
        if ( (reach_time < IP6_MAX_REACHABLE_TIME) &&
             (reach_time != device->dev6_base_reachable_time) )
        {
            device->dev6_base_reachable_time = reach_time;

            /* Recompute the dev6_reachable_time for the device */
            device->dev6_reachable_time = (TICKS_PER_SECOND *
                (ICMP6_COMPUTE_RTIME((device->dev6_base_reachable_time / 1000))));

            /* Unset the current timer to expire in the previous reachableTime */
            if (TQ_Timerunset(NUD6_Check_Neighbors_Event, TQ_CLEAR_EXACT, 
                              device->dev_index, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to stop the timer to check neighbors", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* Since the base reachable time just changed, invoke the call to
             * check all neighbors now, so the timer will be reset according
             * to the neighbor that is about to time out the soonest.
             */
            NUD6_Check_Neighbors(device->dev_index);
        }
    }

    /* Set the device's Retransmit Timer to the Retransmit Timer in the packet
     * if the Retransmit Timer in the packet is not zero.
     */
    retrans_timer = GET32(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RTRNS_TMR_OFFSET);

    /* The retransmission timer is in milliseconds.  Convert it to seconds before
     * storing.
     */
    if (retrans_timer != 0)
        device->dev6_retrans_timer = (retrans_timer / 1000) * TICKS_PER_SECOND;

    /* If the Router Advertisement indicates to use DHCPv6 to
     * obtain an IP address, and the device is currently using stateless 
     * configuration, invoke DHCPv6.
     */
    if ((buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET] & 
         IP6_RA_MANAGED_FLAG) >> 4)
    {
        if (!(device->dev_flags & DV6_MGD_FLAG))
        {
            device->dev_flags |= DV6_MGD_FLAG;
    
#if (INCLUDE_DHCP6 == NU_TRUE)

            /* Set the event to invoke DHCPv6. */
            if (EQ_Put_Event(DHCP6_Stateful_Config_Event, device->dev_index, 
                             0) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set event to invoke DHCPv6.",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
#endif
        }
    }
    else if ((buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET] & 
         IP6_RA_CONFIG_FLAG) >> 4)
    {
        if (!(device->dev_flags & DV6_CFG_FLAG))
        {
            device->dev_flags |= DV6_CFG_FLAG;
    
#if (INCLUDE_DHCP6 == NU_TRUE)

            /* Set the event to invoke DHCPv6. */
            if (EQ_Put_Event(DHCP6_Stateless_Config_Event, device->dev_index, 
                             0) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set event to invoke DHCPv6.",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
#endif
        }
    }

    /* Otherwise, the Router Advertisement indicates to not use DHCPv6.
     * Set the flag accordingly, but continue running DHCPv6.
     */
    else
        device->dev_flags &= ~DV6_MGD_FLAG;

    /* If the Router Advertisement indicates to use DHCPv6 to
     * obtain non-IP address related configuration information, and the 
     * device is currently using stateless configuration, invoke DHCPv6.
     */
    if ((buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET] & 
         IP6_RA_CONFIG_FLAG) >> 4)
    {
        if (!(device->dev_flags & DV6_CFG_FLAG))
        {
            device->dev_flags |= DV6_CFG_FLAG;
        }
    }

    /* Otherwise, the Router Advertisement indicates not to use DHCPv6.
     * Set the flag accordingly, but continue running DHCPv6.
     */
    else
        device->dev_flags &= ~DV6_CFG_FLAG;

    /* Process any options included in the packet */
    if (ND6RADV_Process_Options(device, pkt->ip6_src, ndopts, 
                                router_entry, buf_ptr) != NU_SUCCESS)
        NLOG_Error_Log("Error processing Router Advertisement options", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

} /* ND6RADV_Host_Input */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Process_Options
*                                                                         
*   DESCRIPTION                                                           
*                      
*       This function processes the Prefix Options associated with a
*       received Router Advertisement.                                                   
*                                                                         
*   INPUTS                                                                
*                                     
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       *source_addr            The source address of the incoming packet.
*       *ndopts                 A pointer to the parsed out options.
*       *router_entry           A pointer to the Router entry associated
*                               with the router that sent this 
*                               advertisement.
*       *buf_ptr				A pointer to the incoming buffer.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0                              
*
*************************************************************************/
STATUS ND6RADV_Process_Options(DV_DEVICE_ENTRY *device, UINT8 *source_addr, 
                               union nd_opts *ndopts, 
                               IP6_DEFAULT_ROUTER_ENTRY *router_entry,
                               const NET_BUFFER *buf_ptr)
{
    UINT8                       *link_addr;
    STATUS                      status = NU_SUCCESS;
    UINT32                      updated_mtu;    
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry;

    UNUSED_PARAMETER(router_entry);

    /* If the packet was not received on a virtual device. */
    if (!(device->dev_flags & DV6_VIRTUAL_DEV))
    {
        /* Process the Source Link-Layer Address option if present. */
        if (ndopts->nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR])
        {
            /* Extract the link-layer address from the packet */
            link_addr = (UINT8*)((UNSIGNED)(ndopts->nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR]) +
                        IP6_ICMP_LL_OPTION_ADDRESS_OFFSET);

            /* Process the link-layer option. */
            status = ND6_Process_Link_Layer_Option(source_addr, device, link_addr,
                                                   NC_ISROUTER);
        }

        /* If the Advertisement does not contain a Source Link-Layer option, but a
         * Neighbor Cache entry exists, set the IsRouter flag of the entry to TRUE.
         */   
        else if ( ((nc_entry = device->dev6_fnd_neighcache_entry(device, 
                                                                 source_addr)) 
                                                                 != NU_NULL) )
            NC6_Transition_To_Router(nc_entry);
    }

    /* Process any Prefix options present */
    if (ndopts->nd_opt_array[IP6_ICMP_OPTION_PREFIX])
    {
        ND6RADV_Process_Prefixes(device, ndopts, router_entry, buf_ptr);
    }

    /* Process the MTU option if present */
    if ( (ndopts->nd_opt_array[IP6_ICMP_OPTION_MTU]) &&
         (ndopts->nd_opt_array[IP6_ICMP_OPTION_MTU]->nd_opt_len == 1) )
    {
        /* Extract the MTU */
        updated_mtu = GET32((UINT8*)(ndopts->nd_opt_array[IP6_ICMP_OPTION_MTU]), 
                            IP6_ICMP_MTU_OFFSET);

        /* Update the MTU accordingly */
        if (ND6RADV_Process_MTU_Option(device, updated_mtu) != NU_SUCCESS)
            NLOG_Error_Log("Failed to update the MTU with the advertised MTU", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* ND6RADV_Process_Options */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Process_Prefixes
*                                                                         
*   DESCRIPTION                                                           
*                            
*       This function loops through the Prefix Options, processing
*       each one in the packet.
*                                                                         
*   INPUTS                                                                
*                            
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       *ndopts                 The ndopts data structure that contains
*                               the Prefix Options.
*       *defrtr_ptr             A pointer to the Default Router entry
*                               that sent this Prefix List.
*       *buf_ptr				A pointer to the incoming buffer.
*                                        
*   OUTPUTS                                                               
*                                           
*       None.
*
*************************************************************************/
VOID ND6RADV_Process_Prefixes(DV_DEVICE_ENTRY *device, 
                              const union nd_opts *ndopts,
                              IP6_DEFAULT_ROUTER_ENTRY *defrtr_ptr,
                              const NET_BUFFER *buf_ptr)
{
    struct  nd_opt_hdr          *pt;
    struct  nd_opt_prefix_info  *pi;
    UINT16                      option_length;
    UINT8                       prefix_length;
    UINT32                      valid_lifetime;
    UINT32                      preferred_lifetime;
    UINT8                       flags;

    UNUSED_PARAMETER(defrtr_ptr);

    /* Process each Prefix Option present */
    for (pt = (struct nd_opt_hdr *)ndopts->nd_opts_pi;
         pt <= (struct nd_opt_hdr *)ndopts->nd_opts_pi_end;
         pt = (struct nd_opt_hdr *)((UNSIGNED)pt + option_length))
    {
        /* Get the length of the current option */
        option_length = (UINT16)(pt->nd_opt_len << 3);

        /* Get a pointer to the prefix data structure */
        pi = (struct nd_opt_prefix_info *)pt;

        /* Extract the length of the prefix from the packet */
        prefix_length = (UINT8)(pi->nd_opt_pi_prefix_len);

        /* Extract the valid and preferred lifetime from the packet */
        valid_lifetime = GET32(pi, IP6_ICMP_PREFIX_VALID_LIFETIME_OFFSET);

        preferred_lifetime = GET32(pi, IP6_ICMP_PREFIX_PREF_LIFETIME_OFFSET);

        flags = pi->nd_opt_pi_flags_reserved;

        if (ND6RADV_Process_Prefix_Option(device, 
                                          pi->nd_opt_pi_prefix.is_ip_addrs, 
                                          prefix_length, valid_lifetime,
                                          preferred_lifetime, flags,
                                          buf_ptr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to update the Prefix List with the advertised Prefix", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

} /* ND6RADV_Process_Prefixes */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Process_Prefix_Option
*                                                                         
*   DESCRIPTION                                                           
*                            
*       This function processes a Prefix Option received from a 
*       Router Advertisement.                                             
*
*       The Prefix Information option provides hosts with on-link prefixes
*       and prefixes for Address Autoconfiguration.
*                                                                         
*   INPUTS                                                                
*                            
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       *prefix                 A pointer to the advertised prefix.
*       prefix_length           The length (in bits) of the advertised 
*                               prefix.
*       valid_lifetime          The Valid Lifetime of the advertised 
*                               prefix.
*       preferred_lifetime      The Preferred Lifetime of the advertised
*                               prefix.
*       flag                    Indicates whether the On-Link and/or 
*                               Auto flags were set in the packet.  
*       *buf_ptr				A pointer to the incoming buffer.
*                                        
*   PACKET FORMAT
*
*   0                   1                   2
*   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |      Type     |     Length    | Prefix Length |L|A| Reserved1 |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                         Valid Lifetime                        |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                       Preferred Lifetime                      |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                           Reserved2                           |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +                          Prefix                               +
*  |                                                               |
*  +                                                               +
*  |                                                               |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*                                 
*   OUTPUTS                                                               
*                                           
*       -1                      The option is invalid.
*       0                       The option was successfully processed.
*
*************************************************************************/
STATUS ND6RADV_Process_Prefix_Option(DV_DEVICE_ENTRY *device, const UINT8 *prefix, 
                                     UINT8 prefix_length, UINT32 valid_lifetime,
                                     UINT32 preferred_lifetime, UINT8 flag,
                                     const NET_BUFFER *buf_ptr)
{
    IP6_PREFIX_ENTRY    *prefix_entry;
    STATUS              status = NU_SUCCESS;
    DEV6_IF_ADDRESS     *target_address;
    UINT32              flags = 0;

    /* RFC 4862 section 5.5.3.b - If the prefix is the link-local prefix or a
     * multicast, silently discard the option.
     */
    if ( (IPV6_IS_ADDR_LINKLOCAL(prefix)) || (IPV6_IS_ADDR_MULTICAST(prefix)) )
    {
        NLOG_Error_Log("Discarded Prefix because prefix is the link-local prefix or multicast", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (-1);
    }

    /* RFC 4862 - section 5.5.3.a - If the Autonomous flag is not set, silently
     * ignore the Prefix Information option.
     */
    if (!(flag & IP6_PREFIX_FLAG_AUTO))
    {
        NLOG_Error_Log("Discarded Prefix because the Autonomous Flag is not set", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (-1);
    }

    /* RFC 4862 - section 5.5.3.c - If the Preferred Lifetime is greater than
     * the Valid Lifetime, silently ignore the Prefix Option.
     */
    if (preferred_lifetime > valid_lifetime)
    {
        NLOG_Error_Log("Discarded Prefix because the Preferred lifetime is greater than the Valid lifetime", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (0);
    }

    target_address = device->dev6_addr_list.dv_head;

    /* Determine if there is an address on this interface configured via
     * stateless address autoconfiguration using this advertised prefix.
     */
    while (target_address)
    {
        if ( (!(target_address->dev6_addr_state & DV6_DUPLICATED)) &&
             (memcmp(target_address->dev6_ip_addr, prefix,
                     target_address->dev6_prefix_length >> 3) == 0) &&
             (target_address->dev6_addr_flags & ADDR6_STATELESS_AUTO) )
        {
        	/* Get a pointer to the prefix entry used to create this address. */
        	prefix_entry = PREFIX6_Find_Prefix_List_Entry(device, prefix);

        	break;
        }

        target_address = target_address->dev6_next;
    }

    /* RFC 4862 section 5.5.3.e - If the advertised prefix is equal to the
     * prefix of an address configured by stateless autoconfiguration in the
     * list, the preferred lifetime of the address is reset to the Preferred
     * Lifetime in the received advertisement.
     */
    if  ( (target_address) && (prefix_entry) )
    {
        /* If the new valid lifetime is zero, timeout the Prefix Entry */
        if (valid_lifetime == 0)
        {
            /* Clear the current timer to expire the entry */
            if (TQ_Timerunset(PREFIX6_Expire_Entry_Event, TQ_CLEAR_EXACT, 
                              prefix_entry->ip6_prfx_lst_index, 
                              (UNSIGNED)(prefix_entry->ip6_prefx_lst_device->dev_index))
                              != NU_SUCCESS)
                NLOG_Error_Log("Failed to stop the timer to expire the Prefix entry", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* Delete the entry */
            PREFIX6_Delete_Entry(prefix_entry);

            return (0);
        }

        /* If the received Valid Lifetime is greater than 2 hours or greater
         * than RemainingLifetime, set the valid lifetime of the corresponding
         * address to the advertised Valid Lifetime.
         *
         * The IPv6 Certification test appears to expect the Stored Valid Lifetime
         * to decrement in real-time.  So we store the the time at which valid
         * lifetime is stored in another variable lst_stored_lie and subtract that
         * value from the current time.  This would give us the amount of time
         * expired since we last received an advertisement with valid lifetime.
         */
        else if ( (valid_lifetime > (IP6_DEFAULT_VALID_LIFETIME / SCK_Ticks_Per_Second)) ||
                  (valid_lifetime >= prefix_entry->ip6_prfx_lst_valid_life) ||
                  (valid_lifetime >= (prefix_entry->ip6_prfx_lst_valid_life - 
                  ((NU_Retrieve_Clock()- prefix_entry->ip6_prfx_lst_stored_life)/(SCK_Ticks_Per_Second)))))
        {
            /* Clear the current timer to expire the entry */
            if (prefix_entry->ip6_prfx_lst_valid_life != 0xffffffffUL)            
            {
                if (TQ_Timerunset(PREFIX6_Expire_Entry_Event, TQ_CLEAR_EXACT, 
                                  prefix_entry->ip6_prfx_lst_index, 
                                  (UNSIGNED)(prefix_entry->ip6_prefx_lst_device->dev_index))
                                  != NU_SUCCESS)
                    NLOG_Error_Log("Failed to stop the timer to expire the Prefix entry", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            prefix_entry->ip6_prfx_lst_valid_life = valid_lifetime;

            prefix_entry->ip6_prfx_lst_valid_life_exp = 
                (valid_lifetime * SCK_Ticks_Per_Second) + NU_Retrieve_Clock();

            prefix_entry->ip6_prfx_lst_stored_life = NU_Retrieve_Clock();

            /* Set a new timer to expire the entry in the specified valid lifetime */
            if (prefix_entry->ip6_prfx_lst_valid_life != 0xffffffffUL)
            {
                if (TQ_Timerset(PREFIX6_Expire_Entry_Event, 
                                prefix_entry->ip6_prfx_lst_index, 
                                prefix_entry->ip6_prfx_lst_valid_life * TICKS_PER_SECOND, 
                                (UNSIGNED)(prefix_entry->ip6_prefx_lst_device->dev_index)) 
                                != NU_SUCCESS)
                    NLOG_Error_Log("Failed to set the timer to expire the Prefix entry", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Update the valid lifetime and preferred lifetime of the address */
            if ( (flag & IP6_PREFIX_FLAG_AUTO) &&
                 (!(device->dev6_flags & DV6_DISABLE_ADDR_CONFIG)) )
            {
                DEV6_Update_Address_Entry(device, target_address, 
                                          valid_lifetime, preferred_lifetime);
            }
        }

        /* If RemainingLifetime is less than or equal to 2 hours, ignore
         * the Prefix Information option with regards to the valid lifetime,
         * unless the Router Advertisement from which this option was
         * obtained has been authenticated (e.g., via Secure Neighbor
         * Discovery [RFC3971]).  If the Router Advertisement was
         * authenticated, the valid lifetime of the corresponding address
         * should be set to the Valid Lifetime in the received option.
         */
        else if (prefix_entry->ip6_prfx_lst_valid_life <= (IP6_DEFAULT_VALID_LIFETIME / SCK_Ticks_Per_Second))
        {
        	/* RFC 4862 section 5.5.3 - Note that the preferred lifetime of the
        	 * corresponding address is always reset to the Preferred Lifetime
        	 * in the received Prefix Information option, regardless of whether
        	 * the valid lifetime is also reset or ignored.
          	 */
            if ( (flag & IP6_PREFIX_FLAG_AUTO) &&
                 (!(device->dev6_flags & DV6_DISABLE_ADDR_CONFIG)) )
            {
                DEV6_Update_Address_Entry(device, target_address, 0, preferred_lifetime);
            }

            /* MGC_SECURITY_RELEASE */
            /* If the Router Advertisement was authenticated, set the stored
             * Valid Lifetime to the received Valid Lifetime.
             */
        }

        /* Otherwise, reset the valid lifetime of the corresponding address
         * to 2 hours.
         */
        else
        {
            /* Clear the current timer to expire the entry */
            if (prefix_entry->ip6_prfx_lst_valid_life != 0xffffffffUL)
            {
                if (TQ_Timerunset(PREFIX6_Expire_Entry_Event, TQ_CLEAR_EXACT, 
                                  prefix_entry->ip6_prfx_lst_index, 
                                  (UNSIGNED)(prefix_entry->ip6_prefx_lst_device->dev_index))
                                  != NU_SUCCESS)
                    NLOG_Error_Log("Failed to stop the timer to expire the Prefix entry", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            prefix_entry->ip6_prfx_lst_valid_life = (IP6_DEFAULT_VALID_LIFETIME / SCK_Ticks_Per_Second);

            prefix_entry->ip6_prfx_lst_valid_life_exp = 
                IP6_DEFAULT_VALID_LIFETIME + NU_Retrieve_Clock();

            /* Set a new timer to expire the entry in the specified valid lifetime */
            if (TQ_Timerset(PREFIX6_Expire_Entry_Event, prefix_entry->ip6_prfx_lst_index, 
                             prefix_entry->ip6_prfx_lst_valid_life, 
                             (UNSIGNED)(prefix_entry->ip6_prefx_lst_device->dev_index)) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set the timer to expire the Prefix entry", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* Update the valid lifetime of the address */
            if ( (flag & IP6_PREFIX_FLAG_AUTO) &&
                 (!(device->dev6_flags & DV6_DISABLE_ADDR_CONFIG)) )
            {
                DEV6_Update_Address_Entry(device, target_address, 
                                          (IP6_DEFAULT_VALID_LIFETIME / SCK_Ticks_Per_Second),
                                          preferred_lifetime);
            }
        }
    }

    /* RFC 4862 section 5.5.3.d - If the prefix advertised is not equal to the
     * prefix of an address configured by stateless autoconfiguration already in
     * the list of addresses associated with the interface (where "equal" means
     * the two prefix lengths are the same and the first prefix-length bits of
     * the prefixes are identical), and if the Valid Lifetime is not 0, form an
     * address (and add it to the list) by combining the advertised prefix with
     * an interface identifier of the link.
     */
    else
    {
        /* If the Valid Lifetime is not 0, create a new entry for the Prefix */
        if (valid_lifetime != 0)
        {
            /* Create a new prefix entry. */
            if (PREFIX6_New_Prefix_Entry(device, prefix, prefix_length, 
                                         valid_lifetime, preferred_lifetime, 
                                         flags) == NU_SUCCESS)
            {
                /* If the Autonomous flag is set and address autoconfiguration
                 * has not been disabled on the interface. 
                 */
                if ( (flag & IP6_PREFIX_FLAG_AUTO) && 
                     (!(device->dev6_flags & DV6_DISABLE_ADDR_CONFIG)) )
                {
                    /* If the sum of the prefix length and the interface 
                     * identifier length does not equal 128 bits, the Prefix 
                     * Information option must not be used to create an
                     * address for the interface. 
                     */
                    if ( (prefix_length + 
                         (device->dev6_interface_id_length << 3)) != (IP6_ADDR_LEN << 3) )
                    {
                        NLOG_Error_Log("Did not create address from prefix since prefix length plus interface ID exceed 128 bits", 
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);
                    }

                    else
                    {
                    	/* Set the flag indicating that this address is being created
                    	 * using Stateless Address Autoconfiguration.
                    	 */
                    	flags |= ADDR6_STATELESS_AUTO;

                    	/* If the packet was sent to a multicast address. */
                    	if (buf_ptr->mem_flags & NET_MCAST)
                    	{
                    		/* Set the flag indicating that the address is being created
                    		 * from a multicast RA, so delay joining the solicited-node
                    		 * multicast address.
                    		 */
                    		flags |= ADDR6_DELAY_MLD;
                    	}

                        /* Create an address from the prefix. */
                        status = DEV6_Create_Address_From_Prefix(device, prefix, 
                                                                 prefix_length, 
                                                                 preferred_lifetime,
                                                                 valid_lifetime,
                                                                 flags);

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to add the new IPv6 address to the list of addresses for the device", 
                                           NERR_SEVERE, __FILE__, __LINE__);   
                        }
                    }
                }
            }

            else
                NLOG_Error_Log("Failed to add the Prefix to the Prefix List", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        /* The Valid Lifetime field is zero, and the Prefix does not already
         * exist in the Prefix List - ignore the packet.
         */
        else
        {
            NLOG_Error_Log("Discarded Prefix because the Valid Lifetime is zero and the Prefix does not exist in the Prefix List", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (-1);
        }
    }

    return (status);

} /* ND6RADV_Process_Prefix_Option */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Process_MTU_Option
*                                                                         
*   DESCRIPTION                                                           
*                            
*       This function processes an MTU Option received from a 
*       Router Advertisement.     
*
*       The MTU option is used in Router Advertisement messages to insure
*       that all nodes on a link use the same MTU value in those cases
*       where the link MTU is not well known.
*
*       In configurations in which heterogeneous technologies are bridged
*       together, the maximum supported MTU may differ from one segment
*       to another.  If the bridges do not generate ICMP Packet Too
*       Big Messages, communicating nodes will be unable to use Path
*       MTU to dynamically determine the appropriate MTU on a per-neighbor
*       basis.  In such cases, routers use the MTU option to specify the
*       maximum MTU value that is supported by all segments.
*                                                                         
*   INPUTS                                                                
*                            
*       *device                 A pointer to the device on which the 
*                               Router Advertisement was received.
*       link_mtu                The Link MTU advertised in the packet.
*
*   PACKET FORMAT
*
*   0                   1                   2
*   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |      Type     |     Length    |            Reserved           |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                              MTU                              |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0                       The link MTU was updated to the new MTU 
*                               value.
*       -1                      The new MTU value was invalid for the 
*                               link.
*
*************************************************************************/
STATUS ND6RADV_Process_MTU_Option(DV_DEVICE_ENTRY *device, UINT32 link_mtu)
{
    /* RFC 4861 section 6.3.4 - If the MTU option is present, hosts SHOULD
     * copy the option's value into LinkMTU so long as the value is greater 
     * than or equal to the minimum link MTU [IPv6] and does not exceed 
     * the default LinkMTU value specified in the link type specific 
     * document.
     */
    if ( (link_mtu >= IP6_MIN_LINK_MTU) && 
         (link_mtu <= device->dev6_default_mtu) )
    {
        device->dev6_link_mtu = link_mtu;
        return (0);
    }
    else
        return (-1);

} /* ND6RADV_Process_MTU_Option */
