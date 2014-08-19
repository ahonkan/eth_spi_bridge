/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       rtab4_ur.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for updating an IPv4 route.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Update_Route
*       RTAB4_Create_Subnet_Mask
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

STATIC VOID RTAB4_Create_Subnet_Mask(UINT8 *subnet_mask, INT prefix_length);

extern RTAB_ROUTE_PARMS RTAB4_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Update_Route
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
*       NU_NO_ACTION            The route is currently in use.
*
*************************************************************************/
STATUS RTAB4_Update_Route(const UINT8 *ip_address, const UINT8 *gateway,
                          const UPDATED_ROUTE_NODE *new_route)
{
    STATUS              status = NU_SUCCESS;
    RTAB4_ROUTE_ENTRY   *rt_entry;
    ROUTE_NODE          *default_route = NU_NULL;
    SCK_SOCKADDR_IP     de;
    UINT8               next_hop[IP_ADDR_LEN];

    /* Default variables for a new route */
    UINT8           default_mask[IP_ADDR_LEN] = {255, 255, 255, 255};
    UINT8           default_gateway[IP_ADDR_LEN] = {0, 0, 0, 0};
    DV_DEVICE_ENTRY *default_device;
    UINT32          default_flags = RT_UP;

    /* SNMP variables */
#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8           gw_addr[IP_ADDR_LEN];
    UINT8           dest_addr[IP_ADDR_LEN];
    INT32           type;
    INT32           proto;
#endif

    /* If the destination address is the default route, get a pointer to
     * the default route.
     */
    if (IP_ADDR(ip_address) == 0)
    {
        default_route = RTAB4_Get_Default_Route();
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
            de.sck_family = SK_FAM_IP;
            de.sck_len = sizeof(SCK_SOCKADDR_IP);
            de.sck_addr = IP_ADDR(ip_address);

            rt_entry = RTAB4_Find_Route(&de, RT_HOST_MATCH | RT_BEST_METRIC |
                                             RT_OVERRIDE_METRIC |
                                             RT_OVERRIDE_RT_STATE |
                                             RT_OVERRIDE_DV_STATE);
        }

        /* Find the route with the specific gateway */
        else
            rt_entry = (RTAB4_ROUTE_ENTRY*)
                       RTAB4_Find_Route_By_Gateway(ip_address, gateway,
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
#if (INCLUDE_SR_SNMP == NU_TRUE)

        PUT32(gw_addr, 0, rt_entry->rt_gateway_v4.sck_addr);
        memcpy(dest_addr, ip_address, IP_ADDR_LEN);

        if (rt_entry->rt_flags & RT_GATEWAY)
            type = 4;
        else
            type = 3;

        /* Delete this route from the SNMP routing table */
        SNMP_ipRouteTableUpdate(SNMP_DELETE, (UINT32)rt_entry->rt_device->dev_index,
                                dest_addr, (INT32)rt_entry->rt_metric,
                                (UNSIGNED)-1, (UNSIGNED)-1, (UNSIGNED)-1,
                                (UNSIGNED)-1, gw_addr, type, 2,
                                (INT32)rt_entry->rt_clock,
                                rt_entry->rt_route_node->rt_submask, "0.0");
#endif

        /* We can update the route only if it is not in use, unless this
         * is a route being updated by RIP-II, in which case, we need to
         * update it regardless of the route state.
         */
        if ( (IP_ADDR(rt_entry->rt_route_node->rt_ip_addr) ==
              IP_ADDR(ip_address)) &&
             ((rt_entry->rt_refcnt == 0) || (rt_entry->rt_flags & RT_RIP2)) )
        {
            /* The following parameters can only be changed if the route is not
             * in use.
             */
            if (rt_entry->rt_refcnt == 0)
            {
                /* Update the route's device to the new device */
                if ((INT)new_route->urt_dev.urt_dev_name != -1)
                    rt_entry->rt_device =
                        DEV_Get_Dev_By_Name(new_route->urt_dev.urt_dev_name);

                /* Copy the new submask into the route's data structure as long
                 * as this is not the default route.  The subnet mask of the
                 * default route cannot be changed.
                 */
                if ( (!default_route) && ((INT)new_route->urt_prefix_length != -1) )
                {
                    /* If this is a network route, and we are about to change
                     * the subnet mask, remove the node from the list of
                     * subnet mask nodes for the appropriate node.
                     */
                    if (rt_entry->rt_route_node->rt_submask_length != 32)
                    {
                        /* Remove the entry from the list */
                        DLL_Remove(&rt_entry->rt_route_node->rt_submask_list_parent
                                   ->rt_submask_list, rt_entry->rt_route_node);

                        /* Set the parent as NULL */
                        rt_entry->rt_route_node->rt_submask_list_parent = NU_NULL;
                    }

                    /* Fill in the subnet mask according to the prefix length */
                    RTAB4_Create_Subnet_Mask(rt_entry->rt_route_node->rt_submask,
                                             new_route->urt_prefix_length);

                    /* Find the length of the new subnet mask */
                    rt_entry->rt_route_node->rt_submask_length =
                        (UINT8)new_route->urt_prefix_length;

                    /* If the new network mask makes this a network route,
                     * add the node to the appropriate node's subnet mask list.
                     */
                    if (rt_entry->rt_route_node->rt_submask_length != 32)
                        RTAB_Establish_Subnet_Mask_Links(rt_entry->rt_route_node,
                                                         &RTAB4_Parms);
                }
            }

            /* Determine the new route flags */
            if (new_route->urt_flags != -1)
            {
                /* If the route is being updated to use RIP-II routing rules, the
                 * route should not be flagged as STATIC anymore so the Garbage
                 * Collection and Deletion rules will apply to the route.
                 */
                if ( (rt_entry->rt_flags & RT_STATIC) &&
                     (new_route->urt_flags & RT_RIP2) )
                    rt_entry->rt_flags &= ~RT_STATIC;

                rt_entry->rt_flags |= (UINT32)new_route->urt_flags;

                /* Remove the gateway flag */
                if (new_route->urt_flags & RT_NOT_GATEWAY)
                    rt_entry->rt_flags &= ~(RT_GATEWAY | RT_NOT_GATEWAY);

                /* Flag the route as down */
                else if ( (new_route->urt_flags & RT_UP) == 0)
                {
                    status = RTAB4_Delete_Route(ip_address, gateway);

                    if (status != NU_SUCCESS)
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
                    /* If this is not the default route, add a new node.
                     * There is only one default route allowed.
                     */
                    if ( (IP_ADDR(new_route->urt_dest) !=
                          IP_ADDR(rt_entry->rt_route_node->rt_ip_addr)) &&
                          (!default_route) )
                    {
                        /* Add the new route */
                        status = RTAB4_Add_Route(rt_entry->rt_device,
                                                 IP_ADDR(new_route->urt_dest),
                                                 IP_ADDR(rt_entry->rt_route_node->
                                                 rt_submask),
                                                 rt_entry->rt_gateway_v4.sck_addr,
                                                 rt_entry->rt_flags);

                        if (status == NU_SUCCESS)
                        {
                            /* Store the gateway */
                            PUT32(next_hop, 0, rt_entry->rt_gateway_v4.sck_addr);

                            /* Delete the specific route */
                            status = RTAB4_Delete_Route(rt_entry->rt_route_node->
                                                        rt_ip_addr, next_hop);

                            /* Get a pointer to the new route */
                            rt_entry = (RTAB4_ROUTE_ENTRY*)
                                       RTAB4_Find_Route_By_Gateway(new_route->urt_dest,
                                                                   next_hop,
                                                                   RT_HOST_MATCH);

                            if (rt_entry)
                                rt_entry->rt_refcnt --;

                            else
                                status = NU_INVAL;
                        }
                    }
                }

                /* Update the gateway of the route */
                if ( ((INT)new_route->urt_gateway != -1) && (rt_entry) )
                {
                    rt_entry->rt_gateway_v4.sck_addr =
                        IP_ADDR(new_route->urt_gateway);
                }

                /* Change the Path MTU associated with the route */
                if ( (new_route->urt_path_mtu != -1) && (rt_entry) )
                    rt_entry->rt_path_mtu = (UINT32)new_route->urt_path_mtu;
            }

            /* The target route may have changed.  Check that it is valid. */
            if (rt_entry)
            {
                /* Update the metric of the route */
                if (new_route->urt_metric.urt4_metric != -1)
                    rt_entry->rt_metric =
                        (UINT32)new_route->urt_metric.urt4_metric;

                /* Change the age of the route */
                if (new_route->urt_age != -1)
                    rt_entry->rt_clock =
                        (UINT32)(NU_Retrieve_Clock() - (new_route->urt_age * NU_PLUS_Ticks_Per_Second));
                else
                    rt_entry->rt_clock = NU_Retrieve_Clock();

                /* Change the route tag associated with the route */
                if (new_route->urt_routetag != -1)
                    rt_entry->rt_entry_parms.rt_parm_routetag = (UINT16)new_route->urt_routetag;
            }
        }

        else
            status = NU_NO_ACTION;
    }

    else if ( (new_route->urt_flags & RT_UP) == 0)
        status = NU_INVAL;

    else
    {
        /* If a subnet mask was not specified, use the default */
        if (new_route->urt_prefix_length != -1)
            RTAB4_Create_Subnet_Mask(default_mask,
                                     new_route->urt_prefix_length);

        /* If a gateway was not specified, use the default */
        if ((INT)new_route->urt_gateway != -1)
            memcpy(default_gateway, new_route->urt_gateway, IP_ADDR_LEN);

        /* If a device name was not specified, use the default */
        if ((INT)new_route->urt_dev.urt_dev_name != -1)
            default_device = DEV_Get_Dev_By_Name(new_route->urt_dev.urt_dev_name);
        else
            default_device = DEV_Get_Dev_By_Index(0);

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
        status = RTAB4_Add_Route(default_device, IP_ADDR(ip_address),
                                 IP_ADDR(default_mask), IP_ADDR(default_gateway),
                                 default_flags);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the route */
            rt_entry = (RTAB4_ROUTE_ENTRY*)
                       RTAB4_Find_Route_By_Gateway(ip_address, default_gateway,
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

#if (INCLUDE_SR_SNMP == NU_TRUE)

    if ( (rt_entry) && ((new_route->urt_flags == -1) || (new_route->urt_flags & RT_UP) != 0))
    {
        PUT32(gw_addr, 0, rt_entry->rt_gateway_v4.sck_addr);

        if (rt_entry->rt_flags & RT_GATEWAY)
            type = 4;
        else
            type = 3;

        if (rt_entry->rt_flags & RT_NETMGMT)
            proto = 3;
        else
            proto = 2;

        /* Add this route from to the SNMP routing table */
        SNMP_ipRouteTableUpdate(SNMP_ADD,
                                (UINT32)(rt_entry->rt_device->dev_index),
                                rt_entry->rt_route_node->rt_ip_addr,
                                (INT32)rt_entry->rt_metric,
                                (UNSIGNED)-1, (UNSIGNED)-1, (UNSIGNED)-1,
                                (UNSIGNED)-1, gw_addr, type, proto,
                                (INT32)rt_entry->rt_clock,
                                rt_entry->rt_route_node->rt_submask,
                                "0.0");
    }
#endif

    return (status);

} /* RTAB4_Update_Route */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Create_Subnet_Mask
*
*   DESCRIPTION
*
*       This function fills in the bytes of the array of a subnet mask
*       based on a prefix length.
*
*   INPUTS
*
*       *subnet_mask            A pointer to the memory to fill in with
*                               the subnet mask.
*       prefix_length           The bit length of the subnet mask to
*                               create.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RTAB4_Create_Subnet_Mask(UINT8 *subnet_mask, INT prefix_length)
{
    INT     i;

    /* Zero out the subnet mask */
    memset(subnet_mask, 0, IP_ADDR_LEN);

    /* Fill in the 255's of the subnet mask */
    for (i = 0; prefix_length > 8; i++, prefix_length -= 8)
        subnet_mask[i] = 0xff;

    /* Fill in the final portion of the subnet mask */
    while (prefix_length > 0)
    {
        subnet_mask[i] |= (1 << prefix_length);

        prefix_length --;
    }

} /* RTAB4_Create_Subnet_Mask */

#endif
