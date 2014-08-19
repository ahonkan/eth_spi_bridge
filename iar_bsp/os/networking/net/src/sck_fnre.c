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
*       sck_fnre.c
*
*   COMPONENT
*
*       Routing
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
*       NU_Find_Next_Route_Entry
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
*       NU_Find_Next_Route_Entry
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
*       family                  The family type of the route passed in.
*
*   OUTPUTS
*
*       ROUTE_ENTRY*            A pointer to the next route.
*       NU_NULL                 There is no next route.
*
*************************************************************************/
ROUTE_ENTRY *NU_Find_Next_Route_Entry(ROUTE_ENTRY *current_route,
                                      INT16 family)
{
    ROUTE_ENTRY     *next_route = NU_NULL;

#if (INCLUDE_IPV6 == NU_TRUE)
#ifndef IPV6_VERSION_COMP
    UINT8       current_address[MAX_ADDRESS_SIZE];
#endif
#endif

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Grab the TCP semaphore */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
#if (INCLUDE_IPV6 == NU_TRUE)
        if (family == NU_FAMILY_IP6)
        {
#ifdef IPV6_VERSION_COMP
            next_route =
                (ROUTE_ENTRY*)RTAB6_Find_Next_Route_Entry(current_route);
#else
            if (current_route)
            {
                memcpy(current_address,
                       current_route->rt_route_node->rt_ip_addr,
                       IP6_ADDR_LEN);
            }

            else
                memset(current_address, 0, IP6_ADDR_LEN);

            next_route = (ROUTE_ENTRY*)RTAB6_Find_Next_Route(current_address);
#endif
        }
#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        if (family == NU_FAMILY_IP)
        {
            next_route =
                (ROUTE_ENTRY*)RTAB4_Find_Next_Route_Entry(current_route);
        }
#endif

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    NU_USER_MODE();

    return (next_route);

} /* NU_Find_Next_Route */
