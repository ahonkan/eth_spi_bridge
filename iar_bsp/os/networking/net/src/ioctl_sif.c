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
*       ioctl_sif.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the IP address
*       associated with an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCSIFADDR
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
*       Ioctl_SIOCSIFADDR
*
*   DESCRIPTION
*
*       This function sets the IP address associated with an interface
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
STATUS Ioctl_SIOCSIFADDR(const SCK_IOCTL_OPTION *option)
{
    return (DEV_Set_Net_Address(option->s_ret.s_ipaddr, option->s_optval));

} /* Ioctl_SIOCSIFADDR */
