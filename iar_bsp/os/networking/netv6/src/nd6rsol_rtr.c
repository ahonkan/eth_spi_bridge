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
*       nd6rsol_rtr.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process an 
*       incoming Router Solicitation for an interface acting as an IPv6
*       router.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6RSOL_Input
*                                                                          
*   DEPENDENCIES                                                             
*               
*       nu_net.h
*       nd6.h
*       nc6.h
*       nd6rsol.h
*       nd6radv.h
*                                                                          
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/nd6.h"
#include "networking/nc6.h"
#include "networking/nd6rsol.h"
#include "networking/nd6radv.h"

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

extern TQ_EVENT     IP6_Transmit_Rtr_Adv_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RSOL_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function process a Router Solicitation received on the
*       respective interface.  Hosts send Router Solicitations in order
*       to prompt routers to generate Router Advertisements quickly.
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
*  |                            Reserved                           |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |    Options ...
*  +-+-+-+-+-+-+-+-+-+-
*                                                
*   OUTPUTS                                                               
*                                           
*       0                              
*
*************************************************************************/
STATUS ND6RSOL_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                     const NET_BUFFER *buf_ptr)
{
    UINT32                      icmp6len = buf_ptr->mem_total_data_len;
    UINT32                      rtr_adv_timer;
    union   nd_opts             ndopts;
    UINT8                       *link_addr;
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry;

    /* If the interface the node received the Solicitation on is not a 
     * Router Interface, silently discard the packet.
     */
    if (!(device->dev_flags & DV6_ISROUTER))
        return (0);

    /* Get the length of the options */
    icmp6len -= IP6_RTR_SOL_HDR_SIZE;

    /* Validate the Router Solicitation - if invalid, silently discard
     * the packet.
     */
    if (ND6_Validate_Message(ICMP6_RTR_SOL, pkt->ip6_src, pkt->ip6_dest, 
                             NU_NULL, pkt, icmp6len, IP6_RTR_SOL_HDR_SIZE,
                             &ndopts, buf_ptr) == -1)
    {
        MIB_ipv6IfIcmpInErrors_Inc(device);

        NLOG_Error_Log("Router Solicitation packet silently discarded", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return (0);
    }

    /* RFC 4861 - section 6.2.6 - Upon receipt of a Router Solicitation,
     * compute a random delay within the range 0 through MAX_RA_DELAY_TIME. 
     */
    rtr_adv_timer = ND6_Compute_Random_Timeout(0, IP6_MAX_RA_DELAY_TIME);

#ifdef NET_5_3

    /* RFC 4861 - section 6.2.6 - If the router sent a multicast Router
     * Advertisement (solicited or unsolicited) within the last 
     * IP6_MIN_DELAY_BETWEEN_RAS seconds, schedule the advertisement to 
     * be sent at a time corresponding to IP6_MIN_DELAY_BETWEEN_RAS plus 
     * the random value after the previous advertisement was sent.  
     * This ensures that the multicast Router Advertisements are rate 
     * limited.
     */
    if (TQ_Check_Duetime(device->dev6_last_radv_time + 
                         IP6_MIN_DELAY_BETWEEN_RAS) > 0)
    {
        rtr_adv_timer = rtr_adv_timer+ IP6_MIN_DELAY_BETWEEN_RAS;
        
        /* Set next Adv. time based on time when LAST Adv. was sent. */
        rtr_adv_timer -= (NU_Retrieve_Clock() - device->dev6_last_radv_time);
    }

#endif

    /* If the next Router Advertisement will be sent in greater than the 
     * computed delay, clear the timer running for the next Router 
     * Advertisement, and set a new timer with the lower delay.
     */
    if (device->dev6_next_radv_time > (NU_Retrieve_Clock() + rtr_adv_timer))
    {
        /* Unset the interval timer to transmit the next unsolicited 
         * Router Advertisement, as receipt of this Router Solicitation will 
         * invoke transmission of a Router Advertisement to the All-Nodes 
         * Multicast Address.
         */
        if (TQ_Timerunset(IP6_Transmit_Rtr_Adv_Event, TQ_CLEAR_ALL_EXTRA, 
                          device->dev_index, 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to stop the timer to transmit the next Router Advertisement", 
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Set the timer to transmit the next Router Advertisement */
        if (TQ_Timerset(IP6_Transmit_Rtr_Adv_Event, device->dev_index, 
                        rtr_adv_timer, ND6RADV_SOL_ADV) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set timer to transmit Router Advertisement", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Process the Source Link-Layer Address option if present. RFC 4861 -
     * section 6.2.6 - Router Solicitations in which the Source Address is 
     * the unspecified address MUST NOT update the router's Neighbor Cache.
     */
    if (!(IPV6_IS_ADDR_UNSPECIFIED(pkt->ip6_src)))
    {
        /* If the Source Link-Layer Address option is present */
        if (ndopts.nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR])
        {
            /* Extract the link-layer address from the packet */
            link_addr = (UINT8*)((UNSIGNED)(ndopts.nd_opt_array[IP6_ICMP_OPTION_SRC_ADDR]) +
                        IP6_ICMP_LL_OPTION_ADDRESS_OFFSET);

            /* Process the link-layer option. */
            ND6_Process_Link_Layer_Option(pkt->ip6_src, device, link_addr, 0);
        }

        /* RFC 4861 - section 6.2.6 - Whether or not a Source Link-Layer Address
         * option is provided, if a Neighbor Cache entry for the solicitation's 
         * sender exists (or is created) the entry's IsRouter flag MUST be set to
         * FALSE.
         */   
        nc_entry = device->dev6_fnd_neighcache_entry(device, pkt->ip6_src);

        if (nc_entry)
            nc_entry->ip6_neigh_cache_flags &= ~NC_ISROUTER;
    }

    return (0);

} /* ND6RSOL_Input */

#endif
