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
*        mib2_ip.c
*
* COMPONENT
*
*        MIB II - IP Group.
*
* DESCRIPTION
*
*        This file contains the functions that responsible for providing
*        statistics of IP Group.
*
* DATA STRUCTURES
*
*        Mib2_Ip_Data
*
* FUNCTIONS
*
*        MIB2_Get_IpAdEntAddr
*        MIB2_Get_IpAdEntNetMask
*        MIB2_Get_IpAdEntIfIndex
*        MIB2_IpRoute_Get
*        MIB2_Get_IpRouteAge
*        MIB2_Set_IpRouteAge
*        MIB2_Get_IpRouteDest
*        MIB2_Set_IpRouteDest
*        MIB2_Get_IpRouteIfIndex
*        MIB2_Set_IpRouteIfIndex
*        MIB2_Get_IpRouteMetric1
*        MIB2_Set_IpRouteMetric1
*        MIB2_Get_IpRouteNextHop
*        MIB2_Set_IpRouteNextHop
*        MIB2_Get_IpRouteType
*        MIB2_Set_IpRouteType
*        MIB2_Get_IpRouteProto
*        MIB2_Get_IpRouteMask
*        MIB2_Set_IpRouteMask
*        MIB2_Get_IpNetToMediaIfIndex
*        MIB2_Get_IpNetToMediaIfIndex_If
*        MIB2_Set_IpNetToMediaIfIndex
*        MIB2_Get_IpNetToMediaPhysAddress
*        MIB2_Get_IpNetToMediaPhysAddress_If
*        MIB2_Set_IpNetToMediaPhysAddress
*        MIB2_Get_IpNetToMediaNetAddress
*        MIB2_Get_IpNetToMediaNetAddress_If
*        MIB2_Set_IpNetToMediaNetAddress
*        MIB2_Get_IpNetToMediaType
*        MIB2_Get_IpNetToMediaType_If
*        MIB2_Set_IpNetToMediaType
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp.h"
#endif

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

#if (MIB2_IP_INCLUDE == NU_TRUE)

MIB2_IP_STRUCT               Mib2_Ip_Data;

/* IP Group. */
STATIC  ROUTE_ENTRY             *MIB2_IpRoute_Get(UINT8 *, UINT8);

#endif /* (MIB2_IP_INCLUDE == NU_TRUE) */

#if ( (MIB2_IP_INCLUDE == NU_TRUE) || (MIB2_AT_INCLUDE == NU_TRUE) )

/*-----------------------------------------------------------------------
 * IP Group
 *----------------------------------------------------------------------*/

/*
 * IP Address Table
 */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpAdEntAddr
*
* DESCRIPTION
*
*       This function returns the device with the given IP address.
*
* INPUTS
*
*       *addr                   The IP address of the device.
*       *dev_addr               The IP address which is found.
*       getflag                 NU_TRUE if an exact match is required
*                               or NU_FALSE if the next element is
*                               required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpAdEntAddr(UINT8 *addr, UINT8 *dev_addr, UINT8 getflag)
{
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;
    INT16               status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device. */
        if (getflag != NU_FALSE)
        {
            if (DEV_Get_Dev_By_Addr(addr))
                memcpy(dev_addr, addr, IP_ADDR_LEN);

            /* Otherwise, return an error */
            else
                status = MIB2_UNSUCCESSFUL;
        }

        else
        {
            /* Get a pointer to the next device's IP address */
            dev_addr_entry = DEV_Get_Next_By_Addr(addr);

            /* If an entry was returned */
            if (dev_addr_entry)
                PUT32(dev_addr, 0, dev_addr_entry->dev_entry_ip_addr);

            /* Otherwise, return an error */
            else
                status = MIB2_UNSUCCESSFUL;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the status */
    return (status);

} /* MIB2_Get_IpAdEntAddr */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpAdEntNetMask
*
* DESCRIPTION
*
*       This function returns the device's subnet mask associated with
*       the IP address.
*
* INPUTS
*
*       *addr                   The IP address of the device.
*       *net_mask               The location where the subnet mask is
*                               to be placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpAdEntNetMask(UINT8 *addr, UINT8 *net_mask, UINT8 getflag)
{
    DV_DEVICE_ENTRY     *dev;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry = NU_NULL;
    INT16               status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device. */
        if (getflag != NU_FALSE)
        {
            /* Get a pointer to the device */
            dev = DEV_Get_Dev_By_Addr(addr);

            /* If the device was found, get a pointer to the address
             * structure on the device.
             */
            if (dev)
                dev_addr_entry = DEV_Find_Target_Address(dev, IP_ADDR(addr));
        }

        /* Get a pointer to the next device's IP address */
        else
            dev_addr_entry = DEV_Get_Next_By_Addr(addr);

        /* If device is found, get the subnet mask. */
        if (dev_addr_entry != NU_NULL)
        {
            PUT32(net_mask, 0, dev_addr_entry->dev_entry_netmask);
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the status */
    return (status);

} /* MIB2_Get_IpAdEntNetMask */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpAdEntIfIndex
*
* DESCRIPTION
*
*       This function returns the device's index associated with the
*       IP address.
*
* INPUTS
*
*       *addr                   The IP address of the device.
*       *dev_index              The location where the index is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpAdEntIfIndex(UINT8 *addr, UINT32 *dev_index,
                              UINT8 getflag)
{
    DV_DEVICE_ENTRY     *dev = NU_NULL;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;
    INT16               status;
    UINT8               target_addr[IP_ADDR_LEN];

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device. */
        if (getflag != NU_FALSE)
            dev = DEV_Get_Dev_By_Addr(addr);
        else
        {
            /* Get a pointer to the next device's IP address */
            dev_addr_entry = DEV_Get_Next_By_Addr(addr);

            /* Get a pointer to the device associated with the IP address */
            if (dev_addr_entry)
            {
                PUT32(target_addr, 0, dev_addr_entry->dev_entry_ip_addr);

                dev = DEV_Get_Dev_By_Addr(target_addr);
            }
        }

        /* If device is found, get the index. */
        if (dev != NU_NULL)
        {
            *dev_index = dev->dev_index;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the status */
    return (status);

} /* MIB2_Get_IpAdEntIfIndex */

/*
 * IP Route Table
 */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_IpRoute_Get
*
* DESCRIPTION
*
*       This function returns a pointer to the ROUTE_ENTRY interface
*       associated with the IP address passed to the function.
*
* INPUTS
*
*       *addr                   The destination IP of the ROUTE_ENTRY.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       ROUTE_ENTRY *          If a route node is found with the specified
*                              index, this function returns a pointer to
*                              the route node.
*       NU_NULL                If a route node is not found with the
*                              specified index,
*
*************************************************************************/
STATIC ROUTE_ENTRY *MIB2_IpRoute_Get(UINT8 *addr, UINT8 getflag)
{
    RTAB4_ROUTE_ENTRY   *rt_entry, *next_entry;
    ROUTE_NODE          *temp;
    SCK_SOCKADDR_IP     socket;

    /* Find the route. */
    if (getflag == NU_TRUE)
    {
        /* Initialize the socket. */
        socket.sck_addr = IP_ADDR(addr);

        /* Check whether a default route was requested. */
        if (socket.sck_addr == IP_ADDR_ANY)
        {
            temp = RTAB4_Get_Default_Route();

            if (temp)
                rt_entry = temp->rt_list_head;
            else
                rt_entry = NU_NULL;
        }

        /* Otherwise, find the route. */
        else
        {
            rt_entry = RTAB4_Find_Route(&socket,
                                        (RT_HOST_MATCH | RT_BEST_METRIC |
                                         RT_OVERRIDE_METRIC));

            if (rt_entry)
                RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP);
        }
    }
    else
    {
        rt_entry = RTAB4_Find_Next_Route(addr);

        if (rt_entry != NU_NULL)
        {
            /* Save a pointer to the next entry in the list */
            next_entry = rt_entry->rt_entry_next;

            /* If a route was found, and there are multiple routes to the same
             * destination, return the route with the lowest metric.
             */
            while (next_entry)
            {
                /* If the next entry has a lower metric than the current
                 * entry, save this entry.
                 */
                if (next_entry->rt_metric < rt_entry->rt_metric)
                    rt_entry = next_entry;

                next_entry = next_entry->rt_entry_next;
            }

            /* Fill addr with the address found. */
            memcpy(addr, rt_entry->rt_route_node->rt_ip_addr, IP_ADDR_LEN);
        }
    }

    /* Return a pointer to the route */
    return ((ROUTE_ENTRY*)rt_entry);

} /* MIB2_IpRoute_Get */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteAge
*
* DESCRIPTION
*
*       This function returns the route's number of seconds since this
*       route was last updated or otherwise determined to be correct.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *age                    The location where the age is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteAge(UINT8 *addr, UINT32 *age, UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the age. */
        if(route != NU_NULL)
        {
            *age = (NU_Retrieve_Clock() - route->rt_entry_parms.rt_parm_clock)
                    / TICKS_PER_SECOND;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteAge */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteAge
*
* DESCRIPTION
*
*       This function sets the number of seconds, since this route was
*       last updated or otherwise determined to be correct.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       age                     The age to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteAge(const UINT8 *addr, UINT32 age)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;

    /* Set the parameters of the updated_route data structure to
       NU_NULL. */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;

    updated_route.urt_flags =
        updated_route.urt_flags | (INT32)RT_NETMGMT;

    updated_route.urt_age = (INT32)age;

    /* Update the route */
    if (NU_Update_Route(addr, NU_NULL, &updated_route,
                        NU_FAMILY_IP) == NU_SUCCESS)
        status = NU_SUCCESS;

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteAge */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteDest
*
* DESCRIPTION
*
*       This function checks whether the route exists.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *dest                   The destination address for the route
*                               which was found.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteDest(UINT8 *addr, UINT8 *dest, UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, set the destination. */
        if (route != NU_NULL)
        {
            memcpy(dest, route->rt_route_node->rt_ip_addr, IP_ADDR_LEN);
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteDest */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteDest
*
* DESCRIPTION
*
*       This function sets a new destination address for the route.
*
* INPUTS
*
*       *addr                   The IP address of the route to be updated.
*       *addr_new               The IP address to be updated with.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteDest(const UINT8 *addr, UINT8 *addr_new)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;

    /* Set the parameters of the updated_route data structure to
       NU_NULL. */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;
    updated_route.urt_flags = updated_route.urt_flags | (INT32)RT_NETMGMT;

    updated_route.urt_dest = addr_new;

    /* Update the route */
    if (NU_Update_Route(addr, NU_NULL, &updated_route,
                        NU_FAMILY_IP) == NU_SUCCESS)
        status = NU_SUCCESS;

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;


    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteDest */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteIfIndex
*
* DESCRIPTION
*
*       This function returns the device index associated with the route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *dev_index              The location where the index is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteIfIndex(UINT8 *addr, UINT32 *dev_index,
                              UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the index. */
        if (route != NU_NULL)
        {
            *dev_index = route->rt_entry_parms.rt_parm_device->dev_index;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteIfIndex */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteIfIndex
*
* DESCRIPTION
*
*       This function sets the device index associated with the route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       dev_index               The new value of the index.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteIfIndex(const UINT8 *addr, UINT32 dev_index)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;
    CHAR                if_name[DEV_NAME_LENGTH];

    /* Set the parameters of the updated_route data structure to
       NU_NULL. */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;
    updated_route.urt_flags = updated_route.urt_flags | (INT32)RT_NETMGMT;

    updated_route.urt_dev.urt_dev_name = NU_IF_IndexToName((INT32)dev_index,
                                                           if_name);

    /* Update the route. */
    if (NU_Update_Route(addr, NU_NULL, &updated_route,
                        NU_FAMILY_IP) == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error code. */
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteIfIndex */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteMetric1
*
* DESCRIPTION
*
*       This function returns the primary routing metric for this route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *metric1                The location where the metric1 is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteMetric1(UINT8 *addr, UINT32 *metric1, UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the metric. */
        if (route != NU_NULL)
        {
            *metric1 = route->rt_entry_parms.rt_parm_metric;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteMetric1 */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteMetric1
*
* DESCRIPTION
*
*       This function sets the primary routing metric for this route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       metric1                 The value for the metric1.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteMetric1(const UINT8 *addr, UINT32 metric1)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;

    /* Set the parameters of the updated_route data structure to
       NU_NULL. */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;
    updated_route.urt_flags = updated_route.urt_flags | (INT32)RT_NETMGMT;

    updated_route.urt_metric.urt4_metric = (INT32)metric1;

    /* Update the route */
    if (NU_Update_Route(addr, NU_NULL, &updated_route,
                        NU_FAMILY_IP) == NU_SUCCESS)
        status = NU_SUCCESS;

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteMetric1 */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteNextHop
*
* DESCRIPTION
*
*       This function returns the IP address of the next hop of this route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *nexthop                The location where the nexthop is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteNextHop(UINT8 *addr, UINT8 *nexthop, UINT8 getflag)
{
    RTAB4_ROUTE_ENTRY   *route;
    INT16               status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = (RTAB4_ROUTE_ENTRY*)MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the nexthop. */
        if (route != NU_NULL)
        {
            PUT32(nexthop, 0, route->rt_gateway_v4.sck_addr);
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteNextHop */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteNextHop
*
* DESCRIPTION
*
*       This function sets the IP address of the next hop of this route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       nexthop                 The nexthop to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteNextHop(const UINT8 *addr, UINT8 *nexthop)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;

    /* Set the parameters of the updated_route data structure to
       NU_NULL. */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;
    updated_route.urt_flags = updated_route.urt_flags | (INT32)RT_NETMGMT;

    updated_route.urt_gateway = nexthop;

    /* Update the route */
    if (NU_Update_Route(addr, NU_NULL, &updated_route,
                        NU_FAMILY_IP) == NU_SUCCESS)
        status = NU_SUCCESS;

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteNextHop */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteType
*
* DESCRIPTION
*
*       This function returns the type of this route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *type                   The location where the type is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteType(UINT8 *addr, UINT32 *type, UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the type. */
        if (route != NU_NULL)
        {
            if (route->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
                *type = 4;
            else
                *type = 3;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteType */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteType
*
* DESCRIPTION
*
*       This function sets the type of this route.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       type                    The type to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteType(const UINT8 *addr, UINT32 type)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;

    /* Set the parameters of the updated_route data structure to
       NU_NULL. */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;
    updated_route.urt_flags = updated_route.urt_flags | (INT32)RT_NETMGMT;

    switch(type)
    {
        case 2:

            updated_route.urt_flags &= ~RT_UP;
            status = NU_SUCCESS;
            break;

        case 3:

            updated_route.urt_flags |= RT_NOT_GATEWAY;
            status = NU_SUCCESS;
            break;

        case 4:

            updated_route.urt_flags |= RT_GATEWAY;
            status = NU_SUCCESS;
            break;

        default:

            status = MIB2_UNSUCCESSFUL;
            break;
    }

    /* Update the route */
    if (status == NU_SUCCESS)
    {
        if (NU_Update_Route(addr, NU_NULL, &updated_route,
                            NU_FAMILY_IP) == NU_SUCCESS)
            status = NU_SUCCESS;
        else
            status = MIB2_UNSUCCESSFUL;
    }

    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteType */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteProto
*
* DESCRIPTION
*
*       This function returns the routing mechanism via which this route
*       was learned.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *proto                  The location where the protocol is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteProto(UINT8 *addr, UINT32 *proto, UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the protocol. */
        if (route != NU_NULL)
        {
            if (route->rt_entry_parms.rt_parm_flags & RT_RIP2)
                *proto = 8;
            else if (route->rt_entry_parms.rt_parm_flags & RT_ICMP)
                *proto = 4;
            else if (route->rt_entry_parms.rt_parm_flags & RT_NETMGMT)
                *proto = 3;
            else
                *proto = 2;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteProto */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpRouteMask
*
* DESCRIPTION
*
*       This function returns the mask.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *mask                   The location where the mask is to be
*                               placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpRouteMask(UINT8 *addr, UINT8 *mask, UINT8 getflag)
{
    ROUTE_ENTRY     *route;
    INT16           status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target route. */
        route = MIB2_IpRoute_Get(addr, getflag);

        /* If route is found, get the mask. */
        if (route != NU_NULL)
        {
            memcpy(mask, route->rt_route_node->rt_submask, IP_ADDR_LEN);
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IpRouteMask */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpRouteMask
*
* DESCRIPTION
*
*       This function sets the mask.
*
* INPUTS
*
*       *addr                   The IP address of the route.
*       *mask                   The mask to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpRouteMask(const UINT8 *addr, const UINT8 *mask)
{
    UPDATED_ROUTE_NODE  updated_route;
    INT16               status;

    /* Set the parameters of the updated_route data structure to NU_NULL */
    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    updated_route.urt_flags = RT_UP;
    updated_route.urt_flags = updated_route.urt_flags | (INT32)RT_NETMGMT;

    updated_route.urt_prefix_length = (INT)RTAB4_Find_Prefix_Length(mask);

    /* Update the route */
    if (NU_Update_Route(addr, NU_NULL, &updated_route,
                        NU_FAMILY_IP) == NU_SUCCESS)
        status = NU_SUCCESS;

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IpRouteMask */

/*
 * IP Net to Media
 */

#ifndef SNMP_VERSION_COMP
/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaIfIndex
*
* DESCRIPTION
*
*       This function gets the interface index. If the getflag is not
*       set, then it gets the next interfaces address.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *index                  The location where the index is to be
*                               stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaIfIndex(UINT8 *addr, INT32 *index,
                                   UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY   entry;

    /* Get the next Entry. if the flag is not set*/
    if (getflag == NU_FALSE)
    {
        /* Check if the entry was found. */
        if (ARP_Get_Next(addr, &entry) != NU_NULL)
        {
            /* Check if the entry returned is not of the same address. */
            if (IP_ADDR(addr) ==
                LONGSWAP(IP_ADDR(entry.ip_addr.ip_address)))
            {
                *index = -1;
            }
            else
            {
                /* Update the addr variable. */
                PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

                *index = ARP_Get_Index(addr);
            }
        }
        else
            *index = -1;
    }
    else
    {
        *index = ARP_Get_Index(addr);
    }

    /* Was an index found? */
    if (*index >= 0)
        status = NU_SUCCESS;

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(index);
    UNUSED_PARAMETER(getflag);

#endif /*  (INCLUDE_ARP == NU_TRUE)  */

    return (status);

} /* MIB2_Get_IpNetToMediaIfIndex */

#else

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaIfIndex_If
*
* DESCRIPTION
*
*       This function gets the interface index. If the getflag is not
*       set, then it gets the next interfaces address.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *index                  The location where the index is to be
*                               stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaIfIndex_If(UINT32 *index, UINT8 *addr,
                                      UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY   entry;
    ARP_ENTRY   *ret_entry;

    /* Get the next Entry. if the flag is not set*/
    if (getflag == NU_FALSE)
    {
        ret_entry = ARP_Get_Next_If(*index, addr, &entry);
    }
    else
    {
        ret_entry = ARP_Get_Index_If(*index, addr, &entry);
    }

    if (ret_entry != NU_NULL)
    {
        *index = ret_entry->arp_dev_index + 1;
        PUT32(addr, 0, ret_entry->ip_addr.arp_ip_addr);
        status = NU_SUCCESS;
    }
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(index);
    UNUSED_PARAMETER(getflag);

#endif /*  (INCLUDE_ARP == NU_TRUE)  */

    return (status);

} /* MIB2_Get_IpNetToMediaIfIndex_If */

#endif

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpNetToMediaIfIndex
*
* DESCRIPTION
*
*       This function sets the interface index.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       index                   The new value of the index.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpNetToMediaIfIndex(const UINT8 *addr, UINT32 index)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY       arp_entry;

    /* Set all values in the arp_entry data structure to -1 */
    memset(&arp_entry, -1, sizeof(ARP_ENTRY));

    arp_entry.arp_flags = ARP_UP;
    arp_entry.arp_flags |= (INT32)RT_NETMGMT;

    /* NET's device index array is zero based where as SNMP uses 1 based
     * indexes. So subtract one to adjust that.
     */
    arp_entry.arp_dev_index = (INT32)index-1;

    /* Copy the new IP Address into the arp_entry data structure */
    status = (INT16)NU_ARP_Update(&arp_entry, addr);

    /* If NU_ARP_Update is not successful return error code. */
    if(status != NU_SUCCESS)
    {
        status = MIB2_UNSUCCESSFUL;
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(index);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    return (status);

} /* MIB2_Set_IpNetToMediaIfIndex */

#ifndef SNMP_VERSION_COMP
/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaPhysAddress
*
* DESCRIPTION
*
*       This function gets the physical address.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *paddr                  The location where the physical address
*                               is to be stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaPhysAddress(UINT8 *addr, UINT8 *paddr,
                                       UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY             entry;
    SCK_IOCTL_OPTION      option;

    /* Get the Entry. */
    if (getflag == NU_FALSE)
    {
        /* Check if the entry was found. */
        if (ARP_Get_Next(addr, &entry) != NU_NULL)
        {
            /* Check if the entry returned is not of the same address. */
            if (IP_ADDR(addr) ==
                LONGSWAP(IP_ADDR(entry.ip_addr.ip_address)) )
            {
                status = MIB2_UNSUCCESSFUL;
            }

            else
            {
                /* Update the addr variable. */
                PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

                /* Updating MAC address variable. */
                NU_BLOCK_COPY(paddr, entry.arp_mac_addr,
                              MIB2_MAX_PADDRSIZE);

                /* Returning success code. */
                status = NU_SUCCESS;
            }
        }

        /* If we did not get next ARP entry then return error code. */
        else
            status = MIB2_UNSUCCESSFUL;
    }

    /* If this is get request. */
    else
    {
        /* Setting ARP request option. */
        NU_BLOCK_COPY(option.s_ret.arp_request.arp_pa.sck_data, addr,
                      MIB2_MAX_NETADDRSIZE);

        if (NU_Ioctl_SIOCGARP(&option) == NU_SUCCESS)
        {
            NU_BLOCK_COPY(paddr, option.s_ret.arp_request.arp_ha.sck_data,
                          MIB2_MAX_PADDRSIZE);

            status = NU_SUCCESS;
        }
        else
        {
            status = MIB2_UNSUCCESSFUL;
        }
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(paddr);
    UNUSED_PARAMETER(getflag);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    /* Returning status */
    return (status);

} /* MIB2_Get_IpNetToMediaPhysAddress */

#else

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaPhysAddress_If
*
* DESCRIPTION
*
*       This function gets the physical address.
*
* INPUTS
*
*       *index                  The interface index of the device.
*       *addr                   The IP address of the net to media table.
*       *paddr                  The location where the physical address
*                               is to be stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaPhysAddress_If(UINT32 *index, UINT8 *addr,
                                          UINT8 *paddr, UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY             entry;
    ARP_ENTRY             *ret_entry;

    /* Get the next Entry. if the flag is not set*/
    if (getflag == NU_FALSE)
    {
        ret_entry = ARP_Get_Next_If(*index, addr, &entry);
    }
    else
    {
        ret_entry = ARP_Get_Index_If(*index, addr, &entry);
    }

    if (ret_entry != NU_NULL)
    {
        *index = ret_entry->arp_dev_index + 1;

        /* Update the addr variable. */
        PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

        /* Updating MAC address variable. */
        NU_BLOCK_COPY(paddr, entry.arp_mac_addr,
                        MIB2_MAX_PADDRSIZE);

        /* Returning success code. */
        status = NU_SUCCESS;
    }
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(paddr);
    UNUSED_PARAMETER(getflag);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    /* Returning status */
    return (status);

} /* MIB2_Get_IpNetToMediaPhysAddress_If */

#endif

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpNetToMediaPhysAddress
*
* DESCRIPTION
*
*       This function sets the physical address.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *paddr                  The location where the physical address
*                               is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpNetToMediaPhysAddress(const UINT8 *addr,
                                       const UINT8 *paddr)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY   arp_entry;

    /* Set all values in the arp_entry data structure to -1 */
    memset(&arp_entry, -1, sizeof(ARP_ENTRY));

    arp_entry.arp_flags = ARP_UP;
    arp_entry.arp_flags |= (INT32)RT_NETMGMT;


    memcpy(arp_entry.arp_mac_addr, paddr, DADDLEN);

    status = (INT16)NU_ARP_Update(&arp_entry, addr);

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(paddr);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    return (status);

} /* MIB2_Set_IpNetToMediaPhysAddress */

#ifndef SNMP_VERSION_COMP
/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaNetAddress
*
* DESCRIPTION
*
*       This function gets the IP address.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *ip_addr                The location where the physical address
*                               is to be stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaNetAddress(UINT8 *addr, UINT8 *ip_addr,
                                      UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY             entry;
    SCK_IOCTL_OPTION      option;

    /* Get the Entry. */
    if (getflag == NU_FALSE)
    {
        /* Check if the entry was found. */
        if (ARP_Get_Next(addr, &entry) != NU_NULL)
        {
            /* Check if the entry returned is not of the same address. */
            if (IP_ADDR(addr) ==
                LONGSWAP(IP_ADDR(entry.ip_addr.ip_address)))
            {
                /* Returning error code. */
                status = MIB2_UNSUCCESSFUL;
            }


            else
            {
                /* Update the addr variable. */
                PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

                /* Update the ip_addr variable. */
                NU_BLOCK_COPY(ip_addr, addr, MIB2_MAX_NETADDRSIZE);

                /* Returning success code. */
                status = NU_SUCCESS;
            }
        }

        /* If we did not get the next ARP entry. */
        else
            status = MIB2_UNSUCCESSFUL;
    }

    /* If this is get request. */
    else
    {
        /* Setting ARP request option. */
        NU_BLOCK_COPY(option.s_ret.arp_request.arp_pa.sck_data, addr,
                      MIB2_MAX_NETADDRSIZE);

        if (NU_Ioctl_SIOCGARP(&option) == NU_SUCCESS)
        {
            /* Update the ip_addr variable. */
            NU_BLOCK_COPY(ip_addr, addr, MIB2_MAX_NETADDRSIZE);

            status = NU_SUCCESS;
        }
        else
        {
            status = MIB2_UNSUCCESSFUL;
        }
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(ip_addr);
    UNUSED_PARAMETER(getflag);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    /* Returning status */
    return (status);

} /* MIB2_Get_IpNetToMediaNetAddress */

#else

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaNetAddress_If
*
* DESCRIPTION
*
*       This function gets the IP address.
*
* INPUTS
*
*       *index                  The interface index of the device.
*       *addr                   The IP address of the net to media table.
*       *ip_addr                The location where the physical address
*                               is to be stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaNetAddress_If(UINT32 *index, UINT8 *addr,
                                         UINT8 *ip_addr, UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)
    ARP_ENTRY             entry;
    ARP_ENTRY             *ret_entry;

    /* Get the next Entry. if the flag is not set*/
    if (getflag == NU_FALSE)
    {
        ret_entry = ARP_Get_Next_If(*index, addr, &entry);
    }
    else
    {
        ret_entry = ARP_Get_Index_If(*index, addr, &entry);
    }

    if (ret_entry != NU_NULL)
    {
        *index = ret_entry->arp_dev_index + 1;

        /* Update the addr variable. */
        PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

        /* Update the ip_addr variable. */
        NU_BLOCK_COPY(ip_addr, addr, MIB2_MAX_NETADDRSIZE);

        /* Returning success code. */
        status = NU_SUCCESS;
    }
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(ip_addr);
    UNUSED_PARAMETER(getflag);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    /* Returning status */
    return (status);

} /* MIB2_Get_IpNetToMediaNetAddress_If */

#endif

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpNetToMediaNetAddress
*
* DESCRIPTION
*
*       This function sets the IP address.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *ip_addr                The new value of ip_addr.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpNetToMediaNetAddress(const UINT8 *addr,
                                      const UINT8 *ip_addr)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY   arp_entry;

    /* Set all values in the arp_entry data structure to -1 */
    memset(&arp_entry, -1, sizeof(ARP_ENTRY));

    arp_entry.arp_flags = ARP_UP;
    arp_entry.arp_flags |= (INT32)RT_NETMGMT;

    /* Copy the new IP Address into the arp_entry data structure */
    memcpy(arp_entry.ip_addr.ip_address, ip_addr, IP_ADDR_LEN);

    status = (INT16)NU_ARP_Update(&arp_entry, addr);


#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(ip_addr);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    return (status);

} /* MIB2_Set_IpNetToMediaNetAddress */

#ifndef SNMP_VERSION_COMP
/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaType
*
* DESCRIPTION
*
*       This function gets the media type.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table.
*       *type                   The location where the type is to be
*                               stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaType(UINT8 *addr, UINT32 *type, UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY           entry;
    INT32               arp_flags = 0;
    SCK_IOCTL_OPTION    option;

    /* Get the Entry. */
    if (getflag == NU_FALSE)
    {
        /* Check if the entry was found. */
        if (ARP_Get_Next(addr, &entry) != NU_NULL)
        {
            /* Check if the entry returned is not of the same address. */
            if (IP_ADDR(addr) ==
                LONGSWAP(IP_ADDR(entry.ip_addr.ip_address)))
            {
                status = MIB2_UNSUCCESSFUL;
            }

            /* Update the addr variable. */
            else
            {
                /* Update the addr variable. */
                PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

                /* Storing ARP flags to determine ARP type. */
                arp_flags = entry.arp_flags;

                /* Setting status to success code. */
                status = NU_SUCCESS;
            }
        }

        /* If we did not get the next ARP entry. */
        else
            status = MIB2_UNSUCCESSFUL;
    }

    /* If we have get request. */
    else
    {
        /* Setting ARP request option. */
        NU_BLOCK_COPY(option.s_ret.arp_request.arp_pa.sck_data, addr,
                      MIB2_MAX_NETADDRSIZE);

        if (NU_Ioctl_SIOCGARP(&option) == NU_SUCCESS)
        {
            /* Storing ARP flag for determining the ARP type. */
            arp_flags = option.s_ret.arp_request.arp_flags;

            status = NU_SUCCESS;
        }
        else
        {
            status = MIB2_UNSUCCESSFUL;
        }
    }

    /* Check if the entry was found. */
    if (status == NU_SUCCESS)
    {
        if (arp_flags & ARP_PERMANENT)
            *type = 4;

        else if (arp_flags & ~ARP_PERMANENT)
            *type = 3;

        else
            *type = 0;
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(type);
    UNUSED_PARAMETER(getflag);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    /* Returning status */
    return (status);

} /* MIB2_Get_IpNetToMediaType */

#else

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IpNetToMediaType_If
*
* DESCRIPTION
*
*       This function gets the media type.
*
* INPUTS
*
*       *index                  The interface index of the device.
*       *addr                   The IP address of the net to media table.
*       *type                   The location where the type is to be
*                               stored.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IpNetToMediaType_If(UINT32 *index, UINT8 *addr,
                                   UINT32 *type, UINT8 getflag)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)
    ARP_ENTRY             entry;
    ARP_ENTRY             *ret_entry;

    /* Get the next Entry. if the flag is not set*/
    if (getflag == NU_FALSE)
    {
        ret_entry = ARP_Get_Next_If(*index, addr, &entry);
    }
    else
    {
        ret_entry = ARP_Get_Index_If(*index, addr, &entry);
    }

    if (ret_entry != NU_NULL)
    {
        *index = ret_entry->arp_dev_index + 1;

        /* Update the addr variable. */
        PUT32(addr, 0, entry.ip_addr.arp_ip_addr);

        if (ret_entry->arp_flags & ARP_PERMANENT)
            *type = 4;

        else if (ret_entry->arp_flags & ~ARP_PERMANENT)
            *type = 3;

        else
            *type = 0;

        /* Returning success code. */
        status = NU_SUCCESS;
    }
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

#else /* (INCLUDE_ARP == NU_TRUE) */

    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(type);
    UNUSED_PARAMETER(getflag);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    /* Returning status */
    return (status);

} /* MIB2_Get_IpNetToMediaType_If */

#endif

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IpNetToMediaType
*
* DESCRIPTION
*
*       This function sets the media type.
*
* INPUTS
*
*       *addr                   The IP address of the net to media table
*       ip_addr                 The type to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IpNetToMediaType(const UINT8 *addr, UINT32 type)
{
    INT16   status;

#if (INCLUDE_ARP == NU_TRUE)

    ARP_ENTRY   arp_entry;

    /* Set all values in the arp_entry data structure to -1 */
    memset(&arp_entry, -1, sizeof(ARP_ENTRY));

    arp_entry.arp_flags = ARP_UP;
    arp_entry.arp_flags |= (INT32)RT_NETMGMT;

    switch(type)
    {
        case 4:

            arp_entry.arp_flags |= ARP_PERMANENT;
            status = NU_SUCCESS;
            break;

        case 3:

            status = NU_SUCCESS;
            break;

        case 2:

            arp_entry.arp_flags &= ~ARP_UP;
            status = NU_SUCCESS;
            break;

        default:

            status = MIB2_UNSUCCESSFUL;
            break;
    }

    if (status == NU_SUCCESS)
        status = (INT16)NU_ARP_Update(&arp_entry, addr);

#else /* (INCLUDE_ARP == NU_TRUE) */
    /* Otherwise, return an error */
    status = MIB2_UNSUCCESSFUL;

    UNUSED_PARAMETER(addr);
    UNUSED_PARAMETER(type);

#endif /* (INCLUDE_ARP == NU_TRUE) */

    return (status);

} /* MIB2_Set_IpNetToMediaType */

#endif /* ( (MIB2_IP_INCLUDE == NU_TRUE) || (MIB2_AT_INCLUDE == NU_TRUE) ) */

#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */
