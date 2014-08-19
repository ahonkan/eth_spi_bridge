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
*       rtab4_fr.c
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
*       RTAB4_Find_Route
*       RTAB4_Determine_Matching_Prefix
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

extern ROUTE_NODE   *RTAB4_Default_Route;

extern RTAB_ROUTE_PARMS RTAB4_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Find_Route
*
*   DESCRIPTION
*
*       This function finds the route associated with the target, a
*       network route associated with the target or the Default Route.
*
*   INPUTS
*
*       *de                     Pointer to the socket address information
*       flags                   Flags indicating what routes to return.
*
*   OUTPUTS
*
*       *RTAB4_ROUTE_ENTRY      A pointer to the preferred route
*       NU_NULL                 No route exists
*
*************************************************************************/
RTAB4_ROUTE_ENTRY *RTAB4_Find_Route(const SCK_SOCKADDR_IP *de, INT32 flags)
{
    UINT8               target_address[IP_ADDR_LEN];
    RTAB4_ROUTE_ENTRY   *rt_entry;

    PUT32(target_address, 0, de->sck_addr);

    rt_entry =
        (RTAB4_ROUTE_ENTRY*)RTAB_Find_Route_Entry(target_address,
                                                  &RTAB4_Parms, flags);

    /* If a route could not be found and there is a default route, return
     * the default route.
     */
    if ( (!(flags & RT_HOST_MATCH)) && (rt_entry == NU_NULL) &&
         (RTAB4_Default_Route) )
    {
        rt_entry = RTAB4_Default_Route->rt_list_head;

        rt_entry->rt_entry_parms.rt_parm_refcnt ++;
    }

    return (rt_entry);

} /* RTAB4_Find_Route */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Determine_Matching_Prefix
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
*       *subnet_mask            A pointer to the subnet mask of the
*                               network address.
*       prefix_len              Unused.
*
*   OUTPUTS
*
*       NU_TRUE                 The address is covered by the prefix.
*       NU_FALSE                The address is not covered by the prefix.
*
*************************************************************************/
UINT8 RTAB4_Determine_Matching_Prefix(const UINT8 *target_address,
                                      const UINT8 *network_address,
                                      const UINT8 *subnet_mask, UINT8 prefix_len)
{
    UNUSED_PARAMETER(prefix_len);

    if ( (IP_ADDR((UINT8*)subnet_mask) & IP_ADDR(target_address)) ==
         (IP_ADDR(network_address) & IP_ADDR((UINT8*)subnet_mask)) )
        return (NU_TRUE);
    else
        return (NU_FALSE);

} /* RTAB4_Determine_Matching_Prefix */

#endif
