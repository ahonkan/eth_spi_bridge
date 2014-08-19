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

/*************************************************************************
*
*   FILE NAME
*
*       ioctl_sphy.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the MAC address
*       associated with an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCSPHYSADDR
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
*       Ioctl_SIOCSPHYSADDR
*
*   DESCRIPTION
*
*       This function sets the MAC address associated with an interface
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified IP
*                               address.
*
*************************************************************************/
STATUS Ioctl_SIOCSPHYSADDR(const SCK_IOCTL_OPTION *option)
{
    DV_DEVICE_ENTRY     *dev_ptr;
    STATUS              status;

    dev_ptr = DEV_Get_Dev_By_Addr(option->s_ret.s_ipaddr);

    if (dev_ptr)
        status = DEV_Set_Phys_Address(dev_ptr, option->s_optval);
    else
        status = NU_INVALID_PARM;

    return (status);

} /* Ioctl_SIOCSPHYSADDR */
