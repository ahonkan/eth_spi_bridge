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
*       ip6_gda.c                                    
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       IP6_Ioctl_SIOCGIFDSTADDR_IN6.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Ioctl_SIOCGIFDSTADDR_IN6
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Ioctl_SIOCGIFDSTADDR_IN6                                                         
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Returns the IPv6 address of the other side of the PPP link.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *dev_name               A pointer to the device of which to
*                               return the IP address.
*       *ip_addr                A pointer to the memory in which to
*                               store the address.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         The device does not exist or the device
*                               is not a PPP device.
*                                                                       
*************************************************************************/
STATUS IP6_Ioctl_SIOCGIFDSTADDR_IN6(const CHAR *dev_name, UINT8 *ip_addr)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev;

    /* Get a pointer to the device */
    dev = DEV_Get_Dev_By_Name(dev_name);

    /* If the device does not exist or the device is not a PPP device,
     * return an error.
     */
    if ( (dev == NU_NULL) || (dev->dev_type != DVT_PPP) ||
         !(dev->dev_flags & DV6_IPV6) )
    {
        status = NU_INVALID_PARM;
    }

    /* Otherwise, copy the address into the user's data structure */
    else
    {
        memcpy(ip_addr, dev->dev6_addr_list.dev6_dst_ip_addr, IP6_ADDR_LEN);
        status = NU_SUCCESS;
    }

    return (status);

} /* IP6_Ioctl_SIOCGIFDSTADDR_IN6 */
