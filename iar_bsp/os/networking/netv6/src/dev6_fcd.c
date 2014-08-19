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
*       dev6_fcd.c                                   
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to maintain the IP
*       layer for IPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       DEV6_Find_Configured_Device
*                                                                          
*   DEPENDENCIES                                                             
*              
*       target.h
*       nu_net.h
*                                                                          
*************************************************************************/

#include "networking/target.h"
#include "networking/nu_net.h"

/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       DEV6_Find_Configured_Device
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function returns a pointer to the first real device in the
*       system that has an IPv6 address.
*                                                                       
*   INPUTS                                                                
*       
*       None.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       A pointer to a device with an IPv6 address or NU_NULL if there
*       are no devices with an IPv6 address.
*                                                                       
*************************************************************************/
DV_DEVICE_ENTRY *DEV6_Find_Configured_Device(VOID)
{
    DV_DEVICE_ENTRY *current_dev;

    /* Get a pointer to the first device on the node */
    current_dev = DEV_Table.dv_head;

#if (INCLUDE_LOOPBACK_DEVICE)

    /* Get the first real device in the system */
    current_dev = current_dev->dev_next;

#endif

    while (current_dev)
    {
        /* If this device is IPv6 enabled and there is an IPv6 address on 
         * the device.
         */
        if (current_dev->dev_flags & DV6_IPV6)
        {
            if (current_dev->dev6_addr_list.dv_head)
                break;
        }

        current_dev = current_dev->dev_next;
    }

    return (current_dev);

} /* DEV6_Find_Configured_Device */
