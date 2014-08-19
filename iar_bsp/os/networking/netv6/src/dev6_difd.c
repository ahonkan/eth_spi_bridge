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
*       dev6_difd.c                                  
*                                                                       
*   DESCRIPTION                                                           
*                     
*       This file contains the function to remove an IP address from
*       a device.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       DEV6_Detach_IP_From_Device
*                                                                       
*   DEPENDENCIES                                                          
*               
*       externs.h
*       nd6radv.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/nd6radv.h"

extern UINT8    IP6_Loopback_Address[];

/**************************************************************************
*
*   FUNCTION                                                                 
*                                                                          
*       DEV6_Detach_IP_From_Device                                               
*                                                                          
*   DESCRIPTION                                                              
*                                                                          
*       Given a device name, this routine will remove the IP number from 
*       the network interface table for the device.                                 
*                                                                          
*   INPUTS                                                                   
*                                                                          
*       *dv_ptr                 A pointer to the device.
*                                                                          
*   OUTPUTS                                                                  
*                                                                          
*       NU_SUCCESS              device was updated.                                 
*       -1                      device was NOT updated.                             
*                                                                          
****************************************************************************/
STATUS DEV6_Detach_IP_From_Device(DV_DEVICE_ENTRY *dv_ptr)
{
    INT                 status;
    DEV6_IF_ADDRESS     *current_address;

    /*  If the device is not NULL and it is UP.  */
    if ( (dv_ptr != NU_NULL) && (dv_ptr->dev_flags & DV_UP) &&
    	 (dv_ptr->dev_flags2 & DV6_UP) )
    {
#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)
        
        current_address = dv_ptr->dev6_addr_list.dv_head;

        while (current_address)
        {
            /* Remove the route through the loopback address for this
             * address.
             */
            if (RTAB6_Delete_Route(current_address->dev6_ip_addr, 
                                   IP6_Loopback_Address) != NU_SUCCESS)
                NLOG_Error_Log("Failed to delete route for device", 
                               NERR_SEVERE, __FILE__, __LINE__);

            current_address = current_address->dev6_next;
        }
#endif            

        /* We only need to remove the addresses if this is not a 
         * point to point device. 
         */
        if (!(dv_ptr->dev_flags & DV_POINTTOPOINT))
        {
#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

            /* If this device is configured to transmit Router Advertisement
             * messages, send a final Router Advertisement with the
             * router lifetime set to 0.
             */
            if (dv_ptr->dev_flags & DV6_ISROUTER)
            {
                dv_ptr->dev6_AdvDefaultLifetime = 0;

                ND6RADV_Output(dv_ptr);
            }
#endif

            current_address = dv_ptr->dev6_addr_list.dv_head;

            /* Remove each address from the list of addresses for the device
             * and remove the route for each address.
             */
            while (current_address)
            {
                /* Delete the IP address from the device */
                DEV6_Delete_IP_From_Device(current_address);

                /* Get a pointer to the next address to remove */
                current_address = dv_ptr->dev6_addr_list.dv_head;
            }
        }

        status = NU_SUCCESS;
    }
    else
        status = -1;

    return (status);

} /* DEV6_Detach_IP_From_Device */
