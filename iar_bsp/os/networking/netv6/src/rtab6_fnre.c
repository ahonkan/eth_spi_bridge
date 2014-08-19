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
*       rtab6_fnre.c                                 
*                                                                               
*   COMPONENT                                                             
*                                                                       
*       IPv6 Routing                                                          
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for finding the next route
*       in lexicographical order.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Find_Next_Route_Entry
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       RTAB6_Find_Next_Route_Entry
*
*   DESCRIPTION
*
*       This function finds the route in the routing tree proceeding the
*       provided route.
*
*   INPUTS
*
*       *current_route          A pointer to the route of which the next 
*                               route is desired.
*
*   OUTPUTS
*
*       ROUTE_ENTRY*            A pointer to the next route.
*       NU_NULL                 There is no next route.
*
*************************************************************************/
ROUTE_ENTRY *RTAB6_Find_Next_Route_Entry(ROUTE_ENTRY *current_route)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;
    UINT8               target_address[IP6_ADDR_LEN];

    /* First, check if there is another route to the same destination. */
    if ( (current_route) && 
         (((RTAB6_ROUTE_ENTRY*)current_route)->rt_entry_next) )
        rt_entry = ((RTAB6_ROUTE_ENTRY*)current_route)->rt_entry_next;

    /* Otherwise, find the next route in the tree. */
    else
    {
        /* Get the IP address of the current route */
        if (current_route)
            memcpy(target_address, current_route->rt_route_node->rt_ip_addr,
                   IP6_ADDR_LEN);
        else
            memset(target_address, 0, IP6_ADDR_LEN);

        /* Find the next route in the routing tree. */
        rt_entry = RTAB6_Find_Next_Route(target_address);
    }

    return ((ROUTE_ENTRY*)rt_entry);

} /* RTAB6_Find_Next_Route_Entry */

#endif
