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
*       rtab6_dr.c                                   
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for accessing the IPv6 routing     
*       table.                                                           
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Delete_Route
*       RTAB6_Delete_Route_From_Node
*       RTAB6_Unlink_Next_Hop
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       nc6.h
*       udp6.h
*       tcp6.h
*       ipraw6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nc6.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_UDP == NU_TRUE)
#include "networking/udp6.h"
#endif

#if (INCLUDE_TCP == NU_TRUE)
#include "networking/tcp6.h"
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
#include "networking/ipraw6.h"
#endif

extern ROUTE_NODE   *RTAB6_Default_Route;

extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB6_Delete_Route
*
*   DESCRIPTION
*
*       Removes a route from the routing table. 
*
*   INPUTS
*
*       *ip_dest                A pointer to the route you want to delete
*       *next_hop               The next-hop associated with the route to
*                               delete.  If unspecified, the entire route
*                               node is deleted.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_INVALID_ADDRESS      The route was not found
*       NU_INVALID_PARM         A pointer parameter is NU_NULL.
*
*************************************************************************/
STATUS RTAB6_Delete_Route(const UINT8 *ip_dest, const UINT8 *next_hop)
{
    STATUS              ret_status;
    RTAB6_ROUTE_ENTRY   *rt;
    UINT8               default_route[] =
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )
    RTAB6_ROUTE_ENTRY   *rt_entry;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Verify pointer parameter is non-null. */   
    if (ip_dest)
    {   
        /* Find the node that contains this route and remove it. */
        rt = RTAB6_Find_Route(ip_dest, RT_HOST_MATCH | RT_OVERRIDE_METRIC |
                              RT_OVERRIDE_RT_STATE | RT_OVERRIDE_DV_STATE);

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )

        /* A route was found. */
        if (rt)
        {
            rt_entry = rt;

            /* The reference count was incremented by RTAB6_Find_Route. 
             * We are not using the route so decrement the reference count. 
             */
            rt->rt_entry_parms.rt_parm_refcnt--;
        }

        /* This is the default route. */
        else if ((RTAB6_Default_Route != NU_NULL) && (memcmp(ip_dest, default_route, IP6_ADDR_LEN) == 0))
            rt_entry = RTAB6_Default_Route->rt_list_head;

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
            UDP6_Free_Cached_Route(rt_entry);
#endif

#if (INCLUDE_TCP == NU_TRUE)

            /* Remove any references to this route that may be cached
             * by TCP sockets.
             */
            TCP6_Free_Cached_Route(rt_entry);
#endif
      
#if (INCLUDE_IP_RAW == NU_TRUE)
  
            /* Remove any references to this route that may be cached
             * by RAW sockets.
             */
            IPRaw6_Free_Cached_Route(rt_entry);
#endif
        }
#endif

        /* Make sure a route was found. */
        if (rt)
        {
#if ( (INCLUDE_UDP == NU_FALSE) && (INCLUDE_TCP == NU_FALSE) && \
      (INCLUDE_IP_RAW == NU_FALSE) )

            /* The reference count was incremented by RTAB6_Find_Route. 
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
                                              &RTAB6_Parms);
            }

            /* Otherwise, delete only the route associated with
             * the gateway.
             */
            else
            {
                /* Delete the route */
                ret_status = RTAB6_Delete_Route_From_Node(rt->rt_route_node, 
                                                          next_hop);
            }
        }

        /* Else if the default route is being deleted */
        else if ( (RTAB6_Default_Route) &&
                  (memcmp(ip_dest, default_route, IP6_ADDR_LEN) == 0) )
            ret_status = RTAB6_Delete_Node(RTAB6_Default_Route);

        else
            ret_status = NU_INVALID_ADDRESS;
    }
    else
        ret_status = NU_INVALID_PARM;

    /* Trace log */
    T_DEL_ROUTE6((UINT8*)ip_dest, (UINT8*)next_hop, ret_status, 16);
    
    /* Switch back to user mode. */
    NU_USER_MODE();
    
    return (ret_status);
     
} /* RTAB6_Delete_Route */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB6_Delete_Route_From_Node
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
*       NU_INVALID_ADDRESS      The route does not exist.
*
*************************************************************************/
STATUS RTAB6_Delete_Route_From_Node(ROUTE_NODE *rt_node, const UINT8 *next_hop)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;
    STATUS              status;

    /* Find the route entry associated with the gateway */
    rt_entry = rt_node->rt_list_head;

    do
    {
        if (IP6_ADDRS_EQUAL(rt_entry->rt_next_hop.sck_addr, next_hop))
            break;

        rt_entry = rt_entry->rt_entry_next;

    } while (rt_entry);

    /* If a route entry was found, delete it */
    if (rt_entry)
        status = RTAB_Delete_Route_Entry(rt_node, 
                                         (ROUTE_ENTRY*)rt_entry, 
                                         &RTAB6_Parms);

    /* Otherwise, the route does not exist */
    else
        status = NU_INVALID_ADDRESS;

    return (status);

} /* RTAB6_Delete_Route_From_Node */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB6_Unlink_Next_Hop
*
*   DESCRIPTION
*
*       This function removes the link between the route entry passed in
*       and the Neighbor Cache entry associated with the next-hop of
*       the route entry.
*
*   INPUTS
*
*       *rt_entry               A pointer to the route entry.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID RTAB6_Unlink_Next_Hop(const RTAB6_ROUTE_ENTRY *rt_entry)
{
    /* Delete the route from the list of routes for the Neighbor 
     * Cache entry.
     */
    if (rt_entry->rt_next_hop_entry)
        RTAB6_Delete_DestList_Entry(&rt_entry->rt_next_hop_entry->
                                    ip6_neigh_cache_dest_list, 
                                    rt_entry);

} /* RTAB6_Unlink_Next_Hop */
