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
*       sck_gdr.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions for returning the Default
*       Route.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Get_Default_Route
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
*       NU_Get_Default_Route
*
*   DESCRIPTION
*
*       Get the default route for the system.
*
*   INPUTS
*
*       family                  The family type of the default route to
*                               return:
*                                   NU_FAMILY_IP - Return the IPv4
*                                                  default route.
*                                   NU_FAMILY_IP6 - Return the IPv6
*                                                   default route.
*
*   OUTPUTS
*
*       *ROUTE_NODE             A pointer to the Default Route
*       NU_NULL                 No default route exists
*
*************************************************************************/
ROUTE_NODE *NU_Get_Default_Route(INT16 family)
{
    STATUS      status;
    ROUTE_NODE  *default_route;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (NU_NULL);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    if (family == NU_FAMILY_IP)
        default_route = RTAB4_Get_Default_Route();
#if (INCLUDE_IPV6 == NU_TRUE)
    else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    if (family == NU_FAMILY_IP6)
        default_route = RTAB6_Get_Default_Route();
#endif
    else
        default_route = NU_NULL;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (default_route);

} /* NU_Get_Default_Route */
