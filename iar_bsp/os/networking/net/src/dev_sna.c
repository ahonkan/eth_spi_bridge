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
*       dev_sna.c
*
*   DESCRIPTION
*
*       This file is responsible for changing the IP address of the
*       specified device to a new IP address
*
*   DATA STRUCTURES
*
*
*   FUNCTIONS
*
*       DEV_Set_Net_Address
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
*       DEV_Set_Net_Address
*
*   DESCRIPTION
*
*       This function changes the IP address of the specified device to
*       the new IP address.
*
*  INPUTS
*
*       *old_ip_addr            A pointer to the device's old IP address
*       *new_ip_addr            A pointer to the new IP address
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified IP
*                               address.
*
*****************************************************************************/
STATUS DEV_Set_Net_Address(const UINT8 *old_ip_addr, const UINT8 *new_ip_addr)
{
    DV_DEVICE_ENTRY     *dev;
    STATUS              status;
    UINT8               subnet[4];
    DEV_IF_ADDR_ENTRY   *target_addr;

    /* Get the device using the IP Address */
    dev = DEV_Get_Dev_By_Addr(old_ip_addr);

    if (dev == NU_NULL)
        status = NU_INVALID_PARM;

    else
    {
        /* Get a pointer to the target address structure on the device */
        target_addr = DEV_Find_Target_Address(dev, IP_ADDR(old_ip_addr));

        PUT32(subnet, 0, target_addr->dev_entry_netmask);

        /* Delete the IP address from the device */
        if (DEV4_Delete_IP_From_Device(dev, target_addr) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to remove existing IP address from interface",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Add the new IP address to the device */
        status = DEV_Attach_IP_To_Device(dev->dev_net_if_name, new_ip_addr,
                                         subnet);

        /* Notify the sockets layer to resume all tasks using the old
         * IP address and return an error to the user.  Wait until after
         * the new IP address is attached to do this, so listening
         * sockets bound to IP_ADDR_ANY are not affected.
         */
        DEV_Resume_All_Open_Sockets();
    }

    return (status);

} /* DEV_Set_Net_Address */

#endif
