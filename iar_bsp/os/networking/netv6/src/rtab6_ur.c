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
*       rtab6_ur.c                                   
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for updating an IPv6 route.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Update_Route
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       nu_net6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"

extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB6_Update_Route
*
*   DESCRIPTION
*
*       This function updates the specified members of the 
*       UPDATED_ROUTE_NODE data structure of a given route.
*
*   INPUTS
*
*       *ip_address             A pointer to the IP address of the 
*                               associated route to update.
*       *gateway                The next-hop associated with the route
*                               or NU_NULL if unspecified.
*       *new_route              A pointer to the data structure holding the
*                               updated parameters.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_INVAL                The route is not valid
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATUS RTAB6_Update_Route(const UINT8 *ip_address, const UINT8 *gateway, 
                          const UPDATED_ROUTE_NODE *new_route)
{
    STATUS              status = NU_SUCCESS;
    RTAB6_ROUTE_ENTRY   *rt_entry;
    ROUTE_NODE          *default_route;
    UINT8               next_hop[IP6_ADDR_LEN];

    /* Default variables for a new route */
    DV_DEVICE_ENTRY *default_device;
    UINT32          default_flags = RT_UP;
    UINT8           default_prefix_len = 128;
    UINT8           default_gateway[IP6_ADDR_LEN] =
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /* If the destination address is the default route, get a pointer to
     * the default route.
     */
    if (IPV6_IS_ADDR_UNSPECIFIED(ip_address))
    {
        default_route = RTAB6_Get_Default_Route();
        rt_entry = default_route->rt_list_head;
    }

    /* Otherwise, get a pointer to the route to update */
    else
    {
        /* If a gateway was not specified, the first route matching the
         * route flags passed in will be returned.  Return the route 
         * regardless of the state of the metric, route or device.
         */
        if (gateway == NU_NULL)
        {
            rt_entry = RTAB6_Find_Route(ip_address, RT_HOST_MATCH | 
                                                    RT_BEST_METRIC |
                                                    RT_OVERRIDE_METRIC | 
                                                    RT_OVERRIDE_RT_STATE | 
                                                    RT_OVERRIDE_DV_STATE);
        }
        
        /* Find the route with the specific gateway */
        else
            rt_entry = (RTAB6_ROUTE_ENTRY*)
                       RTAB6_Find_Route_By_Gateway(ip_address, gateway, 
                                                   RT_HOST_MATCH | 
                                                   RT_OVERRIDE_METRIC | 
                                                   RT_OVERRIDE_RT_STATE | 
                                                   RT_OVERRIDE_DV_STATE);

        /* Free the route */
        if (rt_entry)                    
            rt_entry->rt_refcnt --;
    }

    /* If a route was found */
    if (rt_entry)
    {
        /* We can update the route only if it is not in use, unless this 
         * is a route being updated by RIPng, in which case, we need to 
         * update it regardless of the route state.
         */
        if ( (memcmp(rt_entry->rt_route_node->rt_ip_addr, 
                     ip_address, IP6_ADDR_LEN) == 0) && 
             ((rt_entry->rt_refcnt == 0) || (rt_entry->rt_flags & RT_RIPNG)) )
        {
            /* The following parameters can only be changed if the route is not
             * in use.
             */
            if (rt_entry->rt_refcnt == 0)
            {
                /* Update the route's device to the new device */
                if (new_route->urt_dev.urt_dev_index != -1)
                    rt_entry->rt_device = 
                        DEV_Get_Dev_By_Index((UINT32)new_route->urt_dev.urt_dev_index);

                /* Update the prefix length of the entry */
                if ((INT)new_route->urt_prefix_length != -1)
                {
                    /* If this is a network route, and we are about to change
                     * the subnet mask, remove the node from the list of
                     * subnet mask nodes for the appropriate node.
                     */
                    if (rt_entry->rt_route_node->rt_submask_length != 128)
                    {
                        /* Remove the entry from the list */
                        DLL_Remove(&rt_entry->rt_route_node->rt_submask_list_parent
                                   ->rt_submask_list, rt_entry->rt_route_node);

                        /* Set the parent as NULL */
                        rt_entry->rt_route_node->rt_submask_list_parent = NU_NULL;
                    }

                    /* Update the length of the new subnet mask */
                    rt_entry->rt_route_node->rt_submask_length = 
                        (UINT8)new_route->urt_prefix_length;

                    /* If the new network mask makes this a network route,
                     * add the node to the appropriate node's subnet mask list.
                     */
                    if (rt_entry->rt_route_node->rt_submask_length != 128)
                        RTAB_Establish_Subnet_Mask_Links(rt_entry->rt_route_node,
                                                         &RTAB6_Parms);
                }
            }

            /* Determine the new route flags */
            if (new_route->urt_flags != -1)
            {
                /* If the route is being updated to use RIPng routing rules, the
                 * route should not be flagged as STATIC anymore so the Garbage
                 * Collection and Deletion rules will apply to the route.
                 */
                if ( (rt_entry->rt_flags & RT_STATIC) &&
                     (new_route->urt_flags & RT_RIPNG) )
                    rt_entry->rt_flags &= ~RT_STATIC;    

                rt_entry->rt_flags |= (UINT32)new_route->urt_flags;

                /* Remove the gateway flag */
                if (new_route->urt_flags & RT_HOST)
                    rt_entry->rt_flags &= ~RT_GATEWAY;
    
                /* Remove the host flag */
                else if (new_route->urt_flags & RT_GATEWAY)
                    rt_entry->rt_flags &= (~RT_HOST);

                /* Flag the route as down */
                else if ( (new_route->urt_flags & RT_UP) == 0)
                {   
                    if ((status = RTAB6_Delete_Route(ip_address, 
                                                     (UINT8 *)gateway)) != NU_SUCCESS)
                        status = NU_INVAL;
                }
            }

            /* The following items can only be changed if the route is not
             * in use.
             */
            if (rt_entry->rt_refcnt == 0)
            {
                /* To change the rt_dest, we must add a new route with that 
                 * rt_dest, then delete the old route.  We must then get a 
                 * pointer to the new route so that any subsequent changes 
                 * made to the route are actually made.
                 */
                if ((INT)new_route->urt_dest != -1)
                {
                    if (memcmp(new_route->urt_dest, 
                               rt_entry->rt_route_node->rt_ip_addr, 
                               IP6_ADDR_LEN) != 0)
                    {
                        /* Add the new route */
                        status = RTAB6_Add_Route(rt_entry->rt_device, 
                                                 new_route->urt_dest, 
                                                 rt_entry->rt_next_hop.sck_addr,
                                                 rt_entry->rt_route_node->rt_submask_length,
                                                 rt_entry->rt_flags);

                        if (status == NU_SUCCESS)
                        {
                            /* Copy the next-hop since we are about to delete
                             * the route and thus invalidate the memory.
                             */
                            memcpy(next_hop, rt_entry->rt_next_hop.sck_addr, 
                                   IP6_ADDR_LEN);

                            /* Delete the specific route */
                            status = RTAB6_Delete_Route(rt_entry->rt_route_node->rt_ip_addr, 
                                                        rt_entry->rt_next_hop.sck_addr);

                            /* Get a pointer to the new route */
                            rt_entry = (RTAB6_ROUTE_ENTRY*)
                                    RTAB6_Find_Route_By_Gateway(new_route->urt_dest, 
                                                                next_hop,
                                                                RT_HOST_MATCH);
                            if (rt_entry)
                                rt_entry->rt_refcnt --;

                            else
                                return (NU_INVAL);
                        }
                    }
                }

                /* Update the gateway of the route */
                if ((INT)new_route->urt_gateway != -1)
                    NU_BLOCK_COPY(rt_entry->rt_next_hop.sck_addr, 
                                  new_route->urt_gateway, IP6_ADDR_LEN);

                /* Change the Path MTU associated with the route */
                if (new_route->urt_path_mtu != -1)
                    rt_entry->rt_path_mtu = (UINT32)new_route->urt_path_mtu;
            }

            /* Update the metric of the route */
            if (new_route->urt_metric.urt6_metric != -1)
                rt_entry->rt_metric = 
                    (UINT8)new_route->urt_metric.urt6_metric;
  
            /* Change the age of the route */
            if (new_route->urt_age != -1)
                rt_entry->rt_entry_parms.rt_parm_clock = 
                    (UINT32)(NU_Retrieve_Clock() - (new_route->urt_age * NU_PLUS_Ticks_Per_Second));
            else
                rt_entry->rt_entry_parms.rt_parm_clock = NU_Retrieve_Clock();

            /* Change the route tag associated with the route */
            if (new_route->urt_routetag != -1)
                rt_entry->rt_entry_parms.rt_parm_routetag = (UINT16)new_route->urt_routetag;
        }

        else
            status = NU_NO_ACTION;
    }

    else if ( (new_route->urt_flags & RT_UP) == 0)
        status = NU_INVAL;

    else
    {
        /* If a prefix length was not specified, use the default */
        if (new_route->urt_prefix_length != -1)
            default_prefix_len = (UINT8)new_route->urt_prefix_length;

        /* If a gateway was not specified, use the default */
        if ((INT)new_route->urt_gateway != -1)
            memcpy(default_gateway, new_route->urt_gateway, IP_ADDR_LEN);

        /* If a device name was not specified, use the default */
        if (new_route->urt_dev.urt_dev_index != -1)
            default_device = 
                DEV_Get_Dev_By_Index((UINT32)new_route->urt_dev.urt_dev_index);
        else
            default_device = DEV6_Get_Primary_Interface();

        /* If the flags were not specified, use the default */
        if (new_route->urt_flags != -1)
            default_flags = (UINT32)new_route->urt_flags;

        if (new_route->urt_flags & (INT32)RT_NETMGMT)
            default_flags |= RT_NETMGMT;
        else if (new_route->urt_flags & (INT32)RT_LOCAL)
            default_flags |= RT_LOCAL;
        
        /* If we successfully added the route, check that any other parameters to
         * the route need to be set 
         */
        status = RTAB6_Add_Route(default_device, ip_address, default_gateway, 
                                 default_prefix_len, default_flags);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the route */
            rt_entry = (RTAB6_ROUTE_ENTRY*)
                       RTAB6_Find_Route_By_Gateway(ip_address, default_gateway,
                                                   RT_HOST_MATCH);

            if (rt_entry)
            {
                rt_entry->rt_refcnt --;

                if (new_route->urt_age != -1)
                    rt_entry->rt_entry_parms.rt_parm_clock = 
                        (UINT32)(NU_Retrieve_Clock() - (new_route->urt_age * TICKS_PER_SECOND));

                if (new_route->urt_path_mtu != -1)
                    rt_entry->rt_path_mtu = (UINT32)new_route->urt_path_mtu;
            }
        }
    }

    return (status);

} /* RTAB6_Update_Route */
