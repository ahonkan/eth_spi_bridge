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
*       sck_frbg.c
*
*   DESCRIPTION
*
*       This file contains the implementation of the function
*       NU_Find_Route_By_Gateway.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Find_Route_By_Gateway
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_Find_Route_By_Gateway
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
*       family                  The family type of the route.
*       flags                   Flags to apply to the route search.
*
*   OUTPUTS
*
*       A pointer to the ROUTE_ENTRY upon success.
*       NU_NULL if the route does not exist.
*
*************************************************************************/
ROUTE_ENTRY *NU_Find_Route_By_Gateway(const UINT8 *ip_dest,
                                      const UINT8 *gw_addr, INT16 family,
                                      INT32 flags)
{
    ROUTE_ENTRY     *target_route;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Are the parameters valid? */
    if ( (!ip_dest) || (!gw_addr) || (
#if (INCLUDE_IPV4 == NU_TRUE)
        (family != NU_FAMILY_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
        &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
         (family != NU_FAMILY_IP6)
#endif
       ))
        return (NU_NULL);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (NU_NULL);
    }

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        target_route = RTAB6_Find_Route_By_Gateway(ip_dest, gw_addr, flags);
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        target_route = RTAB4_Find_Route_By_Gateway(ip_dest, gw_addr, flags);
#endif

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (target_route);

} /* NU_Find_Route_By_Gateway */
