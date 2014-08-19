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
*       ioctl_shwo.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable hardware
*       offloading options on an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCSHWOPTS
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
*       Ioctl_SIOCSHWOPTS
*
*   DESCRIPTION
*
*       This function enables or disables hardware offloading options
*       on an interface.
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
STATUS Ioctl_SIOCSHWOPTS(const SCK_IOCTL_OPTION *option)
{
#if (HARDWARE_OFFLOAD == NU_TRUE)

    DV_DEVICE_ENTRY *dev;
    STATUS          status;

    /* Get a pointer to the device entry */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If the device exits */
    if ( (dev != NU_NULL) && (dev->dev_ioctl != NU_NULL) )
    {
        /* Disable set of offloading features */
        if (option->s_ret.s_dvreq.dvr_scmd == DEV_DISABLE_HW_OPTIONS)
        {
            dev->dev_hw_options_enabled &=
                ~(option->s_ret.s_dvreq.dvr_pcmd);
        }

        /* Enable set of offloading features */
        else if (option->s_ret.s_dvreq.dvr_scmd == DEV_ENABLE_HW_OPTIONS)
        {
            dev->dev_hw_options_enabled |=
                option->s_ret.s_dvreq.dvr_pcmd;
        }

        /* Inform the controller of the changes */
        status = (dev->dev_ioctl)(dev, DEV_HW_OFFLOAD_CTRL, NU_NULL);
    }

    else
        status = NU_INVALID_PARM;

    return (status);

#else

    UNUSED_PARAMETER(option);

    return (NU_INVALID_PARM);

#endif

} /* Ioctl_SIOCSHWOPTS */
