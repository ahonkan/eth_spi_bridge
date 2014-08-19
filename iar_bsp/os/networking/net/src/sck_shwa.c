/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       sck_shwa.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the interface's MAC Address
*       by name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCSIFHWADDR()
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
*       NU_Ioctl_SIOCSIFHWADDR
*
*   DESCRIPTION
*
*       This function sets the MAC address of the interface by fetching it
*       by name.
*
*   INPUTS
*
*       *option                 Contains the name of the interface whose
*                               MAC has to be set, name must be assigned
*                               to option->s_optval.
*                               Contains the MAC address in
*                               option->s_ret.mac_address[] which needs to
*                               be set on the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         No device exists with the specified
*                               Interface name.
*
*************************************************************************/
STATUS NU_Ioctl_SIOCSIFHWADDR(SCK_IOCTL_OPTION *option)
{
    STATUS status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the user passed a NULL parameter, set status to error */
    if (!option)
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
    if (status == NU_SUCCESS)
    {
        status = Ioctl_SIOCSIFHWADDR(option);

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* Ioctl_SIOCGIFHWADDR */
