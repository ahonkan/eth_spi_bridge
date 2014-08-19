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
*       ioctl_sil.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the rate-limiting
*       value of outgoing ICMP error messages at run-time for the specified
*       interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCICMPLIMIT
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
*       Ioctl_SIOCICMPLIMIT
*
*   DESCRIPTION
*
*       Configure the rate-limiting value of outgoing ICMP error messages
*       at run-time for the specified interface.
*
*   INPUTS
*
*       *dev_name               A pointer to the name of the interface for
*                               which to configure the ICMP rate-limiting
*                               value.
*       max_errors              The number of ICMP error messages to permit
*                               in a certain interval.
*       interval                The interval in hardware ticks over which
*                               to limit the number of ICMP error messages
*                               transmitted over the specified interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*
*************************************************************************/
STATUS Ioctl_SIOCICMPLIMIT(CHAR *dev_name, UINT8 max_errors, UINT32 interval)
{
    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

    /* Get a pointer to the interface */
    dev = DEV_Get_Dev_By_Name(dev_name);

    /* If the interface name is valid, configure the option */
    if ( (dev) && (max_errors > 0) && (interval > 0) )
    {
        /* Set the interval */
        dev->dev_error_msg_rate_limit = interval;

        /* Set the maximum number of errors allowed in the given interval */
        dev->dev_max_error_msg = max_errors;

        status = NU_SUCCESS;
    }

    /* One of the input parameters is invalid */
    else
    {
        status = NU_INVALID_PARM;
    }

    return (status);

} /* Ioctl_SIOCICMPLIMIT */
