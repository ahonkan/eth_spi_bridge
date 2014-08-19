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

/***************************************************************************
*
*   FILENAME
*
*       dev_spa.c
*
*   DESCRIPTION
*
*       This file is responsible for changing the MAC address of the
*       specified device to a new MAC address
*
*   DATA STRUCTURES
*
*
*   FUNCTIONS
*
*       DEV_Set_Phys_Address
*
*   DEPENDENCIES
*
*       nu_net.h
*
****************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*****************************************************************************
*
*   FUNCTION
*
*       DEV_Set_Phys_Address
*
*   DESCRIPTION
*
*       This function changes the mac address of the specified device to
*       the new mac address.  If SNMP is included, the function also changes
*       the ipNetToMediaPhysAddress and atPhysAddress to the new mac
*       address
*
*   INPUTS
*
*       *ip_addr                A pointer to the device's IP address
*       *mac_addr               A pointer to the device's new mac address
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified IP
*                               address.
*
*****************************************************************************/
STATUS DEV_Set_Phys_Address(DV_DEVICE_ENTRY *dev_ptr, UINT8 *mac_addr)
{
    DV_REQ          d_req;
    STATUS          status;

    if (dev_ptr->dev_ioctl)
    {
        /* Fill in the d_req structure that will be passed to the driver's
           ioctl call */
        if (dev_ptr->dev_addr.dev_addr_list.dv_head)
            d_req.dvr_addr = dev_ptr->dev_addr.dev_ip_addr;
        else
            d_req.dvr_addr = 0;

        d_req.dvr_dvru.dvru_data = mac_addr;
        /* Call the driver's ioctl function */
        if ((*dev_ptr->dev_ioctl)(dev_ptr, DEV_SET_HW_ADDR, &d_req) != NU_SUCCESS)
        {
            NLOG_Error_Log("Device IOCTL call failed", NERR_SEVERE,
                           __FILE__, __LINE__);
            status = NU_INVALID_PARM;
        }
        else /* Update the device's mac address */
        {
        	memcpy(dev_ptr->dev_mac_addr, mac_addr, sizeof(dev_ptr->dev_mac_addr));
            status = NU_SUCCESS;
        }

    }
    else
    {
        status = NU_INVALID_PARM;
    }

    return (status);

} /* DEV_Set_Phys_Address */

#endif
