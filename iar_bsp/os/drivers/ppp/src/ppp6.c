/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp6.c
*
*   COMPONENT
*
*       IPV6 - IPv6 support for PPP
*
*   DESCRIPTION
*
*       PPP functions for IPv6 support.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PPP6_Init_IPv6_Device
*       PPP6_Attach_IP_Address
*       PPP6_Detach_IP_Address
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if (INCLUDE_IPV6 == NU_TRUE)

extern UINT8    IP6_Link_Local_Prefix[];
extern UINT8    IP6_Hop_Limit;


/*************************************************************************
*
*   FUNCTION
*
*       PPP6_Init_IPv6_Device
*
*   DESCRIPTION
*
*       This function initializes the link-specific parameters of
*       an IPv6 PPP device.
*
*   INPUTS
*
*       *dev_ptr                A pointer to the IPv6 device.
*
*   OUTPUTS
*
*       NU_SUCCESS              The device was successfully initialized.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS PPP6_Init_IPv6_Device(DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS status;

    dev_ptr->dev_ioctl = PPP_Ioctl;
    dev_ptr->dev6_base_reachable_time = IP6_REACHABLE_TIME;
    dev_ptr->dev6_reachable_time = ICMP6_COMPUTE_RTIME(IP6_REACHABLE_TIME);

    dev_ptr->dev6_cur_hop_limit = IP6_Hop_Limit;
    dev_ptr->dev6_retrans_timer = IP6_RETRANS_TIMER;
    dev_ptr->dev6_link_mtu = dev_ptr->dev6_default_mtu = dev_ptr->dev_mtu;
    dev_ptr->dev6_nc_entries = 1;

    dev_ptr->dev6_interface_id_length = IPV6CP_ADDR_LEN;
    memset(dev_ptr->dev6_addr_list.dev6_dst_ip_addr, 0, 16);
    memcpy(dev_ptr->dev6_addr_list.dev6_dst_ip_addr, IP6_Link_Local_Prefix,
            LINK_LOCAL_PREFIX_LENGTH);

    /* Set up the function pointers to manipulate the Neighbor Cache */
    dev_ptr->dev6_add_neighcache_entry = NC6PPP_Add_NeighCache_Entry;
    dev_ptr->dev6_del_neighcache_entry = NC6PPP_Delete_NeighCache_Entry;
    dev_ptr->dev6_fnd_neighcache_entry = NC6PPP_Find_NeighCache_Entry;

    /* Allocate Memory for the Neighbor Cache and entry. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&dev_ptr->dev6_neighbor_cache,
                 (UNSIGNED)(sizeof(IP6_NEIGHBOR_CACHE_ENTRY)), NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Zero out the Neighbor Cache */
        UTL_Zero(dev_ptr->dev6_neighbor_cache,
                                    (UINT32)(sizeof(IP6_NEIGHBOR_CACHE_ENTRY)));

        status = PREFIX6_Init_Prefix_List(&dev_ptr->dev6_prefix_list);

        if (status == NU_SUCCESS)
        {
            /* Add the Link-Local Prefix to the Prefix List with an infinite
               Preferred and Valid Lifetime. */
            status = PREFIX6_Add_Prefix_Entry(dev_ptr, IP6_Link_Local_Prefix,
                                              LINK_LOCAL_PREFIX_LENGTH << 3,
                                              0xffffffffUL);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to add the Link-Local prefix to the Prefix List",
                               NERR_SEVERE, __FILE__, __LINE__);

                /* Deallocate the memory allocated for the Neighbor Cache */
                if (NU_Deallocate_Memory(dev_ptr->dev6_neighbor_cache) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to Deallocate memory for the Neighbor Cache",
                                   NERR_SEVERE, __FILE__, __LINE__);

                dev_ptr->dev6_neighbor_cache = NU_NULL;
            }
        }
        else
        {
            NLOG_Error_Log("Failed to Create the Prefix List",
                           NERR_SEVERE, __FILE__, __LINE__);

            /* Deallocate the memory allocated for the Neighbor Cache */
            if (NU_Deallocate_Memory(dev_ptr->dev6_neighbor_cache) != NU_SUCCESS)
                NLOG_Error_Log("Failed to Deallocate memory for the Neighbor Cache",
                               NERR_SEVERE, __FILE__, __LINE__);

            dev_ptr->dev6_neighbor_cache = NU_NULL;
        }
    }
    else
        NLOG_Error_Log("Failed to Create the Neighbor Cache",
                       NERR_SEVERE, __FILE__, __LINE__);

    return status;

} /* PPP6_Init_IPv6_Device */



