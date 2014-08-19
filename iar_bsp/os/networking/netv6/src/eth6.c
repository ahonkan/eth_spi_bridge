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
*       eth6.c                                       
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file will hold all the routines which are used by IPv6 to 
*       interface with ethernet hardware.  They will handle the basic 
*       functions of setup, xmit, receive, etc.  This file will change 
*       depending on the type of chip(s) you are using to handle the 
*       TCP/IP interface.  These files are generic for IPv6 and will need 
*       to be changed for your specified interface.  This file will use 
*       structure overlays for the chip(s) with the offset defines from 
*       the file chipint.h.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ETH6_Map_Multi
*       ETH6_Init_IPv6_Device
*
*   DEPENDENCIES                                                             
*               
*       externs.h
*       net6.h
*       nc6.h
*       nc6_eth
*       prefix6.h
*
*************************************************************************/

#include "networking/externs.h"
#include "networking/net6.h"
#include "networking/nc6.h"
#include "networking/nc6_eth.h"
#include "networking/prefix6.h"

extern UINT8    IP6_Link_Local_Prefix[];
extern UINT8    IP6_Hop_Limit;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ETH6_Map_Multi
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function converts an IPv6 IP address into a link-layer 
*       multicast address.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *d_req                  A pointer to the device record.
*       *multi_addr             A pointer to the memory in which to store
*                               the multicast address.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       -1                      The address is not valid.
*       0                       Successful              
*
*************************************************************************/
STATUS ETH6_Map_Multi(const DV_REQ *d_req, UINT8 *multi_addr)
{
    UINT8   tempv6[IP6_ADDR_LEN];

    memcpy(tempv6, d_req->dvr_dvru.dvru_addrv6, IP6_ADDR_LEN);

    /* Convert the IP address to a multicast ethernet address. */
    NET6_MAP_IP_TO_ETHER_MULTI(tempv6, multi_addr);

    /* Verify that the ethernet multicast address is valid. */
    if ( ((multi_addr[0] & 0x33) != 51) || ((multi_addr[1] & 0x33) != 51) )
        return (-1);

    return (0);

} /* ETH6_Map_Multi */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ETH6_Init_IPv6_Device
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function initializes the link-specific parameters of
*       an IPv6 ethernet device.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *device                 A pointer to the IPv6 device.
*       neigh_cache_entries     The number of Neighbor Cache entries 
*                               to retain in the Neighbor Cache of the 
*                               device.
*       dup_detect_trans        The number of Neighbor Solicitation 
*                               messages to transmit during DAD.  
*                               Zero if disabled.
*       interface_id_length     The length of the interface ID 
*                               associated with the device.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              The device was successfully initialized.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS ETH6_Init_IPv6_Device(DV_DEVICE_ENTRY *device, INT16 neigh_cache_entries,
                             UINT8 dup_detect_trans, UINT8 interface_id_length)
{
    INT     i;
    IP6_ETH_NEIGHBOR_CACHE_ENTRY    *nc_entry;
    STATUS                          status;
#if (INCLUDE_VLAN == NU_TRUE)
    SCK_IOCTL_OPTION    option;
    UINT32              sys_clock;  /* Added for VLAN device initialization */
#endif

#if (INCLUDE_DAD6 == NU_TRUE)

    /* If Duplicate Address Detection has not been disabled on the device.
     * RFC 2462 section 5.1 - the number of Consecutive Neighbor 
     * Solicitation messages sent while performing Duplicate Address 
     * Detection on a tentative address.
     */
    if (!(device->dev_flags & DV6_NODAD))
        device->dev6_dup_addr_detect_trans = dup_detect_trans;
    else
        device->dev6_dup_addr_detect_trans = 0;

#else
    UNUSED_PARAMETER(dup_detect_trans);
#endif

    /* Set the Reachable Time and the Base Reachable Time.  Reachable Time is
     * the time a neighbor is considered reachable after receiving a reachability
     * confirmation.  Base Reachable Time is a base value used for computing the
     * Reachable Time value.
     */
    device->dev6_base_reachable_time = IP6_REACHABLE_TIME;
    device->dev6_reachable_time = 
        ICMP6_COMPUTE_RTIME(device->dev6_base_reachable_time);

    /* Set the Hop Limit initially to the default */
    if (device->dev_type != DVT_LOOP)
        device->dev6_cur_hop_limit = IP6_Hop_Limit;
    else
        device->dev6_cur_hop_limit = 1;

    device->dev6_retrans_timer = IP6_RETRANS_TIMER;

    device->dev6_link_mtu = device->dev6_default_mtu = device->dev_mtu;

    device->dev6_nc_entries = neigh_cache_entries;

    /* Set up the function pointers to manipulate the Neighbor Cache */
    device->dev6_add_neighcache_entry = NC6ETH_Add_NeighCache_Entry;
    device->dev6_del_neighcache_entry = NC6ETH_Delete_NeighCache_Entry;
    device->dev6_fnd_neighcache_entry = NC6ETH_Find_NeighCache_Entry;
    device->dev6_neighcache_entry_equal = NC6ETH_Link_Addrs_Equal;
    device->dev6_update_neighcache_link_addr = NC6ETH_Update_NeighCache_Link_Addr;

/* VLANFLAG */

#if (INCLUDE_VLAN == NU_TRUE)

    /*******************************************************************************/
    /* When initializing VLAN devices do not use the MAC Address because this will */
    /* generate the exact same Link-Local address as the real ethernet device the  */
    /* VLAN devices are dependent upon.   Use the device index, the VLAN group ID, */
    /* and the system clock to generate a unique link-local address.               */
    /*******************************************************************************/

    if (device->dev_type == DVT_VLAN)
    {
        sys_clock = NU_Retrieve_Clock();

        option.s_optval = (UINT8*)device->dev_net_if_name;

        /* Get the VLAN ID */
        if (Ioctl_SIOCGETVLAN(&option) != NU_SUCCESS)
            NLOG_Error_Log("Failed to get the VLAN ID for the VLAN interface", 
                           NERR_SEVERE, __FILE__, __LINE__);

        device->dev6_interface_id[0] = (UINT8)(device->dev_index & 0x000000FF);

        device->dev6_interface_id[1] = (UINT8)((option.s_ret.vlan_id & 0xFF00) >> 8);
        device->dev6_interface_id[2] = (UINT8)(option.s_ret.vlan_id & 0x00FF);

        device->dev6_interface_id[3] = 0xDC;

        device->dev6_interface_id[4] = (UINT8)(sys_clock & 0x000000FF);
        device->dev6_interface_id[5] = (UINT8)((sys_clock & 0x0000FF00) >> 8);
        device->dev6_interface_id[6] = (UINT8)((sys_clock & 0x000000FF) | 0x5A);
        device->dev6_interface_id[7] = (UINT8)(((sys_clock & 0x0000FF00) >> 8) | 0xA5);
    }

#endif

    if (device->dev_type == DVT_ETHER)
    {
        /* Generate the Interface Identifier */
        device->dev6_interface_id[0] = device->dev_mac_addr[0];
        device->dev6_interface_id[1] = device->dev_mac_addr[1];
        device->dev6_interface_id[2] = device->dev_mac_addr[2];
        device->dev6_interface_id[3] = 0xFF;
        device->dev6_interface_id[4] = 0xFE;
        device->dev6_interface_id[5] = device->dev_mac_addr[3];
        device->dev6_interface_id[6] = device->dev_mac_addr[4];
        device->dev6_interface_id[7] = device->dev_mac_addr[5];

        /* Invert the value of the u/l bit in the first byte of the MAC address 
         * of the interface identifier.
         */
        if ((device->dev6_interface_id[0] & (0x10 >> 3)) != 0)
            device->dev6_interface_id[0] &= ~(0x10 >> 3);
        else
            device->dev6_interface_id[0] |= (0x10 >> 3);
    }

    /* Set the length of the Interface Identifier */
    device->dev6_interface_id_length = interface_id_length;

    /* Allocate Memory for the Neighbor Cache */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&device->dev6_neighbor_cache,
                                (UNSIGNED)(neigh_cache_entries * (sizeof(IP6_NEIGHBOR_CACHE_ENTRY))), 
                                NU_NO_SUSPEND);
    
    if (status == NU_SUCCESS)
    {
        /* Zero out the Neighbor Cache */
        UTL_Zero(device->dev6_neighbor_cache, (UINT32)(neigh_cache_entries * 
                 sizeof(IP6_NEIGHBOR_CACHE_ENTRY)));

        for (i = 0; i < neigh_cache_entries; i++)
        {
            nc_entry = device->dev6_neighbor_cache[i].ip6_neigh_cache_link_spec;

            /* Allocate Memory for the Neighbor Cache */
            status = NU_Allocate_Memory(MEM_Cached, (VOID**)&nc_entry, 
                                        sizeof(IP6_ETH_NEIGHBOR_CACHE_ENTRY), 
                                        NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {    
                UTL_Zero(nc_entry, sizeof(IP6_ETH_NEIGHBOR_CACHE_ENTRY));    
                device->dev6_neighbor_cache[i].ip6_neigh_cache_link_spec = nc_entry;
            }
            else
            {
                for (i-- ; i >= 0; i--)
                {
                	/* Deallocate all the previously allocated memory for the
                	 * link-specific portion of the Neighbor cache entries
                	 */
                    if (NU_Deallocate_Memory(device->dev6_neighbor_cache[i].
                    		                 ip6_neigh_cache_link_spec) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to deallocate memory for the Neighbor Cache entries",
                                       NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Deallocate the memory previously allocated */
                if (NU_Deallocate_Memory(device->dev6_neighbor_cache) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory for the Neighbor Cache", 
                                   NERR_SEVERE, __FILE__, __LINE__);

                NLOG_Error_Log("Failed to Create the Neighbor Cache entries", 
                               NERR_SEVERE, __FILE__, __LINE__);

                break;
            }
        }
    
        if ( (status == NU_SUCCESS) && (device->dev_type != DVT_LOOP) )
        {
            status = PREFIX6_Init_Prefix_List(&device->dev6_prefix_list);

            if (status == NU_SUCCESS)
            {    
                /* Add the Link-Local Prefix to the Prefix List with an infinite
                 * Preferred and Valid Lifetime.
                 */
                status = PREFIX6_New_Prefix_Entry(device, IP6_Link_Local_Prefix, 
                                                  LINK_LOCAL_PREFIX_LENGTH << 3, 
                                                  0xffffffffUL, 0xffffffffUL, 0);

                if (status != NU_SUCCESS)
                    NLOG_Error_Log("Failed to add the Link-Local prefix to the Prefix List", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
            else
                NLOG_Error_Log("Failed to Create the Prefix List", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
        NLOG_Error_Log("Failed to Create the Neighbor Cache", 
                       NERR_SEVERE, __FILE__, __LINE__);

    return (status);

} /* ETH6_Init_IPv6_Device */
