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
*       ioctl_gsm.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the subnet mask
*       of an IP address associated with a specific interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGIFNETMASK
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
*       Ioctl_SIOCGIFNETMASK
*
*   DESCRIPTION
*
*       This function retrieves the subnet mask associated with an IP
*       address of a specific interface.  If no IP address is specified,
*       the routine returns the subnet mask of the first IP address
*       in the list of addresses for the interface.
*
*   INPUTS
*
*       *option                 s_optval member holds the name of the
*                               interface, and s_ret.s_ipaddr holds the
*                               IP address on the interface for which to
*                               retrieve the subnet mask.  If no IP address
*                               is specified, the routine returns the
*                               subnet mask of the first IP address in
*                               the list of addresses for the interface.
*                               On return, s_ret.s_ipaddr holds the subnet
*                               mask.
*
*   OUTPUTS
*
*       NU_SUCCESS              The subnet mask was properly filled in.
*       NU_INVALID_PARM         The device does not exist or the IP address
*                               specified does not exist on the device.
*
*************************************************************************/
STATUS Ioctl_SIOCGIFNETMASK(SCK_IOCTL_OPTION *option)
{
    STATUS              status = NU_SUCCESS;
    DV_DEVICE_ENTRY     *dev;
    DEV_IF_ADDR_ENTRY   *addr_struct = NU_NULL;

    /* Get a pointer to the device */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If the device is valid */
    if (dev != NU_NULL)
    {
        /* If an IP address was specified. */
        if ((IP_ADDR(option->s_ret.s_ipaddr)) != 0)
        {
            /* Find the IP address structure associated with the IP address
             * passed in by the user.
             */
            addr_struct =
                DEV_Find_Target_Address(dev, IP_ADDR(option->s_ret.s_ipaddr));

            /* If no address structure was found. */
            if (!addr_struct)
                status = NU_INVALID_PARM;
        }

        if (status == NU_SUCCESS)
        {
            /* Fill in the subnet mask from the address structure. */
            if (addr_struct)
            {
                PUT32(option->s_ret.s_ipaddr, 0,
                      addr_struct->dev_entry_netmask);
            }

            /* Fill in the subnet mask from the first IP address entry
             * on the interface.
             */
            else
            {
                PUT32(option->s_ret.s_ipaddr, 0, dev->dev_addr.dev_netmask);
            }
        }
    }

    /* Otherwise, return an error */
    else
        status = NU_INVALID_PARM;

    return (status);

} /* Ioctl_SIOCGIFNETMASK */

#endif
