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
*       sck_fnr.c
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
*       NU_Find_Next_Route
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
*       NU_Find_Next_Route
*
*   DESCRIPTION
*
*       This function finds the route in the routing tree proceeding the
*       provided route.
*
*   INPUTS
*
*       *current_address        A pointer to the address of the route
*                               of which the next route is desired
*       family                  The family type of the address passed in.
*
*   OUTPUTS
*
*       ROUTE_NODE*             A pointer to the route.
*       NU_NULL                 There is no next route.
*
*************************************************************************/
ROUTE_ENTRY *NU_Find_Next_Route(const UINT8 *current_address, INT16 family)
{
    ROUTE_ENTRY   *rt_entry = NU_NULL, *next_entry;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(family);
#endif

    /* Grab the TCP semaphore */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
#if (INCLUDE_IPV6 == NU_TRUE)
        if (family == NU_FAMILY_IP6)
            rt_entry = (ROUTE_ENTRY*)RTAB6_Find_Next_Route(current_address);
#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            rt_entry = (ROUTE_ENTRY*)RTAB4_Find_Next_Route(current_address);
#endif

        /* If a route was found, and there are multiple routes to the same
         * destination, return the route with the lowest metric.
         */
        if ( (rt_entry) && (rt_entry->rt_flink) )
        {
            /* Save a pointer to the next entry in the list */
            next_entry = rt_entry->rt_flink;

            do
            {
                /* If the next entry has a lower metric than the current
                 * entry, save this entry.
                 */
                if (next_entry->rt_metric < rt_entry->rt_metric)
                    rt_entry = next_entry;

                next_entry = next_entry->rt_flink;

            } while (next_entry);
        }

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    NU_USER_MODE();

    return (rt_entry);

} /* NU_Find_Next_Route */
