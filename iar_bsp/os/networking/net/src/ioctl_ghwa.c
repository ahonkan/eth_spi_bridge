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
*       ioctl_ghwa.c
*
*   DESCRIPTION
*
*       This file contains the routine to fetch the physical address
*       of an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGIFHWADDR()
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
*       Ioctl_SIOCGIFHWADDR
*
*   DESCRIPTION
*
*       This function gets the MAC address of the interface by name.
*
*   INPUTS
*
*       *option                 Contains the name of the interface whose
*                               MAC has to be fetched, name must be
*                               assigned to option->s_optval.
*                               Contains the returned MAC address i.e.
*                               assigned to option->s_ret.mac_address[].
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified
*                               Interface name.
*
*************************************************************************/
STATUS Ioctl_SIOCGIFHWADDR(SCK_IOCTL_OPTION *option)
{
    STATUS status;
    DV_DEVICE_ENTRY *dev_ptr;

    dev_ptr = DEV_Get_Dev_By_Name((CHAR*)option->s_optval);
    if (dev_ptr)
    {
        /* Copy the Mac of this device in the return option ptr. */
        memcpy(option->s_ret.mac_address, dev_ptr->dev_mac_addr, DADDLEN);
        status = NU_SUCCESS;
    }
    else
    {
        status = NU_INVALID_PARM;
    }

    return (status);
} /* Ioctl_SIOCGIFHWADDR */
