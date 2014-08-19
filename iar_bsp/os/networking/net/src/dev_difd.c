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
*       dev_difd.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Detach_IP_From_Device
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Detach_IP_From_Device
*       DEV_Detach_IP_From_Device
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Detach_IP_From_Device
*
*   DESCRIPTION
*
*       This function brings the device specified in name down.
*
*   INPUTS
*
*       *name                   A pointer to the name of the device.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         The device does not exist in the system.
*
*************************************************************************/
STATUS NU_Detach_IP_From_Device(const CHAR *name)
{
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* We must grab the NET semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    /* Remove all IP addresses from the device. */
    status = DEV_Detach_IP_From_Device(name);

    /* Delete the device from the cache. */
    NU_Ifconfig_Delete_Interface(name);

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Trace log */
    T_DETACH_IP((char*)name, status);

    NU_USER_MODE();

    return (status);

} /* NU_Detach_IP_From_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Detach_IP_From_Device
*
*   DESCRIPTION
*
*       This function brings the device specified in name down.
*
*   INPUTS
*
*       *name                   A pointer to the name of the device.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         The device does not exist in the system.
*
*************************************************************************/
STATUS DEV_Detach_IP_From_Device(const CHAR *name)
{
    STATUS          status = NU_SUCCESS;
    DV_DEVICE_ENTRY *dev;

    /* Get the device using the IP Address */
    dev = DEV_Get_Dev_By_Name(name);

    if (dev == NU_NULL)
        status = NU_INVALID_PARM;

    else
    {
        DEV_Detach_Addrs_From_Device(dev);

        /* Indicate that the device is not up and running. */
        dev->dev_flags &= ~DV_UP;

        /* Trace log */
        T_DEV_UP_STATUS((char*)name, 1);

        /* Notify the sockets layer to resume all tasks using this
         * interface and return an error to the user.
         */
        DEV_Resume_All_Open_Sockets();
    }

    return (status);

} /* DEV_Detach_IP_From_Device */
