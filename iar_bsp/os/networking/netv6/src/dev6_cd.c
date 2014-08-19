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
*       dev6_cd.c
*
*   DESCRIPTION
*
*       This file contains the functions necessary to clean up all IPv6
*       portions of an interface being removed from the system.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DEV6_Cleanup_Device
*       DEV6_Detach_Addrs_From_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*       nc6.h
*       prefix6.h
*       nd6radv.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nc6.h"
#include "networking/prefix6.h"
#include "networking/nd6radv.h"
#include "networking/defrtr6.h"

extern TQ_EVENT                     ICMP6_RtrSol_Event;
extern IP6_DEFAULT_ROUTER_LIST      *Default_Router_List;

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Cleanup_Device
*
*   DESCRIPTION
*
*       This function deallocates all IPv6-related memory from an
*       interface structure that is being deleted.
*
*   INPUTS
*
*       *dev                    A pointer to the device entry.
*       flags                   Flags regarding the cleanup.  This
*                               parameter is currently unused.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Cleanup_Device(DV_DEVICE_ENTRY *dev, UINT32 flags)
{
    INT16   i;

    UNUSED_PARAMETER(flags);

    /* If the IPv6 data structure was allocated. */
    if (dev->dev6_ipv6_data)
    {
#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

        /* If this interface has been flagged as a router, transmit a
         * final Router Advertisement to inform the nodes on the link
         * that the router will no longer be available.
         */
        if (dev->dev_flags & DV6_ISROUTER)
        {
            /* Set the lifetime to zero so the router will not be used as a
             * default router by any nodes on the link.
             */
            dev->dev6_AdvDefaultLifetime = 0;

            /* Set the next time for a Router Advertisement to be transmitted
             * to zero.
             */
            dev->dev6_next_radv_time = 0;

            /* Transmit the final Router Advertisement */
            ND6RADV_Output(dev);
        }

        /* Otherwise, if the interface is still undergoing startup,
         * unset the timer to insure a Router Solicitation will not be
         * transmitted.
         */
        else
#endif
            TQ_Timerunset(ICMP6_RtrSol_Event, TQ_CLEAR_ALL_EXTRA,
                          dev->dev_index, 0);

        /* Clean up all address information. */
        DEV6_Detach_Addrs_From_Device(dev);

        if (dev->dev6_neighbor_cache)
        {
            /* Delete each entry in the Neighbor Cache.  This must be done
             * to insure all routes that could be using the Neighbor Cache
             * entry are updated.
             */
            for (i = 0; (i < dev->dev6_nc_entries) &&
                        (dev->dev6_neighbor_cache[i].ip6_neigh_cache_link_spec); i++)
            {
                /* Deallocate the memory for the link-specific portion of this
                 * entry
                 */
                if (NU_Deallocate_Memory(dev->dev6_neighbor_cache[i].
                                         ip6_neigh_cache_link_spec) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory for the Neighbor Cache",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Deallocate the memory allocated for the Neighbor Cache */
            if (NU_Deallocate_Memory((VOID*)dev->dev6_neighbor_cache) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for the Neighbor Cache",
                               NERR_SEVERE, __FILE__, __LINE__);

            dev->dev6_neighbor_cache = NU_NULL;
        }

        /* Delete each Prefix Entry */
        if (dev->dev6_prefix_list)
        {
            /* Deallocate the memory allocated for the Prefix List */
            if (NU_Deallocate_Memory((VOID*)dev->dev6_prefix_list) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for the Prefix List",
                               NERR_SEVERE, __FILE__, __LINE__);

            dev->dev6_prefix_list = NU_NULL;
        }

        /* Deallocate the IPv6 data structure. */
        if (NU_Deallocate_Memory((VOID*)dev->dev6_ipv6_data) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the IPv6 specific data",
                           NERR_SEVERE, __FILE__, __LINE__);

        dev->dev6_ipv6_data = NU_NULL;
    }

} /* DEV6_Cleanup_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Detach_Addrs_From_Device
*
*   DESCRIPTION
*
*       This function removes all traces of all IPv6 addresses on an
*       interface from the system.
*
*   INPUTS
*
*       *dev                    A pointer to the device entry.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Detach_Addrs_From_Device(DV_DEVICE_ENTRY *dev)
{
    INT16                       i;
    IP6_DEFAULT_ROUTER_ENTRY    *defrtr_ptr = NU_NULL, *defrtr_next_ptr;
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    IP6_MULTI           *ipm;
#endif

    /* If the IPv6 data structure was allocated. */
    if (dev->dev6_ipv6_data)
    {
#if (INCLUDE_IP_MULTICASTING)
        /* Leave all IPv6 multicast groups to which the interface
         * belongs
         */
        for (i = 0, ipm = dev->dev6_multiaddrs;
             (i < IP6_MAX_MEMBERSHIPS) && (ipm);
             i++)
        {
            /* Leave the group */
            if (IP6_Delete_Multi(ipm) != NU_SUCCESS)
                NLOG_Error_Log("Failed to leave the multicast group",
                               NERR_SEVERE, __FILE__, __LINE__);

            /* If the multicast group was not successfully deleted, get
             * a pointer to the next multicast group for the interface.
             */
            if (ipm == dev->dev6_multiaddrs)
                ipm = ipm->ipm6_next;
            else
                ipm = dev->dev6_multiaddrs;
        }
#endif

        /* If the Default Router List and IP address are not NULL, find the
         * entry.
         */
        if (Default_Router_List)
        {
            /* Get a pointer to the head of the list */
            defrtr_ptr = Default_Router_List->dv_head;

            /* Delete all entries that use this interface. */
            while (defrtr_ptr)
            {
                /* Save a pointer to the next entry. */
                defrtr_next_ptr = defrtr_ptr->ip6_def_rtr_next;

                /* If this entry uses the target interface, delete it. */
                if (defrtr_ptr->ip6_def_rtr_device == dev)
                    DEFRTR6_Delete_Entry(defrtr_ptr);

                /* Get the next entry in the list */
                defrtr_ptr = defrtr_next_ptr;
            }
        }

        if (dev->dev6_neighbor_cache)
        {
            /* Delete each entry in the Neighbor Cache.  This must be done
             * to insure all routes that could be using the Neighbor Cache
             * entry are updated.
             */
            for (i = 0; (i < dev->dev6_nc_entries) &&
                        (dev->dev6_neighbor_cache[i].ip6_neigh_cache_link_spec); i++)
            {
                dev->dev6_del_neighcache_entry(dev,
                        dev->dev6_neighbor_cache[i].ip6_neigh_cache_ip_addr);
            }
        }

        /* Remove each IP address from the device */
        while (dev->dev6_addr_list.dv_head)
        {
            /* Delete the IP and clean up the routes and socket */
            DEV6_Delete_IP_From_Device(dev->dev6_addr_list.dv_head);
        }

        /* Delete each Prefix Entry */
        if (dev->dev6_prefix_list)
        {
            while (dev->dev6_prefix_list->dv_head)
                PREFIX6_Delete_Entry(dev->dev6_prefix_list->dv_head);
        }

        /* Delete all routes from this device. */
        RTAB6_Delete_Routes_For_Device(dev);
    }

} /* DEV6_Detach_Addrs_From_Device */