/*************************************************************************
*
*   FUNCTION
*
*       PPP6_Attach_IP_Address
*
*   DESCRIPTION
*
*       After IPV6CP has negotiated successfully, this function is
*       called to assign the IPv6 addresses to the PPP device. It will
*       also add a route to the peer.
*
*   INPUTS
*
*       *dev_ptr                A pointer to the IPv6 PPP device.
*
*   OUTPUTS
*
*       NU_SUCCESS on success, otherwise an error code is returned.
*
*************************************************************************/
STATUS PPP6_Attach_IP_Address(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER *link = (LINK_LAYER*)dev_ptr->dev_link_layer;
    UINT8 *local;
    UINT8 *remote;
    STATUS status;

    local = link->ncp6.options.ipv6cp.local_address;
    remote = link->ncp6.options.ipv6cp.remote_address;

    /* Store the obtained addresses in the device structure for this link. */
    memcpy(dev_ptr->dev6_interface_id, local, IPV6CP_ADDR_LEN);
    memcpy(&dev_ptr->dev6_addr_list.dev6_dst_ip_addr[IPV6CP_ADDR_LEN], remote, IPV6CP_ADDR_LEN);

    /* Add the IP address and route to the peer. */
    status = DEV6_AutoConfigure_Device(dev_ptr);
    if (status == NU_SUCCESS)
    {
        /* Add the remote peer to the neighbor cache. This will be the only entry
           in the neighbor cache for this device. */
        status = NC6PPP_Add_RemotePPP_Entry(dev_ptr, dev_ptr->dev6_addr_list.dev6_dst_ip_addr);
        if (status == NU_SUCCESS)
        {
            /* Add a route to the remote peer, using the local PPP interface as the
               next hop. */
            status = RTAB6_Add_Route(dev_ptr,
                            dev_ptr->dev6_addr_list.dev6_dst_ip_addr,
                            dev_ptr->dev6_addr_list.dv_head->dev6_ip_addr,
                            128, (RT_UP | RT_HOST | RT_STATIC));
        }
    }

    return status;
}


/*************************************************************************
*
*   FUNCTION
*
*       PPP6_Detach_IP_Address
*
*   DESCRIPTION
*
*       Removes the global and link-local IPv6 addresses from the PPP
*       device.
*
*   INPUTS
*
*       *dev_ptr                A pointer to the IPv6 PPP device.
*
*   OUTPUTS
*
*       NU_SUCCESS on success, otherwise an error code is returned.
*
*************************************************************************/
STATUS PPP6_Detach_IP_Address(DV_DEVICE_ENTRY *dev_ptr)
{
    /* Remove the destination IP from the device as well. */
    memset(&dev_ptr->dev6_addr_list.dev6_dst_ip_addr[IPV6CP_ADDR_LEN], 0, IPV6CP_ADDR_LEN);

    if(dev_ptr->dev6_addr_list.dv_head)
        /* Remove the local IP address and route. */
        DEV6_Delete_IP_From_Device(dev_ptr->dev6_addr_list.dv_head);

    /* Delete the peer's neighbor cache entry. */
    NC6PPP_Delete_NeighCache_Entry(dev_ptr, dev_ptr->dev6_addr_list.dev6_dst_ip_addr);

    return NU_SUCCESS;
}


#endif                                      /* INCLUDE_IPV6 */
