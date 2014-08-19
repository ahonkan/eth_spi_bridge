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
*       ioctl_ghwc.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the hardware
*       offloading capabilities of the specific interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGHWCAP
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
*       Ioctl_SIOCGHWCAP
*
*   DESCRIPTION
*
*       This function retrieves the hardware offloading capabilities of
*       an interface.
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
STATUS Ioctl_SIOCGHWCAP(SCK_IOCTL_OPTION *option)
{
#if (HARDWARE_OFFLOAD == NU_TRUE)

    DV_DEVICE_ENTRY *dev;
    STATUS          status;

    /* Get a pointer to the device */
    dev = DEV_Get_Dev_By_Name((CHAR *)option->s_optval);

    /* If the device is valid */
    if (dev != NU_NULL)
    {
        /* Does this device support the hw query function */
        if (dev->dev_hw_query)
        {
            /* Query the interface for the supported hardware offloading
             * capabilities.
             */
            (dev->dev_hw_query)(dev);

            /* Save the options in the user provided data structure */
            option->s_ret.s_dvreq.dvr_flags = dev->dev_hw_options;
        }

        /* Otherwise, the interface cannot be queried */
        else
            option->s_ret.s_dvreq.dvr_flags = NU_NULL;

        status = NU_SUCCESS;
    }
    else
        status = NU_INVALID_PARM;

    return (status);

#else

    UNUSED_PARAMETER(option);

    return (NU_INVALID_PARM);

#endif

} /* Ioctl_SIOCGHWCAP */
