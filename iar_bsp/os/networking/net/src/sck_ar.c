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
*       sck_ar.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Add_Route.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Add_Route
*
* DEPENDENCIES
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
*       NU_Add_Route
*
*   DESCRIPTION
*
*       Add a route to the routing table.
*
*   INPUTS
*
*       *ip_dest                The destination IP address for the route
*                               to be added.  There are three possible
*                               cases.
*
*                               1)  A host IP address - 192.200.100.13
*                               2)  A network address - 192.200.100.0
*                               3)  A NULL address for adding a default
*                                   gateway.
*       *mask                   A mask to be used with this route.
*       *gw_addr                The IP address of the gateway or next hop
*
*   OUTPUTS
*
*       NU_SUCCESS              The route was successfully added.
*       NU_INVALID_PARM         One of the parameters is an invalid value.
*       NU_MEM_ALLOC            Insufficient memory to add the route.
*
*************************************************************************/
STATUS NU_Add_Route(const UINT8 *ip_dest, const UINT8 *mask,
                    const UINT8 *gw_addr)
{
    DV_DEVICE_ENTRY             *dev;
    RTAB_ROUTE                  route;
    SCK_SOCKADDR_IP             *dest;
    UINT32                      flags;
    UINT32                      dest_addr;
    STATUS                      status;
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Are the parameters valid? */
    if ((!ip_dest) || (!mask) || (!gw_addr))
        return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    dest_addr = IP_ADDR(ip_dest);

    UTL_Zero(&route, sizeof (route));

    /* Point to the destination. */
    dest = (SCK_SOCKADDR_IP *) &route.rt_ip_dest;
    dest->sck_family = SK_FAM_IP;
    dest->sck_len = sizeof (*dest);
    dest->sck_addr = IP_ADDR (gw_addr);

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* Find a route to the gateway/next hop. In order for this to be a valid
     * route the gateway should be directly connected and thus we should be
     * able to find a route to it.
     */
    IP_Find_Route(&route);

    /* If a route to the gw_addr cannot be found then the user is attempting to
     * add an invalid route.
     */
    if (route.rt_route == NU_NULL)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }

    /* Free the route that was just discovered since we were really only
     * interested in the device that is associated with the route.
     */
    RTAB_Free((ROUTE_ENTRY*)route.rt_route, NU_FAMILY_IP);

    flags = RT_UP | RT_STATIC;

    /* If this is a directly connected device then do not set the gateway
     * flag as this route is not to a gateway.
     */
    dev = DEV_Get_Dev_By_Addr(gw_addr);

    if (dev == NU_NULL)
    {
        dev = route.rt_route->rt_entry_parms.rt_parm_device;

        flags |= RT_GATEWAY;
    }

    /* Add the new route. If the destination IP address for the route is 0, then
     * a default route is being added.
     */
    if (dest_addr == 0)
    {
        status = RTAB4_Set_Default_Route(dev, IP_ADDR(gw_addr), flags);
    }
    else
    {
        if (IP_ADDR(mask) == 0xFFFFFFFFUL)
            flags |= RT_HOST;

        flags |= RT_LOCAL;

        status = RTAB4_Add_Route(dev, dest_addr, IP_ADDR(mask),
                                 IP_ADDR(gw_addr), flags);
    }

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Add_Route */

#endif
