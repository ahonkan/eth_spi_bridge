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
* FILE NAME
*
*       dev_du.c
*
* DESCRIPTION
*
*       This file contains the implementation of DEV_Device_Up.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Device_Up
*       DEV_Device_Up
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

/**************************************************************************
*
*   FUNCTION
*
*       NU_Device_Up
*
*   DESCRIPTION
*
*       Given a device name, this routine will return NU_TRUE if device
*       is up else NU_FALSE.
*
*   INPUTS
*
*       *if_name                A pointer to the name of the Device
*
*   OUTPUTS
*
*       NU_TRUE                 The device is up.
*       NU_FALSE                The device is not up.
*       NU_INVALID_PARM         The name passed in is NULL.
*
****************************************************************************/
STATUS NU_Device_Up(const CHAR *if_name)
{
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        status = DEV_Device_Up(if_name);

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Trace log */
    T_DEV_UP_STATUS((char*)if_name, status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Device_Up */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Device_Up
*
*   DESCRIPTION
*
*       Given a device name, this routine will return NU_TRUE if device
*       is up else NU_FALSE.
*
*   INPUTS
*
*       *if_name                A pointer to the name of the Device
*
*   OUTPUTS
*
*       NU_TRUE                 The device is up.
*       NU_FALSE                The device is not up.
*       NU_INVALID_PARM         The name passed in is NULL.
*
****************************************************************************/
STATUS DEV_Device_Up(const CHAR *if_name)
{
    DV_DEVICE_ENTRY *device;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    device = DEV_Get_Dev_By_Name(if_name);

    if (device == NU_NULL)
        status = NU_INVALID_PARM;

    else if (device->dev_flags & DV_UP)
        status = NU_TRUE;

    else
        status = NU_FALSE;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* DEV_Device_Up */





