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
*       ioctl_req.c
*
*   DESCRIPTION
*
*       This file contains the routine that issues an ioctl command
*       directly to the device
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCIFREQ
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
*       Ioctl_SIOCIFREQ
*
*   DESCRIPTION
*
*       This function issues an ioctl command directly to the device
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         The device is invalid.
*
*       Otherwise, interface specific error code.
*
*************************************************************************/
STATUS Ioctl_SIOCIFREQ(SCK_IOCTL_OPTION *option)
{
    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

    /* Get a pointer to the device */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    if ( (dev) && (dev->dev_ioctl) )
        status = (*dev->dev_ioctl)(dev, option->s_ret.s_dvreq.dvr_optname,
                                   &(option->s_ret.s_dvreq));
    else
        status = NU_INVALID_PARM;

    return (status);

} /* Ioctl_SIOCIFREQ */

