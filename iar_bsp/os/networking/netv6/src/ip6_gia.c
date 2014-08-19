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
*       ip6_gia.c                                    
*
*   DESCRIPTION
*
*       This file contains the implementation of IP6_Ioctl_SIOCGIFADDR_IN6.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Ioctl_SIOCGIFADDR_IN6
*
*   DEPENDENCIES
*
*       nu_net.h
*       in6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/in6.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Ioctl_SIOCGIFADDR_IN6                                                         
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Returns the best scoped address for the device.  If a globally
*       unique IP address does not exist on the device, the link-local
*       address is returned.  If no link-local address exists, an error
*       is returned.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *dev_name               A pointer to the device of which to
*                               return the IP address.
*       *ip_addr                A pointer to the memory in which to
*                               store the address.
*       *prefix_len             A pointer to store prefix length for that
*                               address.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         The device does not exist or there is no
*                               link-local or global address on the
*                               device.
*                                                                       
*************************************************************************/
STATUS IP6_Ioctl_SIOCGIFADDR_IN6(const CHAR *dev_name, UINT8 *ip_addr,
                                 UINT8 *prefix_len)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev;
    DEV6_IF_ADDRESS *current_address, *link_local_addr = NU_NULL;
    int             current_scope;

    /* Get a pointer to the device */
    dev = DEV_Get_Dev_By_Name(dev_name);

    if (dev == NU_NULL)
    {
        status = NU_INVALID_PARM;
    }
    else if (dev->dev_flags & DV6_IPV6)
    {
        /* Get a pointer to the list of addresses for the device */
        current_address = dev->dev6_addr_list.dv_head;

        /* Traverse the list of addresses for a globally unique 
         * IP address.  If one does not exist but a link-local IP
         * address does exist, return the link-local address.
         */
        while (current_address)
        {
            /* Check the scope of the current address */
            current_scope = in6_addrscope(current_address->dev6_ip_addr);
    
            /* If the address is globally unique, break */
            if (current_scope == IPV6_ADDR_SCOPE_GLOBAL)
                break;

            /* If the address is link-local, save it off */
            else if (current_scope == IPV6_ADDR_SCOPE_LINKLOCAL)
                link_local_addr = current_address;

            /* Get the next address */
            current_address = current_address->dev6_next;
        }

        /* If a globally unique IP address was found, return it */
        if (current_address != NU_NULL)
        {
            memcpy(ip_addr, current_address->dev6_ip_addr, IP6_ADDR_LEN);
            *prefix_len = current_address->dev6_prefix_length;

            status = NU_SUCCESS;
        }

        /* Otherwise, if a link-local IP address was found, return it */
        else if (link_local_addr != NU_NULL)
        {
            memcpy(ip_addr, link_local_addr->dev6_ip_addr, IP6_ADDR_LEN);
            *prefix_len = link_local_addr->dev6_prefix_length;

            status = NU_SUCCESS;
        }
       
        /* Otherwise, no address was found.  Return an error */
        else
            status = NU_INVALID_PARM;
    }
    else
    {
        status = NU_INVALID_PARM;
    }

    return (status);

} /* IP6_Ioctl_SIOCGIFADDR_IN6 */
