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
*       rtab4_dr.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for accessing the IPv4 routing
*       table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Delete_Route
*       RTAB4_Delete_Route_From_Node
*
*   DEPENDENCIES
*
*       nu_net.h
*       udp4.h
*       tcp4.h
*       ipraw4.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_UDP == NU_TRUE)
#include "networking/udp4.h"
#endif

#if (INCLUDE_TCP == NU_TRUE)
#include "networking/tcp4.h"
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
#include "networking/ipraw4.h"
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

extern ROUTE_NODE   *RTAB4_Default_Route;

extern RTAB_ROUTE_PARMS RTAB4_Parms;

STATIC  STATUS RTAB4_Delete_Route_From_Node(ROUTE_NODE *, UINT32);

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Delete_Route
*
*   DESCRIPTION
*
*       Removes a route from the routing table.
*
*   INPUTS
*
*       *ip_dest                A pointer to the route you want to delete
*       *next_hop               The next-hop associated with the route
*                               to delete.  If NU_NULL, the entire route
*                               and all route entries will be deleted.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_NOT_FOUND            The route was not found
*       NU_INVALID_PARM         A pointer parameter is NU_NULL.
*
*************************************************************************/
STATUS RTAB4_Delete_Route(const UINT8 *ip_dest, const UINT8 *next_hop)
{
    STATUS              ret_status;
    RTAB4_ROUTE_ENTRY   *rt;
    SCK_SOCKADDR_IP     ro;

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) || (INCLUDE_IP_FORWARDING == NU_TRUE) )
    RTAB4_ROUTE_ENTRY   *rt_entry;
#endif

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    DV_DEVICE_ENTRY     *dv_ptr;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Verify pointer parameter is non-null. */
    if (ip_dest)
    {
        ro.sck_family = NU_FAMILY_IP;
        ro.sck_len = sizeof(ro);
        ro.sck_addr = IP_ADDR(ip_dest);

        /* Find the node that contains this route and remove it regardless
         * of the state of the route, the device or the value of the metric.
         */
        rt = RTAB4_Find_Route(&ro, RT_HOST_MATCH | RT_OVERRIDE_RT_STATE |
                              RT_OVERRIDE_DV_STATE | RT_OVERRIDE_METRIC);

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) || (INCLUDE_IP_FORWARDING == NU_TRUE) )

        /* A route was found. */
        if (rt)
        {
            rt_entry = rt;

            /* The reference count was incremented by RTAB4_Find_Route.
             * We are not using the route so decrement the reference count.
             */
            rt->rt_entry_parms.rt_parm_refcnt--;
        }

        /* This is the default route. */
        else if (ro.sck_addr == 0)
        {
            /* If we have set our default route. */
            if (RTAB4_Default_Route)
            {
                rt_entry = RTAB4_Default_Route->rt_list_head;
            }
            else
            {
                rt_entry = NU_NULL;
            }
        }

        /* No matching route was found. */
        else
            rt_entry = NU_NULL;

        /* If a matching route was found. */
        if (rt_entry)
        {
#if (INCLUDE_UDP == NU_TRUE)

            /* Remove any references to this route that may be cached
             * by UDP sockets.
             */
            UDP4_Free_Cached_Route(rt_entry);
#endif

#if (INCLUDE_TCP == NU_TRUE)

            /* Remove any references to this route that may be cached
             * by TCP sockets.
             */
            TCP4_Free_Cached_Route(rt_entry);
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* Remove any references to this route that may be cached
             * by RAW sockets.
             */
            IPRaw4_Free_Cached_Route(rt_entry);
#endif

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
            /* If the reference count is not zero and the route is being held
             * by the forwarding module, free the route, remove the link to
             * the routing module, and check the reference count again.
             */
            if (rt_entry->rt_entry_parms.rt_parm_refcnt != 0)
            {
                /* Routes cached by the forwarding module should be deleted by
                 * a call to delete the route, because it is unknown when they
                 * will get freed.
                 */
                dv_ptr = DEV_Table.dv_head;

                /* Traverse the routing table, checking the cached route of
                 * each interface.
                 */
                while (dv_ptr)
                {
                    /* If this interface is holding a pointer to the cached
                     * route, free it.
                     */
                    if ( (dv_ptr->dev_forward_rt.rt_route) &&
                         (dv_ptr->dev_forward_rt.rt_route->rt_route_node ==
                          rt_entry->rt_route_node) )
                    {
                        /* Decrement the reference count.  Don't call
                         * RTAB_Free, because the routine could inadvertently
                         * delete the route.
                         */
                        rt_entry->rt_entry_parms.rt_parm_refcnt --;

                        /* Set the route to NULL so the forwarding module
                         * does not attempt to use it again.
                         */
                        dv_ptr->dev_forward_rt.rt_route = NU_NULL;
                    }

                    dv_ptr = dv_ptr->dev_next;
                }
            }
#endif
        }
