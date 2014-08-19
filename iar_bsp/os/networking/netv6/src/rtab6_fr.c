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
*       rtab6_fr.c                                   
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
*       RTAB6_Find_Route
*       RTAB6_Determine_Matching_Prefix
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

extern ROUTE_NODE   *RTAB6_Default_Route;

extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Find_Route                                                 
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function finds the route associated with the target, a 
*       network route associated with the target or the Default Route.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *ip_addr                A pointer to the IP address to which to
*                               find a route.
*       flags                   Flags associated with the type of route
*                               to return.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       *RTAB6_ROUTE_ENTRY      A pointer to the preferred route
*       NU_NULL                 No route exists
*                                                                       
*************************************************************************/
RTAB6_ROUTE_ENTRY *RTAB6_Find_Route(const UINT8 *ip_addr, INT32 flags)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;

    rt_entry = (RTAB6_ROUTE_ENTRY*)RTAB_Find_Route_Entry(ip_addr, 
                                                         &RTAB6_Parms,
                                                         flags);

    /* If a route could not be found and there is a default route, return
     * the default route.
     */
    if ( (!(flags & RT_HOST_MATCH)) && (rt_entry == NU_NULL) && 
         (RTAB6_Default_Route) && 
         (((RTAB6_ROUTE_ENTRY*)RTAB6_Default_Route->rt_list_head)->
          rt_entry_parms.rt_parm_flags & RT_UP) )
    {
        rt_entry = RTAB6_Default_Route->rt_list_head;

        rt_entry->rt_entry_parms.rt_parm_refcnt ++;
    }

    return (rt_entry);

} /* RTAB6_Find_Route */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Determine_Matching_Prefix                                                 
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function determines whether the IP address target_address
*       is covered by the prefix of the network_address.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *target_address         A pointer to the IP address.
*       *network_address        A pointer to the network IP address.
*       *subnet_mask            Unused.
*       prefix_len              The prefix length of the network IP 
*                               address.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_TRUE                 The address is covered by the prefix.
*       NU_FALSE                The address is not covered by the prefix.
*                                                                       
*************************************************************************/
UINT8 RTAB6_Determine_Matching_Prefix(const UINT8 *target_address, 
                                      const UINT8 *network_address,
                                      const UINT8 *subnet_mask, UINT8 prefix_len)
{
    UINT8   current_length = 0;
    UINT8   byte = 0;
    UINT8   bit_index = 7;

    UNUSED_PARAMETER(subnet_mask);

    /* Do a bit by bit comparison of the two IP addresses until all
     * prefix_len bits have been checked or a non-matching bit is
     * found.
     */
    while (current_length < prefix_len)
    {
        /* If the two bits do not match, break out of the loop */
        if ( ((1 << bit_index) & (target_address[byte])) !=
             ((1 << bit_index) & (network_address[byte])) )
            break;

        current_length ++;

        /* If all of the bits in this byte have been checked, get the
         * next byte and start over at bit 7.
         */
        if (bit_index == 0)
        {
            bit_index = 7;
            byte++;
        }

        /* Look at the next bit in the byte */
        else
            bit_index --;
    }

    /* If the bits match */
    if (current_length >= prefix_len)
        return (NU_TRUE);

    /* The bits do not match */
    else 
        return (NU_FALSE);

} /* RTAB6_Determine_Matching_Prefix */
