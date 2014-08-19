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
* FILE NAME
*
*       sck_ur.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Update_Route.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Update_Route
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Update_Route
*
*   DESCRIPTION
*
*       This function updates the parameters of the route associated with
*       the IP address.
*
*   INPUTS
*
*       *ip_address             The destination IP address of the route
*                               to update.
*       *gateway                A pointer to the gateway associated with
*                               the route.
*       *new_route              A pointer to the data structure holding
*                               the updated parameters.
*       family                  The family type of the route to update.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVAL                One of the parameters is invalid
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATUS NU_Update_Route(const UINT8 *ip_address, const UINT8 *gateway,
                       UPDATED_ROUTE_NODE *new_route, INT16 family)
{
    STATUS      status;
    NU_SUPERV_USER_VARIABLES

    new_route->urt_flags |= (INT32)RT_LOCAL;

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return(status);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    if (family == NU_FAMILY_IP)
        status = RTAB4_Update_Route(ip_address, gateway, new_route);
    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    if (family == NU_FAMILY_IP6)
        status = RTAB6_Update_Route(ip_address, gateway, new_route);
    else
#endif
        status = NU_INVAL;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_USER_MODE();

    return (status);

} /* NU_Update_Route */