#endif

        /* If a route was found */
        if (rt)
        {
#if ( (INCLUDE_UDP == NU_FALSE) && (INCLUDE_TCP == NU_FALSE) && \
      (INCLUDE_IP_RAW == NU_FALSE) && (INCLUDE_IP_FORWARDING == NU_FALSE) )

            /* The reference count was incremented by RTAB4_Find_Route.
             * We are not using the route so decrement the reference count.
             */
            rt->rt_entry_parms.rt_parm_refcnt--;
#endif

            /* If a next-hop was not provided, delete the entire route
             * node.
             */
            if (next_hop == NU_NULL)
            {
                /* Delete the node. */
                ret_status = RTAB_Delete_Node(rt->rt_route_node,
                                              &RTAB4_Parms);
            }

            /* Otherwise, delete only the route associated with
             * the gateway.
             */
            else
            {
                /* Delete the route */
                ret_status = RTAB4_Delete_Route_From_Node(rt->rt_route_node,
                                                          IP_ADDR(next_hop));
            }
        }

        /* Else if the route to delete is the default route */
        else if ( (ro.sck_addr == 0) && (RTAB4_Default_Route) )
            ret_status = RTAB4_Delete_Node(RTAB4_Default_Route);

        else
            ret_status = NU_INVALID_ADDRESS;
    }
    else
        ret_status = NU_INVALID_PARM;

    /* Trace log */
    T_DEL_ROUTE((UINT8*)ip_dest, (UINT8*)next_hop, ret_status, 4);
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* RTAB4_Delete_Route */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Delete_Route_From_Node
*
*   DESCRIPTION
*
*       This function deletes a route associated with a node.
*
*   INPUTS
*
*       *rt_node                A pointer to the ROUTE_NODE which contains
*                               the route.
*       *next_hop               A pointer to the next-hop of the target
*                               route.
*
*   OUTPUTS
*
*       NU_SUCCESS              The route was successfully deleted
*       NU_NOT_FOUND            The route does not exist.
*
*************************************************************************/
STATIC STATUS RTAB4_Delete_Route_From_Node(ROUTE_NODE *rt_node,
                                           UINT32 next_hop)
{
    RTAB4_ROUTE_ENTRY   *rt_entry;
    STATUS              status;

    /* Find the route entry associated with the gateway */
    rt_entry = rt_node->rt_list_head;

    do
    {
        if (rt_entry->rt_gateway_v4.sck_addr == next_hop)
            break;

        rt_entry = rt_entry->rt_entry_next;

    } while (rt_entry);

    /* If a route entry was found, delete it */
    if (rt_entry)
        status = RTAB_Delete_Route_Entry(rt_node,
                                         (ROUTE_ENTRY*)rt_entry,
                                         &RTAB4_Parms);

    /* Otherwise, the route does not exist */
    else
        status = NU_NOT_FOUND;

    return (status);

} /* RTAB4_Delete_Route_From_Node */

#endif
