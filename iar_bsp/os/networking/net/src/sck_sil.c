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
*       sck_sil.c
*
*   DESCRIPTION
*
*       This file contains the API routine to configure the rate-limiting
*       value of outgoing ICMP error messages at run-time for the specified
*       interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCICMPLIMIT
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
*       NU_Ioctl_SIOCICMPLIMIT
*
*   DESCRIPTION
*
*       API routine to configure the rate-limiting value of outgoing ICMP
*       error messages at run-time for the specified interface.
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
STATUS NU_Ioctl_SIOCICMPLIMIT(CHAR *dev_name, UINT8 max_errors,
                              UINT32 interval)
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Set the value. */
        status = Ioctl_SIOCICMPLIMIT(dev_name, max_errors, interval);

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Ioctl_SIOCICMPLIMIT  */
