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
*       sck6_ar.c                                    
*                                                                                 
*   DESCRIPTION                                                           
*                                                                         
*       This file contains the function to add a route to the IPv6 
*       routing table.                
*                                                                         
*   DATA STRUCTURES                                                       
*                                                                         
*       None
*                                                                         
*   FUNCTIONS                                                             
*                                                                         
*       NU_Add_Route6    
*
*   DEPENDENCIES                                                          
*                                                                         
*       externs.h
*       nc6.h
*                                                                         
*************************************************************************/

#include "networking/externs.h"
#include "networking/nc6.h"

/*************************************************************************
*
*   FUNCTION                                                              
*
*       NU_Add_Route6
*
*   DESCRIPTION                                                           
*
*       This function adds an IPv6 route to the IPv6 Routing Table.
*
*   INPUTS                    
*                           
*       *ip_addr                The IP address of the destination.
*       *gateway                The Next-Hop node to the destination.
*       prefixlen               The length of the prefix associated with
*                               the destination of the route.
*
*   OUTPUTS
*
*       NU_SUCCESS              The route was successfully added.
*       NU_INVALID_PARM         One of the parameters is invalid.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS NU_Add_Route6(UINT8 *ip_addr, UINT8 *gateway, UINT8 prefixlen)
{
    RTAB6_ROUTE                 ro;
    STATUS                      status;
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry = NU_NULL;
    UINT32                      flags;
    DV_DEVICE_ENTRY             *dev;
    NU_SUPERV_USER_VARIABLES

    /* Validate the parameters */
    if ( (gateway == NU_NULL) || (ip_addr == NU_NULL) )
        status = NU_INVALID_PARM;
    else
    {
        /* Switch to supervisor mode. */
        NU_SUPERVISOR_MODE();
		
		memset(&ro, 0, sizeof(RTAB6_ROUTE));

        NU_BLOCK_COPY(ro.rt_ip_dest.rtab6_rt_ip_dest.sck_addr, 
                      gateway, IP6_ADDR_LEN);

        ro.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;

        /* Obtain the semaphore */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status != NU_SUCCESS)
        {
            NU_USER_MODE();
            return (status);
        }
    
        /* Find a route to the gateway */
        IP6_Find_Route(&ro);

        if (ro.rt_route)
        {
            RTAB_Free((ROUTE_ENTRY*)ro.rt_route, NU_FAMILY_IP6);

            flags = (RT_UP | RT_STATIC);

            /* If the gateway is not a directly connected interface, flag
             * the route as a Gateway route.
             */
            dev = DEV6_Get_Dev_By_Addr(gateway);

            if (dev == NU_NULL)
            {
                dev = ro.rt_route->rt_entry_parms.rt_parm_device;
                flags |= RT_GATEWAY;
            }
    
            /* If the prefix length is 128 it is a host route. Otherwise, 
             * it is a network route.
             */
            if (prefixlen == 128)
                flags |= RT_HOST;
             
            /* If the caller is specifying a default route, set the default 
             * route.
             */
            if (IPV6_IS_ADDR_UNSPECIFIED(ip_addr))
                status = RTAB6_Set_Default_Route(dev, gateway, flags);
            else
            {          
                /* The destination cache entry will have only one next-hop entry,
                 * because the gateway is a neighboring node.
                 */
                if (ro.rt_route)
                {
                    /* If a Neighbor Cache entry does not already exist for
                     * the gateway, add a Neighbor Cache entry.
                     */
                    nc_entry = dev->dev6_fnd_neighcache_entry(dev, 
                                                              ro.rt_route->rt_route_node->
                                                              rt_ip_addr);

                    if (nc_entry == NU_NULL)
                    {
                        nc_entry = dev->
                            dev6_add_neighcache_entry(dev, gateway, NU_NULL, 0, 
                                                      NU_NULL, 
                                                      NC_NEIGH_INCOMPLETE);
                    }
                }

                /* Create the route */
                if (nc_entry)
                {
                    flags |= RT_LOCAL;

                    status = RTAB6_Add_Route(dev, ip_addr, 
                                             nc_entry->ip6_neigh_cache_ip_addr, 
                                             prefixlen, flags);
                }
            }
        }
        else
            status = NU_INVALID_PARM;

        /* Release the semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();
    }

    return (status);

} /* NU_Add_Route6 */
