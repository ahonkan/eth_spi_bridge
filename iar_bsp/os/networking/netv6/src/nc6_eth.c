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
*       nc6_eth.c                                    
*                                                                               
*   COMPONENT                                                             
*                                                                       
*       IPv6 Neighbor Cache for Ethernet
*                                                                       
*   DESCRIPTION                                                           
*                                                                    
*       This file contains those functions necessary to maintain the
*       Ethernet Neighbor Cache for an IPv6 node.   
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       NC6ETH_Add_NeighCache_Entry
*       NC6ETH_Delete_NeighCache_Entry
*       NC6ETH_CleanUp_Entry
*       NC6ETH_Find_NeighCache_Entry
*       NC6ETH_Retrieve_NeighCache_Entry
*       NC6ETH_Link_Addrs_Equal
*       NC6ETH_Update_NeighCache_Link_Addr
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       net_extr.h
*       nc6_eth.h
*       nc6.h
*       defrtr6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/net_extr.h"
#include "networking/nc6_eth.h"
#include "networking/nc6.h"
#include "networking/defrtr6.h"

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_Add_NeighCache_Entry
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function adds an entry to the Neighbor Cache Table.
*                                                                         
*   INPUTS                                                                
*                         
*       *device                 The device on which to add the entry.
*       *source_addr            The IP address of the new entry.
*       *link_addr              The link-layer address of the new entry.
*       flags                   Flags associated with the new entry.
*       *queued_packets         A pointer to any packets awaiting address
*                               resolution.
*       reach_state             The NUD reachability state of the 
*                               Neighbor.                                                
*                                                                         
*   OUTPUTS                                                               
*                                                                  
*       A pointer to the new entry or NU_NULL if the entry could not
*       be created.       
*
*************************************************************************/
IP6_NEIGHBOR_CACHE_ENTRY *NC6ETH_Add_NeighCache_Entry(DV_DEVICE_ENTRY *device, 
                                                      UINT8 *source_addr, 
                                                      const UINT8 *link_addr, 
                                                      UINT32 flags, 
                                                      NET_BUFFER *queued_packets, 
                                                      UINT8 reach_state)
{
    UINT32                          timer;
    INT16                           i, found = -1;
    IP6_NEIGHBOR_CACHE_ENTRY        *nc_entry;

    /* Search through the Neighbor Cache to see if we already have a
     * matching entry.  If an entry is not found, timeout the oldest
     * entry, and put the new entry in its place.
     */
    nc_entry = device->dev6_fnd_neighcache_entry(device, source_addr);

    if (nc_entry == NU_NULL)
    {
        timer = NU_Retrieve_Clock();

        for (i = 0; i < device->dev6_nc_entries; i++)
        {
            /* If the timestamp of the entry is less than the current oldest
             * timestamp, the entry is not undergoing Address Resolution, and the
             * entry is not permanent, update the current timestamp and set the 
             * found variable to the index of the entry.
             */
            if ( (device->dev6_neighbor_cache[i].ip6_neigh_cache_timestamp == 0) ||
                 ((INT32_CMP(device->dev6_neighbor_cache[i].ip6_neigh_cache_timestamp, 
                            timer) < 0) && 
                 (device->dev6_neighbor_cache[i].ip6_neigh_cache_resolve_id == 0) &&
                 ((device->dev6_neighbor_cache[i].ip6_neigh_cache_flags & NC_PERMANENT) == 0)) )
            {
                found = i;
                timer = device->dev6_neighbor_cache[i].ip6_neigh_cache_timestamp;

                /* If the timer is zero, this entry is empty. Use it. */
                if (timer == 0)
                    break;
            }
        }

        if (found >= 0)
        {
            nc_entry = &device->dev6_neighbor_cache[found];

            /* If the entry is being reused, clean up the members */
            if (timer != 0)
                NC6ETH_CleanUp_Entry(nc_entry);

            /* Copy the IP address into the entry */
            NU_BLOCK_COPY(nc_entry->ip6_neigh_cache_ip_addr, source_addr, IP6_ADDR_LEN);

            /* If a link-layer address was provided, copy it into the entry */
            if (link_addr)
                memcpy(((IP6_ETH_NEIGHBOR_CACHE_ENTRY*)
                        (nc_entry->ip6_neigh_cache_link_spec))->
                        ip6_neigh_cache_hw_addr, link_addr, device->dev_addrlen);
            
            /* Timestamp the entry */
            nc_entry->ip6_neigh_cache_timestamp = NU_Retrieve_Clock();

            /* Set the reachability state of the entry */
            nc_entry->ip6_neigh_cache_state = reach_state;

            /* If the Neighbor is a router, set the default router parameter to
             * the corresponding default router entry if one exists.
             */
            if (flags & NC_ISROUTER)
                NC6_Transition_To_Router(nc_entry);

            /* Set the flag to indicate that the entry is up and set any other
             * flags that were passed in.
             */
            nc_entry->ip6_neigh_cache_flags |= (NC_UP | flags);
                    

            /* Place any packets queued up to be sent on the queued packets list */
            if (queued_packets)
            {
                if (MEM_Buffer_Enqueue(&nc_entry->ip6_neigh_cache_packet_list,
                                       queued_packets) == NU_NULL)
                    NLOG_Error_Log("Failed to enqueue the packet awaiting Address Resolution to complete", 
                                   NERR_SEVERE, __FILE__, __LINE__);

                nc_entry->ip6_neigh_cache_qpkts_count++;
            }

            nc_entry->ip6_neigh_cache_device = device;
        }
        else
            NLOG_Error_Log("IPv6 Neighbor Cache full", NERR_SEVERE, 
                           __FILE__, __LINE__);
    }

    return (nc_entry);

} /* NC6ETH_Add_NeighCache_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_Delete_NeighCache_Entry
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function will deletes an entry from the Neighbor Cache Table.
*                                                                         
*   INPUTS                                                                
*                                               
*       *device                 A pointer to the device.
*       *source_addr            A pointer to the IP address of the entry 
*                               to delete.                      
*                                                                         
*   OUTPUTS                                                               
*                                                                  
*       NU_SUCCESS              The Neighbor Cache entry was deleted.
*       -1                      The Neighbor Cache entry does not exist.
*
*************************************************************************/
STATUS NC6ETH_Delete_NeighCache_Entry(DV_DEVICE_ENTRY *device, UINT8 *source_addr)
{
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry;

    /* If the entry exists in the list, delete it.  If the entry does
     * not exist, return -1.
     */
    nc_entry = device->dev6_fnd_neighcache_entry(device, source_addr);

    if (nc_entry != NU_NULL)
    {
        /* Clean up the members of the entry. */
        NC6ETH_CleanUp_Entry(nc_entry);
    }
    else
        return (-1);

    return (NU_SUCCESS);

} /* NC6ETH_Delete_NeighCache_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_CleanUp_Entry
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function cleans up the members of a Neighbor Cache entry.
*                                                                         
*   INPUTS                                                                
*                                               
*       *entry                  A pointer to the Neighbor Cache entry.
*                                                                         
*   OUTPUTS                                                               
*                                                                  
*       None
*
*************************************************************************/
VOID NC6ETH_CleanUp_Entry(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    IP6_DEST_LIST_ENTRY *dl_entry;
    NET_BUFFER          *current_packet;

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    IP6LAYER            *pkt;
    RTAB6_ROUTE         ro;
#endif

    /* Get the first Destination entry in the list */
    dl_entry = nc_entry->ip6_neigh_cache_dest_list.dv_head;       

    /* Delete each pointer to the route entry, and delete the 
     * corresponding pointer in the routing table to this
     * Neighbor Cache entry.
     */
    while (dl_entry)
    {
        /* Remove the route from the Neighbor Cache entry */
        DLL_Remove(&nc_entry->ip6_neigh_cache_dest_list, dl_entry);

        if (RTAB6_Delete_Route_From_Node(dl_entry->ip6_dest_list_entry->rt_route_node,
                                         nc_entry->ip6_neigh_cache_ip_addr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to delete route from Neighbor Cache entry", 
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)dl_entry) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the Destination List entry", 
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Get the head Destination entry in the list */
        dl_entry = nc_entry->ip6_neigh_cache_dest_list.dv_head;
    }

    /* If the Neighbor Cache entry is pointing to a Default Router entry,
     * set the pointer to NULL.
     */
    if (nc_entry->ip6_neigh_cache_def_rtr != NU_NULL)
        nc_entry->ip6_neigh_cache_def_rtr->ip6_def_rtr_nc_entry = NU_NULL;

    /* If there are any buffers being used by this entry, release the buffers. */
    if (nc_entry->ip6_neigh_cache_packet_list.head)
    {
        /* Get the first buffer from the list */
        current_packet = MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);

        /* While there are packets in the list, place them on their respective
         * dlists.
         */
        while (current_packet)
        {
#if (INCLUDE_IP_FORWARDING == NU_TRUE)

            pkt = (IP6LAYER*)current_packet->data_ptr;

            /* If the packet is being forwarded, transmit an ICMP Destination
             * Unreachable message to the original sender.
             */
            if (!DEV6_Get_Dev_By_Addr(pkt->ip6_src))
            {
                ro.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;

                ro.rt_ip_dest.rtab6_rt_ip_dest.sck_len = 
                    sizeof(SCK6_SOCKADDR_IP);

                NU_BLOCK_COPY(ro.rt_ip_dest.rtab6_rt_ip_dest.sck_addr, 
                              pkt->ip6_src, IP6_ADDR_LEN);

                /* Find a route to the destination of the ICMPv6 error */
                IP6_Find_Route(&ro);

                if (ro.rt_route)
                {
                    /* Restore the Hop Limit */
                    pkt->ip6_hop ++;

                    /* Send the ICMPv6 Destination Unreachable message to
                     * the source of the invoking packet.
                     */
                    ICMP6_Send_Error(pkt, current_packet, ICMP6_DST_UNREACH, 
                                     ICMP6_DST_UNREACH_ADDRESS, 0, 
                                     ro.rt_route->rt_entry_parms.rt_parm_device);

                    /* Free the route. */
                    RTAB_Free((ROUTE_ENTRY*)ro.rt_route, NU_FAMILY_IP6);
                }
            }
#endif

            /* Put the buffer on its dlist */
            MEM_Multiple_Buffer_Chain_Free(current_packet);

            /* Get the next buffer in the list */
            current_packet = MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);
        }
    }

    /* Zero out the link-layer address in the NC entry. */
    memset(((IP6_ETH_NEIGHBOR_CACHE_ENTRY*)
           (nc_entry->ip6_neigh_cache_link_spec))->ip6_neigh_cache_hw_addr, 
           0, nc_entry->ip6_neigh_cache_device->dev_addrlen);

    /* Zero out the entry. */
    UTL_Zero(nc_entry, sizeof(IP6_NEIGHBOR_CACHE_ENTRY) - sizeof(VOID*));

} /* NC6ETH_CleanUp_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_Find_NeighCache_Entry
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function returns the Neighbor Cache entry associated with
*       the source address provided.  If no device is provided, the
*       function will traverse the Neighbor Cache of each device in
*       search for the address.
*                                                                         
*   INPUTS                                                                
*                                              
*       *device                 A pointer to the device or NU_NULL.
*       *source_addr            A pointer to the IP address of the target 
*                               entry.                       
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       A pointer to the Neighbor Cache entry or NU_NULL if the entry
*       does not exist.
*
*************************************************************************/
IP6_NEIGHBOR_CACHE_ENTRY *NC6ETH_Find_NeighCache_Entry(const DV_DEVICE_ENTRY *device,
                                                       const UINT8 *source_addr)
{
    DV_DEVICE_ENTRY             *target_device;
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry = NU_NULL;

    /* If a device was passed in, find the Neighbor Cache entry associated
     * with the source_addr for that device.
     */
    if (device != NU_NULL)
        nc_entry = NC6ETH_Retrieve_NeighCache_Entry(device, source_addr);

    /* If a device was not passed in, search each device's Neighbor Cache
     * for an entry matching the source_addr that was passed in.
     */
    else
    {
        target_device = DEV_Table.dv_head;

        /* Traverse each device's Neighbor Cache for the target address */
        while ( (nc_entry == NU_NULL) && (target_device != NU_NULL) )
        {
            if (target_device->dev_flags & DV6_IPV6)
                nc_entry = NC6ETH_Retrieve_NeighCache_Entry(target_device, source_addr);

            target_device = target_device->dev_next;
        }
    }

    /* Return the entry or NU_NULL if no entry was found */
    return (nc_entry);

} /* NC6ETH_Find_NeighCache_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_Retrieve_NeighCache_Entry
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function returns the Neighbor Cache entry associated with
*       the device and source address provided.                                                           
*                                                                         
*   INPUTS                                                                
*                                              
*       *device                 A pointer to the device.
*       *source_addr            A pointer to the IP address of the target 
*                               entry.                       
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       A pointer to the Neighbor Cache entry or NU_NULL if the entry
*       does not exist.
*
*************************************************************************/
IP6_NEIGHBOR_CACHE_ENTRY *NC6ETH_Retrieve_NeighCache_Entry(const DV_DEVICE_ENTRY *device,
                                                           const UINT8 *source_addr)
{
    INT16                       i;
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry = NU_NULL;
    UNSIGNED                    current_time;

    current_time = NU_Retrieve_Clock();

    /* Search the Neighbor Cache for a matching entry. */
    for (i = 0; i < device->dev6_nc_entries; i++)
    {
        if (memcmp(source_addr, 
                   device->dev6_neighbor_cache[i].ip6_neigh_cache_ip_addr, 
                   IP6_ADDR_LEN) == 0)
        {
            /* If the entry has not timed out or is permanent and the entry
             * is up.
             */
            if (((INT32_CMP((device->dev6_neighbor_cache[i].ip6_neigh_cache_timestamp + NC_TIMEOUT_ENTRY), 
                             current_time) > 0)
                || (device->dev6_neighbor_cache[i].ip6_neigh_cache_flags & NC_PERMANENT))
                && (device->dev6_neighbor_cache[i].ip6_neigh_cache_flags & NC_UP) )
            {
                nc_entry = &device->dev6_neighbor_cache[i];

                /* Update the timestamp of the entry */
                nc_entry->ip6_neigh_cache_timestamp = current_time;
                break;
            }
        }
    }

    return (nc_entry);

} /* NC6ETH_Retrieve_NeighCache_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_Link_Addrs_Equal
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function determines whether the link_addr provided is less
*       than, equal to or greater than the link-layer address in the
*       Neighbor Cache entry.                                    
*                                                                         
*   INPUTS                                                                
*                                              
*       *link_addr              A pointer to the first link-layer address.
*       *nc_entry               A pointer to the data structure containing 
*                               the second link-layer address.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       0                       The two addresses are equal.
*       -1                      The first address is less than the second.
*       1                       The first address is greater than the 
*                               second.
*
*************************************************************************/
INT NC6ETH_Link_Addrs_Equal(const UINT8 *link_addr, 
                            const IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    return (memcmp(link_addr, ((IP6_ETH_NEIGHBOR_CACHE_ENTRY*)
            (nc_entry->ip6_neigh_cache_link_spec))->ip6_neigh_cache_hw_addr, 
            nc_entry->ip6_neigh_cache_device->dev_addrlen));

} /* NC6ETH_Link_Addrs_Equal */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6ETH_Update_NeighCache_Link_Addr
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function updates the link-layer address of the Neighbor
*       Cache entry with the address specified.
*                                                                         
*   INPUTS                                                                
*                                              
*       *nc_entry               A pointer to the Neighbor Cache entry.
*       *link_addr              A pointer to the new link-layer address.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       None
*
*************************************************************************/
VOID NC6ETH_Update_NeighCache_Link_Addr(const IP6_NEIGHBOR_CACHE_ENTRY *nc_entry,
                                        const UINT8 *link_addr)
{
    memcpy(((IP6_ETH_NEIGHBOR_CACHE_ENTRY*)
           (nc_entry->ip6_neigh_cache_link_spec))->ip6_neigh_cache_hw_addr, link_addr, 
           nc_entry->ip6_neigh_cache_device->dev_addrlen);

} /* NC6ETH_Update_NeighCache_Link_Addr */
