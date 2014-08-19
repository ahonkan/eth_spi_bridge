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
*       rtab4_frbg.c
*
*   DESCRIPTION
*
*       This file contains the implementation of the function
*       RTAB4_Find_Route_By_Gateway.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       RTAB4_Find_Route_By_Gateway
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       RTAB4_Find_Route_By_Gateway
*
*   DESCRIPTION
*
*       Find a route through a specific gateway.
*
*   INPUTS
*
*       *ip_addr                The destination IP address of the route
*                               to find.
*       *gw_addr                The IP address of the gateway of the
*                               target route to find.
*       flags                   Flags indicating what routes to return
*
*   OUTPUTS
*
*       A pointer to the ROUTE_ENTRY upon success.
*       NU_NULL if the route does not exist.
*
*************************************************************************/
ROUTE_ENTRY *RTAB4_Find_Route_By_Gateway(const UINT8 *ip_dest,
                                         const UINT8 *gw_addr, INT32 flags)
{
    RTAB4_ROUTE_ENTRY   *target_route;
    UINT32              gateway_addr;
    SCK_SOCKADDR_IP     de;

    de.sck_family = SK_FAM_IP;
    de.sck_len = sizeof(SCK_SOCKADDR_IP);
    de.sck_addr = IP_ADDR(ip_dest);

    /* Find the preferred route to the destination */
    target_route = RTAB4_Find_Route(&de, flags);

    /* If a route was found */
    if (target_route)
    {
        gateway_addr = IP_ADDR(gw_addr);

        /* Check that the preferred route is not the target route */
        if (target_route->rt_gateway_v4.sck_addr != gateway_addr)
        {
            /* Decrement the reference count of the route found.  It
             * is not the route that will be returned.
             */
            target_route->rt_entry_parms.rt_parm_refcnt --;

            /* Get a pointer to the first route in the list of routes */
            target_route = target_route->rt_route_node->rt_list_head;

            /* Traverse the routes for the target route */
            while (target_route)
            {
                if (target_route->rt_gateway_v4.sck_addr == gateway_addr)
                {
                    /* Increment the reference count.  This is the route
                     * that will be returned.
                     */
                    target_route->rt_entry_parms.rt_parm_refcnt ++;

                    break;
                }

                target_route = target_route->rt_entry_next;
            }
        }
    }

    return ((ROUTE_ENTRY*)target_route);

} /* RTAB4_Find_Route_By_Gateway */

#endif
