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
*       sck6_aip.c                                   
*                                                                                 
*   DESCRIPTION                                                           
*                                                                         
*       This file contains the function to add an IP address to an IPv6
*       enabled device.                
*                                                                         
*   DATA STRUCTURES                                                       
*                                                                         
*       None
*                                                                         
*   FUNCTIONS                                                             
*                                                                         
*       NU_Add_IP_To_Device                                                      
*
*   DEPENDENCIES                                                          
*                                                                         
*       externs.h
*       prefix6.h
*                                                                         
*************************************************************************/

#include "networking/externs.h"
#include "networking/prefix6.h"

/*************************************************************************
*
*   FUNCTION                                                              
*
*       NU_Add_IP_To_Device
*
*   DESCRIPTION                                                           
*
*       This function adds an IPv6 address to an IPv6 enabled device.
*
*   INPUTS                    
*                           
*       *dev_name               The name of the address to which to add 
*                               the IP address.
*       *ip_addr                The new IP address.
*       prefix_length           The length of the prefix in bits.
*       preferred_lifetime      The preferred lifetime of the new address.
*       valid_lifetime          The valid lifetime of the new address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IP address was successfully added.
*       NU_INVALID_PARM         One of the parameters is invalid.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS NU_Add_IP_To_Device(const CHAR *dev_name, const UINT8 *ip_addr, 
                           UINT8 prefix_length, UINT32 preferred_lifetime, 
                           UINT32 valid_lifetime)
{
    DV_DEVICE_ENTRY *device;
    STATUS          status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    device = DEV_Get_Dev_By_Name(dev_name);

    /* If an IPv6 enabled device was found */
    if ( (device) && (device->dev_flags & DV6_IPV6) )
    {
        status = DEV6_Add_IP_To_Device(device, ip_addr, prefix_length, 
                                       preferred_lifetime, valid_lifetime, 0);

        /* If the device is a configured device, add the prefix to the list of
         * prefixes for the device.
         */
        if ( (status == NU_SUCCESS) && (device->dev_type == DVT_CFG6) )
        {
            if (PREFIX6_New_Prefix_Entry(device, ip_addr, prefix_length, 
                                         valid_lifetime, 
                                         preferred_lifetime, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to add the Prefix to the Prefix List", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

#if (INCLUDE_IP_MULTICASTING)

        /* If the status is successful, the interface is multicast-enabled,
         * and Duplicate Address Detection is disabled on the interface, join 
         * the Solicited-Node multicast group for this address.  If Duplicate
         * Address Detection is enabled on the interface, the Solicited-Node 
         * multicast group will be joined upon successful completion of
         * Duplicate Address Detection.
         */
        if ( (status == NU_SUCCESS) && (device->dev_flags & DV_MULTICAST) &&
             ((device->dev_flags & DV6_NODAD) || (INCLUDE_DAD6 == NU_FALSE)) )
            IP6_Join_Solicited_Node_Multi(device, ip_addr, 0);

#endif
    }

    else
    {
        status = NU_INVALID_PARM;

        NLOG_Error_Log("No matching device entry for name", NERR_INFORMATIONAL, 
                       __FILE__, __LINE__);
    }

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Add_IP_To_Device */
