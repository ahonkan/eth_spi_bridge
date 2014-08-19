/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       ioctl_shwa.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the interface's MAC Address
*       by name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCSIFHWADDR()
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
*       Ioctl_SIOCSIFHWADDR
*
*   DESCRIPTION
*
*       This function sets the MAC address of the interface by fetching it
*       by name.
*
*   INPUTS
*
*       *option                 Contains the name of the interface whose
*                               MAC has to be set, name must be assigned
*                               to option->s_optval.
*                               Contains the MAC address in
*                               option->s_ret.mac_address[] which needs to
*                               be set on the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_DEVICE_NOT_DOWN      IOCTL was called for an interface which was not
*                               in "link down" state.
*       NU_INVALID_PARM         No device exists with the specified
*                               Interface name.
*
*************************************************************************/
STATUS Ioctl_SIOCSIFHWADDR(SCK_IOCTL_OPTION *option)
{
    STATUS status;
    DV_DEVICE_ENTRY *dev_ptr;

    dev_ptr = DEV_Get_Dev_By_Name((CHAR*)option->s_optval);
    if (dev_ptr)
    {
        /* Return error if interface is not in "link down" state. */
        if (dev_ptr->dev_flags & DV_UP)
        {
            status = NU_DEVICE_NOT_DOWN;
        }
        else
        {
            status = DEV_Set_Phys_Address(dev_ptr, option->s_ret.mac_address);
        }
    }
    else
    {
        status = NU_INVALID_PARM;
    }

    return (status);
} /* Ioctl_SIOCGIFHWADDR */
