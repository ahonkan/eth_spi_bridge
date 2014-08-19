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
*       nc6_ppp.c
*
*   COMPONENT
*
*       IPV6 - IPv6 support for PPP
*
*   DESCRIPTION
*
*       This file contains those functions necessary to maintain the
*       PPP Neighbor Cache for an IPv6 node.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NC6PPP_Add_NeighCache_Entry
*       NC6PPP_Add_RemotePPP_Entry
*       NC6PPP_Delete_NeighCache_Entry
*       NC6PPP_Find_NeighCache_Entry
*       NC6PPP_CleanUp_Entry
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if (INCLUDE_IPV6 == NU_TRUE)
VOID NC6PPP_CleanUp_Entry(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry);


/*************************************************************************
*
*   FUNCTION
*
*       NC6PPP_Add_NeighCache_Entry
*
*   DESCRIPTION
*
*       This function is a stub for the dev_ptr->dev6_add_neighcache_entry
*       since IP6 does not need to add a neighbor entry for PPP links.
*       The neighbor cache entry for this device should already be filled
*       in, or else it is invalid (no neighbors, device is off-link).
*
*   INPUTS
*
*       *device                 The device on which to add the entry.
*       *unused1                Unused in NC6PPP
*       *unused2                Unused in NC6PPP
*       unused3                 Unused in NC6PPP
*       *unused4                Unused in NC6PPP
*       unused5                 Unused in NC6PPP
*
*   OUTPUTS
*
*       A pointer to the new entry.
*
*************************************************************************/
IP6_NEIGHBOR_CACHE_ENTRY *NC6PPP_Add_NeighCache_Entry(DV_DEVICE_ENTRY *dev_ptr,
    UINT8 *unused1, const UINT8 *unused2, UINT32 unused3,
    NET_BUFFER *unused4, UINT8 unused5)
{
    UNUSED_PARAMETER(unused1);
    UNUSED_PARAMETER(unused2);
    UNUSED_PARAMETER(unused3);
    UNUSED_PARAMETER(unused4);
    UNUSED_PARAMETER(unused5);

    return dev_ptr->dev6_neighbor_cache;

} /* NC6PPP_Add_NeighCache_Entry */



/*************************************************************************
*
*   FUNCTION
*
*       NC6PPP_Add_RemotePPP_Entry
*
*   DESCRIPTION
*
*       This function adds the Neighbor Cache entry for the PPP peer. This
*       is called after the IPv6 addresses have been resolved within PPP
*       negotiation.
*
*   INPUTS
*
*       *device                 The device on which to add the entry.
*       *remote_ip              The IP address of the entry.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS NC6PPP_Add_RemotePPP_Entry(DV_DEVICE_ENTRY *device, UINT8 *remote_ip)
{
    IP6_NEIGHBOR_CACHE_ENTRY *nc_entry;

    /* Point to the only entry in the neighbor cache. */
    nc_entry = device->dev6_neighbor_cache;

    /* Copy the IP address into the entry */
    NU_BLOCK_COPY(nc_entry->ip6_neigh_cache_ip_addr, remote_ip, IP6_ADDR_LEN);

    /* Timestamp the entry */
    nc_entry->ip6_neigh_cache_timestamp = NU_Retrieve_Clock();

    /* Set the reachability state of the entry */
    nc_entry->ip6_neigh_cache_state = NC_NEIGH_INCOMPLETE;

    /* Set the default router parameter to the corresponding default router
       entry if one exists. */
    NC6_Transition_To_Router(nc_entry);

    /* Set the flag to indicate that the entry is up. */
    nc_entry->ip6_neigh_cache_flags = NC_UP;

    /* Assign this device for the entry. */
    nc_entry->ip6_neigh_cache_device = device;

    device->dev6_neighbor_cache = nc_entry;
    return NU_SUCCESS;

} /* NC6PPP_Add_RemotePPP_Entry */



/*************************************************************************
*
*   FUNCTION
*
*       NC6PPP_Delete_NeighCache_Entry
*
*   DESCRIPTION
*
*       This function will delete the Neighbor Cache entry.
*
*   INPUTS
*
*       *device                 A pointer to the device.
*       *ip_addr                A pointer to the IP address of the entry
*                               to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The Neighbor Cache entry was deleted.
*       -1                      The Neighbor Cache entry does not exist.
*
*************************************************************************/
STATUS NC6PPP_Delete_NeighCache_Entry(DV_DEVICE_ENTRY *device, UINT8 *ip_addr)
{
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry;

    /* If the entry exists, delete it. If the entry does not exist,
       return error. */
    nc_entry = device->dev6_fnd_neighcache_entry(device, ip_addr);

    if (nc_entry != NU_NULL)
    {
        /* Clean up the members of the entry. */
        NC6PPP_CleanUp_Entry(nc_entry);
    }
    else
        return (-1);

    return NU_SUCCESS;

} /* NC6PPP_Delete_NeighCache_Entry */



/*************************************************************************
*
*   FUNCTION
*
*       NC6PPP_Find_NeighCache_Entry
*
*   DESCRIPTION
*
*       This function returns the Neighbor Cache entry associated with
*       the source address provided.
*
*   INPUTS
*
*       *device                 A pointer to the device or NU_NULL.
*       *ip_addr                A pointer to the IP address of the target
*                               entry.
*
*   OUTPUTS
*
*       A pointer to the Neighbor Cache entry or NU_NULL if the entry
*       does not exist.
*
*************************************************************************/
IP6_NEIGHBOR_CACHE_ENTRY*
NC6PPP_Find_NeighCache_Entry(const DV_DEVICE_ENTRY *device,
                             const UINT8 *ip_addr)
{
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry = device->dev6_neighbor_cache;

    /* Make sure the Neighbor Cache entry is associated with the
       source_addr for this device. */
    if (device != NU_NULL && memcmp(ip_addr,
        device->dev6_neighbor_cache->ip6_neigh_cache_ip_addr,
        IP6_ADDR_LEN) == 0)
    {
        nc_entry = device->dev6_neighbor_cache;
    }

    /* Return the entry or NU_NULL. */
    return nc_entry;

} /* NC6PPP_Find_NeighCache_Entry */



/*************************************************************************
*
*   FUNCTION
*
*       NC6PPP_CleanUp_Entry
*
*   DESCRIPTION
*
*       This function cleans up the members of a Neighbor Cache entry.
*
*   INPUTS
*
*       *nc_entry               A pointer to the Neighbor Cache entry.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NC6PPP_CleanUp_Entry(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    NET_BUFFER          *current_packet;

    /* If there are any buffers being used by this entry, release them. */
    if (nc_entry->ip6_neigh_cache_packet_list.head)
    {
        /* Get the first buffer from the list */
        current_packet = MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);

        /* While there are packets in the list, place them on their
           respective dlists. */
        while (current_packet)
        {
            /* Put the buffer on its dlist */
            MEM_Multiple_Buffer_Chain_Free(current_packet);

            /* Get the next buffer in the list */
            current_packet = MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);
        }
    }

    /* Zero out the entry. */
    UTL_Zero(nc_entry, sizeof(IP6_NEIGHBOR_CACHE_ENTRY));

} /* NC6PPP_CleanUp_Entry */

#endif
