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
*       rtab6_frbg.c                                 
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the function RTAB6_Find_Route_By_Gateway.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Find_Route_By_Gateway
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
*       RTAB6_Find_Route_By_Gateway
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function finds the route associated with the target, a 
*       network route associated with the target or the Default Route.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *ip_addr                The destination IP address of the route
*                               to find.
*       *gateway                The IP address of the gateway of the
*                               target route to find.
*       flags                   Flags to apply to the route search.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       A pointer to the ROUTE_ENTRY upon success.
*       NU_NULL if the route does not exist.
*                                                                       
*************************************************************************/
ROUTE_ENTRY *RTAB6_Find_Route_By_Gateway(const UINT8 *ip_addr, const UINT8 *gateway, 
                                         INT32 flags)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Find a route to the destination */
    rt_entry = RTAB6_Find_Route(ip_addr, flags);

    /* If a route was found */
    if (rt_entry)
    {
        /* If the gateway of this route does not match the target
         * gateway, find the matching route.
         */
        if (memcmp(rt_entry->rt_next_hop.sck_addr, gateway, IP6_ADDR_LEN) != 0)
        {
            /* Decrement the reference count.  This is not the route that
             * will be returned.
             */
            rt_entry->rt_entry_parms.rt_parm_refcnt --;

            /* Start at the beginning of the route list */
            rt_entry = rt_entry->rt_route_node->rt_list_head;

            while (rt_entry)
            {
                /* If the gateway of this route matches the target, this
                 * is the target route.
                 */
                if (memcmp(rt_entry->rt_next_hop.sck_addr, gateway, IP6_ADDR_LEN) == 0)
                {
                    /* Increment the reference count.  This is the route
                     * that will be returned.
                     */
                    rt_entry->rt_entry_parms.rt_parm_refcnt ++;

                    break;
                }

                /* Get a pointer to the next route entry */
                rt_entry = rt_entry->rt_entry_next;
            }
        }
    }

    return ((ROUTE_ENTRY*)rt_entry);

} /* RTAB6_Find_Route_By_Gateway */
