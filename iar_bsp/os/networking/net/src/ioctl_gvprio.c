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
*       ioctl_gvprio.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the VLAN user priority
*       associated with an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGETVLANPRIO
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
*       Ioctl_SIOCGETVLANPRIO
*
*   DESCRIPTION
*
*       This function gets the VLAN priority of an interface.
*
*   INPUTS
*
*       *option                 Pointer that holds the interface name
*                               of the interface for which to retrieve
*                               the priority in the s_optval parameter
*                               of the structure.  Upon successful
*                               completion, the routine fills in the
*                               s_ret.vlan_prio member of the structure
*                               with the VLAN priority of the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified name.
*
*************************************************************************/
STATUS Ioctl_SIOCGETVLANPRIO(SCK_IOCTL_OPTION *option)
{
#if (INCLUDE_VLAN == NU_TRUE)

    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

    /* Get a pointer to the device specified in s_optval */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If the device is valid, get the VLAN priority as requested */
    if (dev)
    {
#if (USE_SW_VLAN_METHOD == NU_TRUE)
        option->s_ret.vlan_prio = dev->dev_vlan_vprio;
        status = NU_SUCCESS;
#else
        status = (dev->dev_ioctl)(dev, DEV_GET_VLAN_TAG,
                                  &option->s_ret.s_dvreq);
#endif
    }

    else
        status = NU_INVALID_PARM;

    return (status);

#else

    UNUSED_PARAMETER(option);

    return (NU_INVALID_PARM);

#endif

} /* Ioctl_SIOCGETVLANPRIO */
