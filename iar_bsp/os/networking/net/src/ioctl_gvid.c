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
*       ioctl_gvid.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the MAC address
*       associated with an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGETVLAN
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
*       Ioctl_SIOCGETVLAN
*
*   DESCRIPTION
*
*       This function gets the VLAN ID of an interface.
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified name.
*
*************************************************************************/
STATUS Ioctl_SIOCGETVLAN(SCK_IOCTL_OPTION *option)
{
#if (INCLUDE_VLAN == NU_TRUE)

    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

    /* Get a pointer to the device specified in s_optval */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If the device is valid, get the VLAN ID as requested */
    if (dev)
    {
#if (USE_SW_VLAN_METHOD == NU_TRUE)
        option->s_ret.vlan_id = dev->dev_vlan_vid;
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

} /* Ioctl_SIOCGETVLAN */
