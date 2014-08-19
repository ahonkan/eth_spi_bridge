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
*       ioctl_gid.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the IP address of the
*       foreign side of a PPP link.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGIFDSTADDR
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       Ioctl_SIOCGIFDSTADDR
*
*   DESCRIPTION
*
*       This function retrieves the IP address of the foreign side of a
*       PPP link.
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IP address was properly filled in.
*       NU_INVALID_PARM         The device does not exist or the device
*                               is not a PPP device.
*
*************************************************************************/
STATUS Ioctl_SIOCGIFDSTADDR(SCK_IOCTL_OPTION *option)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev;

    /* Get a pointer to the device entry */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If this is a valid PPP device, get the IP address */
    if ( (dev) && (dev->dev_type == DVT_PPP) )
    {
        status = NU_SUCCESS;

        *(UINT32 *)option->s_ret.s_ipaddr =
                        LONGSWAP(dev->dev_addr.dev_dst_ip_addr);
    }

    /* Otherwise, return an error */
    else
        status = NU_INVALID_PARM;

    return (status);

} /* Ioctl_SIOCGIFDSTADDR */

#endif
