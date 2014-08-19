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
*       ioctl_gif.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the IP address
*       associated with an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGIFADDR
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
*       Ioctl_SIOCGIFADDR
*
*   DESCRIPTION
*
*       This function retrieves the IP address associated with an
*       interface.
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IP address was properly filled in.
*       NU_INVALID_PARM         The device does not exist.
*       NU_INVALID_ADDRESS      The address does not exist.
*
*************************************************************************/
STATUS Ioctl_SIOCGIFADDR(SCK_IOCTL_OPTION *option)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev;

    /* Get a pointer to the device */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If the device is valid */
    if (dev != NU_NULL)
    {
        /* Ensure that the device is UP and there is an IP address assigned to
         * it.
         */
        if ( (dev->dev_flags & DV_UP) && (dev->dev_addr.dev_addr_list.dv_head) )
        {
            /* Fill in the IP address */
            *(UINT32 *)option->s_ret.s_ipaddr =
                    LONGSWAP(dev->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr);

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = NU_INVALID_ADDRESS;
    }

    /* Otherwise, return an error */
    else
        status = NU_INVALID_PARM;

    return (status);

} /* Ioctl_SIOCGIFADDR */

#endif
