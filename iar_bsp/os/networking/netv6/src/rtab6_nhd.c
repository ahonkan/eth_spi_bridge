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
*       rtab6_nhd.c                                  
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the function for determining the
*       next hop.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Next_Hop_Determination
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       prefix6.h
*       defrtr6.h
*       nc6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/prefix6.h"
#include "networking/defrtr6.h"
#include "networking/nc6.h"

extern ROUTE_NODE   *RTAB6_Default_Route;
extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       RTAB6_Next_Hop_Determination
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function performs Next Hop Determination as described in 
*       RFC 4861 section 5.2.  It returns the route associated with the
*       next hop to the destination.
*
*       Next-Hop Determination for a given unicast destination operates
*       as follows.  The sender performs a longest Prefix Match against
*       the Prefix List to determine whether the packet's destination is
*       on or off-link.  If the destination is on-link, the next-hop
*       address is the same as the packet's destination address.  
*       Otherwise, the sender selects a router from the Default Router
*       List.
*                                                                         
*   INPUTS                                                                
*                     
*                                                                         
*   OUTPUTS                                                               
*                                                           
*       None
*
*************************************************************************/
RTAB6_ROUTE_ENTRY *RTAB6_Next_Hop_Determination(UINT8 *ip_addr)
{
    RTAB6_ROUTE_ENTRY           *rt_entry;
    IP6_DEFAULT_ROUTER_ENTRY    *default_router;
    IP6_NEIGHBOR_CACHE_ENTRY    *neighcache_entry = NU_NULL;
    DV_DEVICE_ENTRY             *current_device, *new_device;
    UINT8                       *next_hop = NU_NULL;
    UINT32                      flags;

    /* If a corresponding entry already exists in the route table,
     * return the entry.  Otherwise, create a new entry.
     */
    rt_entry = (RTAB6_ROUTE_ENTRY*)
               RTAB_Find_Route_Entry(ip_addr, &RTAB6_Parms, 0);

    if ( ((rt_entry == NU_NULL) ||
           ((rt_entry->rt_next_hop_entry == NU_NULL) &&
           (rt_entry->rt_entry_parms.rt_parm_flags & RT_GATEWAY))) &&
          (!(IPV6_IS_ADDR_MULTICAST(ip_addr))) )
    {
        /* If a route was found with no next-hop entry, but a next-hop is
         * specified, set next_hop to the specified next-hop and add a 
         * Neighbor Cache entry back in for the route.
         */
        if ( (rt_entry) &&
             (memcmp(rt_entry->rt_next_hop.sck_addr, 
                     "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", IP6_ADDR_LEN) != 0) )
        {
            next_hop = rt_entry->rt_next_hop.sck_addr;
            current_device = rt_entry->rt_entry_parms.rt_parm_device;
        }

        else
        {
            current_device = DEV_Table.dv_head;

            /* Perform a longest prefix match against the Prefix List to determine
             * whether the packet's destination is on or off-link.
             */
            while (current_device)
            {
                if (current_device->dev_flags & DV6_IPV6)
                {
                    /* If the prefix matches an on-link Prefix. */
                    if (PREFIX6_Match_Longest_Prefix_By_Device(current_device->dev6_prefix_list, 
                                                               ip_addr) != NU_NULL)
                    {
                        /* Set the next hop to the address. */
                        next_hop = ip_addr;
                        break;
                    }
                }

                current_device = current_device->dev_next;
            }

            /* If the destination is not on-link, select a router from the Default 
             * Router List.
             */
            if (current_device == NU_NULL)
            {
                /* If the Default Router List is not empty, get the Neighbor Cache
                 * entry associated with the Default Router.
                 */
                default_router = DEFRTR6_Find_Default_Router();

                if (default_router != NU_NULL)
                {
                    /* Set the Neighbor Cache entry to point at the Default Router's
                     * Neighbor Cache entry.
                     */
                    neighcache_entry = default_router->ip6_def_rtr_nc_entry;

                    /* Set the device and next hop. */
                    current_device = default_router->ip6_def_rtr_device;
                    next_hop = default_router->ip6_def_rtr_ip_addr;
                }

                /* If the Default Router List is empty, check for a default
                 * route.
                 */
                else
                {
                    /* If there is a default route, use the route */
                    if (RTAB6_Default_Route)
                    {
                        current_device = ((RTAB6_ROUTE_ENTRY*)
                                   (RTAB6_Default_Route->rt_list_head))
                                   ->rt_entry_parms.rt_parm_device;

                        next_hop = ((RTAB6_ROUTE_ENTRY*)
                                   (RTAB6_Default_Route->rt_list_head))
                                   ->rt_next_hop.sck_addr;
                    }
                }
            }
        }

        if (next_hop)
        {
            /* If a matching Neighbor Cache entry does not already exist for the
             * next_hop IP address, add a new entry. 
             */
            if ( (!neighcache_entry) && (current_device) &&
                 (neighcache_entry = 
                    current_device->dev6_fnd_neighcache_entry(current_device, 
                                                              next_hop)) == NU_NULL)
            {
                neighcache_entry = 
                    current_device->dev6_add_neighcache_entry(current_device,
                                                              next_hop, NU_NULL, 
                                                              0, NU_NULL,
                                                              NC_NEIGH_INCOMPLETE);
            }

            /* If a route did not already exist, add a new entry. */
            if ( (!rt_entry) && (neighcache_entry) )
            {
                new_device = 
                    DEV6_Get_Dev_By_Addr(neighcache_entry->ip6_neigh_cache_ip_addr);

                if (new_device != NU_NULL)
                {
                    current_device = new_device;
                    flags = RT_UP;
                }
                else
                    flags = (RT_UP | RT_GATEWAY);

                flags |= (RT_LOCAL | RT_HOST | RT_STATIC);

                if (RTAB6_Add_Route(current_device, ip_addr, 
                                    neighcache_entry->ip6_neigh_cache_ip_addr, 
                                    128, flags) == NU_SUCCESS)
                    rt_entry = (RTAB6_ROUTE_ENTRY*)
                               RTAB6_Find_Route_By_Gateway(ip_addr,
                                                           neighcache_entry->ip6_neigh_cache_ip_addr,
                                                           RT_HOST_MATCH);
            }

            /* A route already exists, but it does not have a corresponding 
             * Neighbor Cache entry.  Add the Neighbor Cache entry.
             */
            else if ( (rt_entry) && (neighcache_entry) )
            {
                /* Set the next hop of the route to the Neighbor Cache 
                 * entry. 
                 */
                rt_entry->rt_next_hop_entry = neighcache_entry;

                NU_BLOCK_COPY(rt_entry->rt_next_hop.sck_addr,
                              neighcache_entry->ip6_neigh_cache_ip_addr, IP6_ADDR_LEN);

                rt_entry->rt_next_hop.sck_family = NU_FAMILY_IP6;
                rt_entry->rt_next_hop.sck_len = sizeof(SCK6_SOCKADDR_IP);

                /* Add the route to the Destination List of the Neighbor 
                 * Cache entry.
                 */
                RTAB6_Add_DestList_Entry(&neighcache_entry->ip6_neigh_cache_dest_list,
                                         rt_entry);                
            }  
        }
    }

    return (rt_entry);

} /* RTAB6_Next_Hop_Determination */
