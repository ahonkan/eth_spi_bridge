/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       ip6_gnd.c
*
*   DESCRIPTION
*
*       This file contains the implementation of IP6_Ioctl_SIOCLIFGETND.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Ioctl_SIOCLIFGETND
*
*   DEPENDENCIES
*
*       nu_net.h
*       in6.h
*       nc6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/in6.h"
#include "networking/nc6.h"
#include "networking/nc6_eth.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Ioctl_SIOCLIFGETND
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function retrieves the neighbor cache entry associated with
*       an IPv6 address and returns the MAC address associated with that
*       IPv6 address.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *dev_name               Name of the device to be used to look for
*                               the neighbor cache entry.
*       *ip6_addr               IPv6 address for which the neighbor cache
*                               entry must be retrieved.
*       *mac_addr               Return pointer for the MAC address.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Successful completion of the service.
*       NU_NOT_FOUND            A matching neighbor cache entry was not
*                               found.
*       NU_INVALID_PARM         The device does not exist or there is no
*                               link-local or global address on the
*                               device.
*                                                                       
*************************************************************************/
STATUS IP6_Ioctl_SIOCLIFGETND(const CHAR *dev_name, UINT8 *ip6_addr,
                              UINT8 *mac_addr)
{
    STATUS                    status = NU_INVALID_PARM;
    DV_DEVICE_ENTRY           *dev_ptr;
    IP6_NEIGHBOR_CACHE_ENTRY  *nc_entry;

    /* Get a pointer to the device */
    dev_ptr = DEV_Get_Dev_By_Name(dev_name);

    /* Ensure that we have a device pointer and that IPv6 is enabled on the
     * device.
     */
    if ( (dev_ptr != NU_NULL) && (dev_ptr->dev_flags & DV6_IPV6) )
    {
        /* Look for the neighbor cache entry associated with the IPv6 address. */
        nc_entry = dev_ptr->dev6_fnd_neighcache_entry(dev_ptr, ip6_addr);

        if (nc_entry)
        {
            /* Retrieve the MAC address from the neighbor cache. */
            memcpy(mac_addr, ((IP6_ETH_NEIGHBOR_CACHE_ENTRY*)
                              (nc_entry->ip6_neigh_cache_link_spec)) -> ip6_neigh_cache_hw_addr,
                   dev_ptr->dev_addrlen);

            status = NU_SUCCESS;
        }
        else
        {
            NLOG_Error_Log("Failed to find a matching neighbor cache entry",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            status = NU_NOT_FOUND;
        }
    }

    return (status);
} /* IP6_Ioctl_SIOCLIFGETND */
