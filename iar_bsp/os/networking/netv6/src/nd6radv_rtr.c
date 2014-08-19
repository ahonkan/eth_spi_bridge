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
*       nd6radv_rtr.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process an 
*       incoming Router Advertisement message and build and transmit an 
*       outgoing Router Advertisement message for an interface that is
*       acting as an IPv6 router.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6RADV_Router_Input
*       ND6RADV_Handle_Event
*       ND6RADV_Output
*       ND6RADV_Build
*       ND6RADV_Build_MTU_Option
*       ND6RADV_Build_Prefix_Option
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
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/externs6.h"
#include "networking/nd6opts.h"
#include "networking/nd6.h"
#include "networking/nd6radv.h"
#include "networking/defrtr6.h"
#include "networking/prefix6.h"
#include "networking/nc6.h"
#include "networking/nud6.h"

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

STATIC NET_BUFFER *ND6RADV_Build(const DV_DEVICE_ENTRY *, const UINT8 *);
STATIC VOID ND6RADV_Build_MTU_Option(const DV_DEVICE_ENTRY *, 
                                     const NET_BUFFER *);
STATIC VOID ND6RADV_Build_Prefix_Option(const DV_DEVICE_ENTRY *, 
                                        const NET_BUFFER *);
STATIC VOID ND6RADV_Add_Prefix(const NET_BUFFER *, UINT8 *, UINT8, 
                               UINT32, UINT32, UINT8, INT);

extern UINT8        IP6_All_Nodes_Multi[];
extern TQ_EVENT     IP6_Transmit_Rtr_Adv_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Router_Input
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function process a Router Advertisement received on the
*       respective interface for a router interface.
*                                                                         
*   INPUTS                                                                
*           
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
VOID ND6RADV_Router_Input(const DV_DEVICE_ENTRY *device, 
                          const NET_BUFFER *buf_ptr, const union nd_opts *ndopts)
{
    UINT8                       cur_hop_limit, flag;
    UINT16                      option_length;
    UINT32                      reach_time, retrans_timer, updated_mtu;
    IP6_PREFIX_ENTRY            *prefix_entry;
    struct  nd_opt_hdr          *pt;
    struct  nd_opt_prefix_info  *pi;

    /* Extract the Cur Hop Limit value from the packet */
    cur_hop_limit = buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_CURHOPLMT_OFFSET];

    /* If the value is not zero, validate the Cur Hop Limit value */
    if (cur_hop_limit != 0)
    {
        if (cur_hop_limit != device->dev6_AdvCurHopLimit)
            NLOG_Error_Log("Received CurHopLimit in Router Advertisement Inconsistent", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);               
    }

    /* Extract the value of the M flag */
    flag = (UINT8)((buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET] & 
                    IP6_RA_MANAGED_FLAG) >> 4);

    /* If the M bit in the Router Advertisement does not match the M
     * bit on the interface, log an error.
     */
    if ( ((flag) && (!(device->dev_flags & DV6_ADV_MGD))) ||
         ((!flag) && (device->dev_flags & DV6_ADV_MGD)) )
        NLOG_Error_Log("Received M-bit in Router Advertisement Inconsistent", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);               

    /* Extract the value of the O flag */
    flag = (UINT8)((buf_ptr->data_ptr[IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET] & 
                    IP6_RA_CONFIG_FLAG) >> 4);

    /* If the O bit in the Router Advertisement does not match the O
     * bit on the interface, log an error.
     */
    if ( ((flag) && (!(device->dev_flags & DV6_ADV_OTH_CFG))) ||
         ((!flag) && (device->dev_flags & DV6_ADV_OTH_CFG)) )
        NLOG_Error_Log("Received O-bit in Router Advertisement Inconsistent", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);               

    /* Extract the Reachable Time from the packet */
    reach_time = GET32(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RCHBL_TIME_OFFSET);

    /* If the value is not zero, validate the Reachable Time value */
    if (reach_time != 0)
    {
        if (reach_time != device->dev6_AdvReachableTime)
            NLOG_Error_Log("Received Reachable Time in Router Advertisement Inconsistent", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);               
    }

    /* Extract the Retrans Timer from the packet */
    retrans_timer = GET32(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RTRNS_TMR_OFFSET);

    /* If the value is not zero, validate the Retrans Timer value */
    if (retrans_timer != 0)
    {
        if (retrans_timer != device->dev6_AdvRetransTimer)
            NLOG_Error_Log("Received Reachable Time in Router Advertisement Inconsistent", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);               
    }

    /* Validate the MTU option */
    if ( (device->dev6_AdvLinkMTU) && 
         (ndopts->nd_opt_array[IP6_ICMP_OPTION_MTU]) &&
         (ndopts->nd_opt_array[IP6_ICMP_OPTION_MTU]->nd_opt_len == 1) )
    {
        /* Extract the MTU */
        updated_mtu = GET32((UINT8*)(ndopts->nd_opt_array[IP6_ICMP_OPTION_MTU]), 
                            IP6_ICMP_MTU_OFFSET);

        if (updated_mtu != device->dev6_AdvLinkMTU)
            NLOG_Error_Log("Received MTU in Router Advertisement Inconsistent", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);               
    }

    /* Validate the Prefix options */
    if (ndopts->nd_opt_array[IP6_ICMP_OPTION_PREFIX])
    {
        /* Process each Prefix Option present */
        for (pt = (struct nd_opt_hdr *)ndopts->nd_opts_pi;
             pt <= (struct nd_opt_hdr *)ndopts->nd_opts_pi_end;
             pt = (struct nd_opt_hdr *)((UNSIGNED)pt + option_length))
        {
            /* Get the length of the current option */
            option_length = (UINT16)(pt->nd_opt_len << 3);

            /* Get a pointer to the prefix data structure */
            pi = (struct nd_opt_prefix_info *)pt;

            prefix_entry = PREFIX6_Find_Prefix_List_Entry(device, 
                                                          pi->nd_opt_pi_prefix.is_ip_addrs);

            /* If the interface is advertising the Prefix too */
            if (prefix_entry)
            {
                /* If the Preferred Lifetime does not decrement in real-time,
                 * check the consistency of the Preferred Lifetime.
                 */
                if (!(prefix_entry->ip6_prfx_lst_flags & PRFX6_DEC_PREF_LIFE))
                {
                    if (GET32(pi, IP6_ICMP_PREFIX_PREF_LIFETIME_OFFSET) !=
                        prefix_entry->ip6_prfx_lst_pref_life)
                        NLOG_Error_Log("Received Preferred Lifetime in Router Advertisement Inconsistent", 
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);             
                }

                /* If the Valid Lifetime does not decrement in real-time,
                 * check the consistency of the Valid Lifetime.
                 */
                if (!(prefix_entry->ip6_prfx_lst_flags & PRFX6_DEC_VAL_LIFE))
                {
                    if (GET32(pi, IP6_ICMP_PREFIX_VALID_LIFETIME_OFFSET) !=
                        prefix_entry->ip6_prfx_lst_valid_life)
                        NLOG_Error_Log("Received Valid Lifetime in Router Advertisement Inconsistent", 
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);             
                }
            }
        }
    }

} /* ND6RADV_Router_Input */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Handle_Event
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function handles a Router Advertisement event.
*                                                                         
*   INPUTS                                                                
*           
*       event                   Unused.
*       *device                 The device index of the device out 
*                               which to transmit the Router Advertisement.
*       resp_type               Flag to determine whether the Router
*                               Advertisement is solicited or 
*                               unsolicited.
*
*   OUTPUTS                                                               
*                                           
*       None.
*
*************************************************************************/
VOID ND6RADV_Handle_Event(TQ_EVENT event, UNSIGNED dev_index, 
                          UNSIGNED resp_type)
{
    DV_DEVICE_ENTRY     *device;
    UINT32              rtr_adv_timer;

    UNUSED_PARAMETER(event);

    /* Get a pointer to the device */
    device = DEV_Get_Dev_By_Index(dev_index);

    /* If the device is valid, transmit a Router Advertisement out the
     * specified device.
     */
    if (device)
    {
        /* Transmit the Router Advertisement */
        ND6RADV_Output(device);

        /* Compute a random delay for transmitting the next Router
         * Advertisement.
         */
        rtr_adv_timer = 
            ND6_Compute_Random_Timeout(device->dev6_MinRtrAdvInterval,
                                       device->dev6_MaxRtrAdvInterval);
        
#ifdef NET_5_3

        device->dev6_init_max_RtrAdv++;

        if (device->dev6_init_max_RtrAdv <= IP6_MAX_NEIGHBOR_ADVERTISEMENT)
        {
            if (rtr_adv_timer > IP6_MAX_INITIAL_RTR_ADVERT_INTERVAL)
                rtr_adv_timer = IP6_MAX_INITIAL_RTR_ADVERT_INTERVAL;
        }
        else
#endif
        {
            /* If a Router Advertisement is being transmitted due to a Router
             * Solicitation and the last unsolicited Router Advertisement
             * was transmitted less than the minimum advertisement value for the 
             * interface seconds ago, reset the new timer value to the minimum 
             * advertisement value for the interface.
             */
            if ( (resp_type == ND6RADV_SOL_ADV) &&
                 ((((rtr_adv_timer * SCK_Ticks_Per_Second) + NU_Retrieve_Clock()) -
                 device->dev6_next_radv_time) < 
                 (device->dev6_MinRtrAdvInterval * SCK_Ticks_Per_Second)) )
                rtr_adv_timer = device->dev6_MinRtrAdvInterval;
        }
                
        /* Set the timer to transmit the next Router Advertisement */
        if (TQ_Timerset(IP6_Transmit_Rtr_Adv_Event, device->dev_index, 
                        rtr_adv_timer * SCK_Ticks_Per_Second, 
                        ND6RADV_UNSOL_ADV) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set timer to transmit Router Advertisement", 
                            NERR_SEVERE, __FILE__, __LINE__);
        
        /* Save off the time the next Router Advertisement will be transmitted */
        device->dev6_next_radv_time = 
            (rtr_adv_timer * SCK_Ticks_Per_Second + NU_Retrieve_Clock());        
    }

    else
        NLOG_Error_Log("Invalid device index", NERR_INFORMATIONAL, __FILE__, 
                       __LINE__);
        
} /* ND6RADV_Handle_Event */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Output
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function transmits a Router Advertisement on the respective 
*       interface.  Routers send out Router Advertisement messages 
*       periodically, or in response to a Router Solicitation.
*                                                                         
*   INPUTS                                                                
*           
*       *device                 A pointer to the device out which to 
*                               transmit the Router Advertisement.
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
*       None.
*
*************************************************************************/
VOID ND6RADV_Output(DV_DEVICE_ENTRY *device)
{
    NET_BUFFER          *buf_ptr;
    DEV6_IF_ADDRESS     *current_address;
    UINT16              checksum;
    STATUS              status;
    MULTI_SCK_OPTIONS   multi_opts;
    IP6_S_OPTIONS       ip6_options;

    memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

    /* Get a pointer to the link-local address for the interface */
    current_address = IP6_Find_Link_Local_Addr(device);

    if (current_address)
    {
        ip6_options.tx_source_address = current_address->dev6_ip_addr;

        /* Build the ICMP Router Advertisement portion of the packet */
        buf_ptr = ND6RADV_Build(device, device->dev_mac_addr);

        if (buf_ptr != NU_NULL)
        {
            /* Compute the Checksum */
            checksum = UTL6_Checksum(buf_ptr, ip6_options.tx_source_address, 
                                     IP6_All_Nodes_Multi, 
                                     buf_ptr->mem_total_data_len, IPPROTO_ICMPV6, 
                                     IPPROTO_ICMPV6);

            PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);

            UTL_Zero(&multi_opts, sizeof(MULTI_SCK_OPTIONS));

            /* The Destination Address is a multicast address, set the multicast
             * option for the device.
             */
            multi_opts.multio_device = device;

            ip6_options.tx_dest_address = IP6_All_Nodes_Multi;
            ip6_options.tx_hop_limit = ICMP6_VALID_HOP_LIMIT;

            /* Increment the number of ICMP messages sent. */
            MIB_ipv6IfIcmpOutMsgs_Inc(device);

#ifdef NET_5_3

            /* Store off the time of the last Router Advertisement message */
            device->dev6_last_radv_time = NU_Retrieve_Clock();

#endif

            status = IP6_Send(buf_ptr, &ip6_options, IP_ICMPV6_PROT, NU_NULL, 
                              NU_NULL, &multi_opts);

            if (status != NU_SUCCESS)
            {
                /* Increment the number of send errors. */
                MIB_ipv6IfIcmpOutErrors_Inc(device);

                NLOG_Error_Log("Router Advertisement not sent", NERR_SEVERE, 
                               __FILE__, __LINE__);

                /* The packet was not sent.  Deallocate the buffer.  If the packet was
                 * transmitted it will be deallocated when the transmit complete
                 * interrupt occurs. 
                 */
                MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
            }

            else
                MIB_ipv6IfIcmpOutRtAdv_Inc(device);
        }
        else
        {
            /* Increment the number of send errors. */
            MIB_ipv6IfIcmpOutErrors_Inc(device);

            NLOG_Error_Log("Router Advertisement not sent", NERR_SEVERE, 
                           __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Router Advertisement not sent - no link-local address", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

} /* ND6RADV_Output */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Build
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function builds a Router Advertisement Message.                                                               
*                                                                         
*   INPUTS                                                                
*                         
*       *device                 A pointer to the device.                            
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
STATIC NET_BUFFER *ND6RADV_Build(const DV_DEVICE_ENTRY *device,
                                 const UINT8 *source_link_addr)
{
    UINT32              message_size, prefix_entries = 0;
    NET_BUFFER          *buf_ptr;
    UINT8               flags = 0;
    IP6_PREFIX_ENTRY    *current_entry;
    UINT8 HUGE          *saved_data_ptr;

    current_entry = device->dev6_prefix_list->dv_head;

    /* Determine how many Prefix options will be included in the packet */
    while (current_entry)
    {
        if (!(IPV6_IS_ADDR_LINKLOCAL(current_entry->ip6_prfx_lst_prefix)))
            prefix_entries ++;

        current_entry = current_entry->ip6_prefx_lst_next;
    }

    /* Determine the length of the packet */
    message_size = IP6_RTR_ADV_HDR_SIZE + IP6_LINK_LAYER_OPT_SIZE +
        device->dev_addrlen + (prefix_entries * IP6_PREFIX_OPT_SIZE);

    /* If the link MTU option is included, increment the message size */
    if (device->dev6_AdvLinkMTU)
        message_size += IP6_MTU_OPT_SIZE;   

    /* Build the common fields of the ICMP header */
    buf_ptr = ICMP6_Header_Init(ICMP6_RTR_ADV, 0, message_size);

    if (buf_ptr != NU_NULL)
    {
        /* Initialize the checksum to 0 */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);

        /* Fill in the Current Hop Limit */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_CURHOPLMT_OFFSET, 
             (UINT8)device->dev6_AdvCurHopLimit);

        /* Zero out the reserved field */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET, 0);

        /* Determine whether to set the Managed Flag in the packet */
        if (device->dev_flags & DV6_ADV_MGD)
            flags |= (UINT8)(1 << 7);

        /* Determine whether to set the Other Config flag in the packet */
        if (device->dev_flags & DV6_ADV_OTH_CFG)
            flags |= (UINT8)(1 << 6);

        /* Fill in the flags field */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET, 
             flags);

        /* Fill in the Router Lifetime */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RTR_LIFETIME_OFFSET, 
              device->dev6_AdvDefaultLifetime);

        /* Fill in the Reachable Time */
        PUT32(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RCHBL_TIME_OFFSET,
              device->dev6_AdvReachableTime);

        /* Fill in the Retrans Timer */
        PUT32(buf_ptr->data_ptr, IP6_ICMP_RTR_ADV_RTRNS_TMR_OFFSET, 
              device->dev6_AdvRetransTimer);
     
        saved_data_ptr = buf_ptr->data_ptr;

        /* Build the Source Link-Layer option */
        buf_ptr->data_ptr += IP6_RTR_ADV_HDR_SIZE;

        ND6_Build_Link_Layer_Opt(IP6_ICMP_OPTION_SRC_ADDR, source_link_addr, 
                                 buf_ptr, device->dev_addrlen);
        
        /* Increment past the Link-Layer option */
        buf_ptr->data_ptr += IP6_LINK_LAYER_OPT_LENGTH;

        /* Build the MTU option */
        if (device->dev6_AdvLinkMTU)
        {
            ND6RADV_Build_MTU_Option(device, buf_ptr);

            /* Increment past the MTU option */
            buf_ptr->data_ptr += IP6_MTU_OPT_SIZE;
        }

        /* Build the Prefix option(s) */
        ND6RADV_Build_Prefix_Option(device, buf_ptr);

        /* Restore the data pointer to the beginning of the ICMPv6 header */
        buf_ptr->data_ptr = saved_data_ptr;
    }

    else
        NLOG_Error_Log("No buffers available", NERR_SEVERE, __FILE__, 
                       __LINE__);

    return (buf_ptr);

} /* ND6RADV_Build */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Build_MTU_Option
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function builds a Link MTU option for a Router Advertisement 
*       Message.                                                               
*
*        0                   1                   2                   3
*        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |     Type      |    Length     |           Reserved            |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |                              MTU                              |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*                                                                         
*   INPUTS                                                                
*                         
*       *device                 A pointer to the device.                 
*       *buf_ptr                A pointer to the buffer being built.           
*                                                                         
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
STATIC VOID ND6RADV_Build_MTU_Option(const DV_DEVICE_ENTRY *device, 
                                     const NET_BUFFER *buf_ptr)                                    
{
    /* Fill in the option type */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_OPTION_TYPE_OFFSET, IP6_ICMP_OPTION_MTU);

    /* RFC 4861 - section 4.6.4 - length is 1 */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_OPTION_LENGTH_OFFSET, 1);

    /* Zero out the reserved field */
    PUT16(buf_ptr->data_ptr, IP6_ICMP_MTU_RESERVED_OFFSET, 0);

    /* Fill in the configured MTU */
    PUT32(buf_ptr->data_ptr, IP6_ICMP_MTU_OFFSET, device->dev6_AdvLinkMTU);

} /* ND6RADV_Build_MTU_Option */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Build_Prefix_Option
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function builds a Prefix option for a Router Advertisement 
*       Message.        
*
*
*        0                   1                   2                   3
*        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |     Type      |    Length     | Prefix Length |L|A| Reserved1 |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |                         Valid Lifetime                        |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |                       Preferred Lifetime                      |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |                           Reserved2                           |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*       |                                                               |
*       +                                                               +
*       |                                                               |
*       +                            Prefix                             +
*       |                                                               |
*       +                                                               +
*       |                                                               |
*       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*                                                                         
*   INPUTS                                                                
*                         
*       *device                 A pointer to the device.
*       *buf_ptr                A pointer to the buffer being built. 
*                                                                         
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
STATIC VOID ND6RADV_Build_Prefix_Option(const DV_DEVICE_ENTRY *device,
                                        const NET_BUFFER *buf_ptr)
{
    IP6_PREFIX_ENTRY    *current_entry;
    INT                 i = 0;
    UINT8               flags = 0;

    /* Get a pointer to the first prefix entry in the list for the
     * interface.
     */
    current_entry = device->dev6_prefix_list->dv_head;

    /* Add each entry to the prefix option */
    while (current_entry)
    {
        /* Do not add the link-local prefix to the message. */
        if (!(IPV6_IS_ADDR_LINKLOCAL(current_entry->ip6_prfx_lst_prefix)))
        {
            /* Determine if the L and/or A bits should be set */
            if (!(current_entry->ip6_prfx_lst_flags & PRFX6_NO_ADV_ON_LINK))
                flags |= (UINT8)(1 << 7);

            if (!(current_entry->ip6_prfx_lst_flags & PRFX6_NO_ADV_AUTO))
                flags |= (UINT8)(1 << 6);

            /* If the Valid Lifetime decrements in real time, set the new
             * Valid Lifetime value.
             */
            if (current_entry->ip6_prfx_lst_flags & PRFX6_DEC_VAL_LIFE)
            {
                current_entry->ip6_prfx_lst_valid_life = 
                    TQ_Check_Duetime(current_entry->ip6_prfx_lst_valid_life_exp) / 
                    SCK_Ticks_Per_Second;
            }

            /* If the Preferred Lifetime decrements in real time, set the
             * new Preferred Lifetime value.
             */
            if (current_entry->ip6_prfx_lst_flags & PRFX6_DEC_PREF_LIFE)
            {
                current_entry->ip6_prfx_lst_pref_life = 
                    TQ_Check_Duetime(current_entry->ip6_prfx_lst_pref_life_exp) / 
                    SCK_Ticks_Per_Second;
            }

            /* Add the prefix entry */
            ND6RADV_Add_Prefix(buf_ptr, current_entry->ip6_prfx_lst_prefix, 
                               flags, current_entry->ip6_prfx_lst_valid_life, 
                               current_entry->ip6_prfx_lst_pref_life, 
                               current_entry->ip6_prfx_lst_prfx_length, i);

            i += IP6_PREFIX_OPT_SIZE;
        }

        current_entry = current_entry->ip6_prefx_lst_next;        
    }

} /* ND6RADV_Build_Prefix_Option */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RADV_Add_Prefix
*                                                                         
*   DESCRIPTION                                                           
*          
*       This function adds a given prefix to the packet.
*
*   INPUTS                                                                
*                         
*       *buf_ptr                A pointer to the buffer being built. 
*       *prefix                 A pointer to the prefix to add to the 
*                               packet.
*       flags                   The flags to add to the packet.
*       valid_lifetime          The valid lifetime to add to the packet.
*       preferred_lifetime      The preferred lifetime to add to the 
*                               packet.
*       length                  The length of the prefix to add to the
*                               packet.
*       offset                  The offset into the packet of this
*                               prefix entry.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
STATIC VOID ND6RADV_Add_Prefix(const NET_BUFFER *buf_ptr, UINT8 *prefix, 
                               UINT8 flags, UINT32 valid_lifetime, 
                               UINT32 preferred_lifetime, UINT8 length,
                               INT offset)
{
    /* Fill in the type */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_OPTION_TYPE_OFFSET + offset, 
         IP6_ICMP_OPTION_PREFIX);

    /* Fill in the length */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_OPTION_LENGTH_OFFSET + offset, 4);

    /* Fill in the Prefix Length */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_PREFIX_PRE_LENGTH_OFFSET + offset, 
         length);

    /* Zero out the next field */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_PREFIX_FLAG_OFFSET + offset, 0);

    /* Fill in the flags */
    PUT8(buf_ptr->data_ptr, IP6_ICMP_PREFIX_FLAG_OFFSET + offset, 
         flags);

    /* Set the Valid Lifetime */
    PUT32(buf_ptr->data_ptr, 
          IP6_ICMP_PREFIX_VALID_LIFETIME_OFFSET + offset,
          valid_lifetime);

    /* Set the Preferred Lifetime */
    PUT32(buf_ptr->data_ptr, 
          IP6_ICMP_PREFIX_PREF_LIFETIME_OFFSET + offset,
          preferred_lifetime);

    /* Zero out the Reserved field */
    PUT32(buf_ptr->data_ptr, IP6_ICMP_PREFIX_RES2_OFFSET + offset, 0);

    /* Copy the Prefix into the packet */
    NU_BLOCK_COPY(&buf_ptr->data_ptr[IP6_ICMP_PREFIX_PREFIX_OFFSET + offset],
                  prefix, IP6_ADDR_LEN);

} /* ND6RADV_Add_Prefix */

#endif
