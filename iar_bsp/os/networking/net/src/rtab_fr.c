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
*       rtab_fr.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions for traversing the routing
*       table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB_Find_Route_Entry
*       RTAB_Free
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Find_Route_Entry
*
*   DESCRIPTION
*
*       This function either finds the route associated with the target
*       or a network route associated with the target.  If a host route
*       does not exist for the destination, a network route will be
*       returned if one exists and the caller will accept network routes.
*
*   INPUTS
*
*       *target_address         A pointer to the destination IP address
*                               of the route to find.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*       flags                   Flags indicating what routes to return:
*
*                               RT_HOST_MATCH        Return only a host
*                                                    route.
*                               RT_BEST_METRIC       Find the route with
*                                                    the best metric
*                               RT_OVERRIDE_METRIC   Find a route even if
*                                                    the metric is
*                                                    infinity
*                               RT_OVERRIDE_DV_STATE Find a route even if
*                                                    the device is not
*                                                    UP and RUNNING
*                               RT_OVERRIDE_RT_STATE Find a route even if
*                                                    the route is not UP.
*
*   OUTPUTS
*
*       *ROUTE_NODE             A pointer to the route entry
*       NU_NULL                 No route exists
*
*************************************************************************/
ROUTE_ENTRY *RTAB_Find_Route_Entry(const UINT8 *target_address,
                                   const RTAB_ROUTE_PARMS *rt_parms,
                                   INT32 flags)
{
    ROUTE_NODE      *current_route, *previous_route;
    ROUTE_NODE      *current_network_route;
    UINT8           best_length = 0;
    ROUTE_ENTRY     *rt_entry, *best_network_route = NU_NULL,
                    *best_metric = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    current_route = *(rt_parms->rt_root_node);

    while (current_route)
    {
        /* Save a pointer to the route */
        previous_route = current_route;

        /* If there are subnets associated with the current node, check if
         * any of the subnets represent network routes for the target.
         */
        if ( (!(flags & RT_HOST_MATCH)) &&
             (current_route->rt_submask_list.submask_head) )
        {
            /* Get a pointer to the first network route node in the list */
            current_network_route =
                current_route->rt_submask_list.submask_head;

            do
            {
                /* If the length of the current subnet mask is greater than
                 * the saved subnet mask, determine if the prefix covers the
                 * target address.
                 */
                if (current_network_route->rt_submask_length > best_length)
                {
                    /* If the target address is covered by the prefix */
                    if (rt_parms->rt_determine_matching_prefix(target_address,
                                                               current_network_route->rt_ip_addr,
                                                               current_network_route->rt_submask,
                                                               current_network_route->rt_submask_length)
                                                               == NU_TRUE)
                    {
                        rt_entry = current_network_route->rt_list_head;

                        /* Find a valid route */
                        do
                        {
                            /* If the route metric is valid, the route is up and
                             * the interface for the route is up, use this route.
                             */
                            if ( ((rt_entry->rt_entry_parms.rt_parm_metric != RT_INFINITY) ||
                                  (flags & RT_OVERRIDE_METRIC)) &&
                                 ((rt_entry->rt_entry_parms.rt_parm_flags & RT_UP) ||
                                  (flags & RT_OVERRIDE_RT_STATE)) &&
                                 ((rt_entry->rt_entry_parms.rt_parm_device->dev_flags & DV_UP) ||
                                  (flags & RT_OVERRIDE_DV_STATE)) )
                            {
                                /* If the RT_BEST_METRIC flag is set, determine
                                 * if this route has a better metric than another
                                 * route for the same node.
                                 */
                                if (flags & RT_BEST_METRIC)
                                {
                                    /* If there is not already a best_metric found or
                                     * this route has a shorter metric than the one
                                     * already found, set the best_metric to this route.
                                     */
                                    if ( (best_metric == NU_NULL) ||
                                         (best_metric->rt_entry_parms.rt_parm_metric >
                                          rt_entry->rt_entry_parms.rt_parm_metric) )
                                        best_metric = rt_entry;
                                }

                                /* Otherwise, use the first valid route found */
                                else
                                    break;
                            }

                            rt_entry = rt_entry->rt_flink;

                        } while (rt_entry);

                        /* If looking for the shortest metric node */
                        if (flags & RT_BEST_METRIC)
                        {
                            rt_entry = best_metric;
                            best_metric = NU_NULL;
                        }

                        /* If a valid route exists, save it */
                        if (rt_entry)
                        {
                            /* Save a pointer to the best network route */
                            best_network_route = rt_entry;

                            /* Save the best length */
                            best_length = current_network_route->rt_submask_length;

                            /* Do not check any other subnet masks in the list.
                             * Since the list is ordered from most-specific to
                             * least-specific, the first match found will be the
                             * best match in the list.
                             */
                            break;
                        }
                    }
                }

                /* If the most specific subnet mask in the list is not better
                 * than the subnet mask already found, do not check any other
                 * subnet masks in the list.
                 */
                else
                    break;

                current_network_route = current_network_route->rt_next;

            } while (current_network_route);
        }

        /* Set current_route to the next child node */
        current_route =
            current_route->rt_child[RTAB_Determine_Branch(current_route->rt_bit_index,
                                                          target_address,
                                                          rt_parms)];

        /* If we followed a link upward, break out of the loop. */
        if ( (current_route) &&
             (current_route->rt_bit_index >= previous_route->rt_bit_index) )
            break;
    }

    /* Determine if a host route was found */
    if (current_route)
    {
        /* If this is a host route match */
        if (memcmp(current_route->rt_ip_addr, target_address,
                   rt_parms->rt_byte_ip_len) == 0)
        {
            rt_entry = current_route->rt_list_head;

            /* Use the first route that is valid */
            while (rt_entry)
            {
                /* If the route metric for this route is valid, the route
                 * is up and the interface for the route is up, use this
                 * route.
                 */
                if ( ((rt_entry->rt_entry_parms.rt_parm_metric != RT_INFINITY) ||
                      (flags & RT_OVERRIDE_METRIC)) &&
                     ((rt_entry->rt_entry_parms.rt_parm_flags & RT_UP) ||
                      (flags & RT_OVERRIDE_RT_STATE)) &&
                     ((rt_entry->rt_entry_parms.rt_parm_device->dev_flags & DV_UP) ||
                      (flags & RT_OVERRIDE_DV_STATE)) )
                {
                    /* If the RT_BEST_METRIC flag is set, determine
                     * if this route has a better metric than another
                     * route for the same node.
                     */
                    if (flags & RT_BEST_METRIC)
                    {
                        /* If there is not already a best_metric found or
                         * this route has a shorter metric than the one
                         * already found, set the best_metric to this route.
                         */
                        if ( (best_metric == NU_NULL) ||
                             (best_metric->rt_entry_parms.rt_parm_metric >
                              rt_entry->rt_entry_parms.rt_parm_metric) )
                            best_metric = rt_entry;
                    }

                    /* Otherwise, use the first valid route found */
                    else
                        break;
                }

                rt_entry = rt_entry->rt_flink;

            }

            /* If looking for the shortest metric node */
            if (flags & RT_BEST_METRIC)
                rt_entry = best_metric;
        }

        /* A host route does not exist for the destination IP address */
        else
            rt_entry = NU_NULL;
    }

    /* A host route does not exist for the destination IP address */
    else
        rt_entry = NU_NULL;

    /* If a valid host route does not exist, check if a network route
     * exists.  The route has already been confirmed as valid in the
     * search code, so do not check that it is valid again.
     */
    if ( (!rt_entry) && (best_network_route) )
        rt_entry = best_network_route;

    /* Increment the reference count */
    if (rt_entry)
        rt_entry->rt_entry_parms.rt_parm_refcnt ++;

    /* Return to user mode */
    NU_USER_MODE();

    return (rt_entry);

} /* RTAB_Find_Route_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Free
*
*   DESCRIPTION
*
*       Initialize the default_route structure
*
*   INPUTS
*
*       *rt_entry               The route pointer node to remove
*       family                  The family of the route.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID RTAB_Free(ROUTE_ENTRY *rt_entry, INT16 family)
{
    STATUS  status;

#if ( (INCLUDE_IPV6 != NU_TRUE) || (INCLUDE_IPV4 != NU_TRUE) )
    /* Remove compiler warnings */
    UNUSED_PARAMETER(family);
#endif

    if ( (rt_entry) && (rt_entry->rt_entry_parms.rt_parm_refcnt > 0) )
    {
        rt_entry->rt_entry_parms.rt_parm_refcnt--;

        /* If the route has been flagged as down and has been freed by
         * the last process using the route, delete the route from the
         * system.
         */
        if ( (!(rt_entry->rt_entry_parms.rt_parm_flags & RT_UP)) &&
             (rt_entry->rt_entry_parms.rt_parm_refcnt == 0) )
        {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
            if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                status = RTAB6_Delete_Node(rt_entry->rt_route_node);

#if (INCLUDE_IPV4 == NU_TRUE)
            else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                status = RTAB4_Delete_Node(rt_entry->rt_route_node);
#endif

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to delete route",
                               NERR_SEVERE, __FILE__, __LINE__);

                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }
    }
    else
        NLOG_Error_Log("RTAB_Free cannot free the route",
                  NERR_INFORMATIONAL, __FILE__, __LINE__);

} /* RTAB_Free */
