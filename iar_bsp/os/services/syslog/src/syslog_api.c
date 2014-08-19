/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       syslog_api.c
*
*   DESCRIPTION
*
*       This file contains the system logging API for Nucleus.
*
*   FUNCTIONS
*
*       SysLogOpen
*       SysLogSetMask
*       SysLog
*       SysLogClose
*
***********************************************************************/

/* Include necessary services */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/syslog_extern.h"

/* Local variables */
static  DV_IOCTL0_STRUCT        SysLog_IOCTL0;


/***********************************************************************
*
*   FUNCTION
*
*       SysLogOpen
*
*   DESCRIPTION
*
*       This function opens the system logger and returns a unique
*       handle to the caller
*
*   CALLED BY
*
*       Middleware
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       name                                Component name (output as
*                                           part of the logged data)
*
*   OUTPUTS
*
*       handle != SYSLOG_INVALID_HANDLE     System logging driver open
*       handle == SYSLOG_INVALID_HANDLE     System logging driver not available
*
***********************************************************************/
SYSLOG_HANDLE   SysLogOpen(const char * name)
{
    SYSLOG_HANDLE       handle;
    STATUS              status;
    DV_DEV_LABEL        label[1] = {{SYSLOGGER_CLASS_LABEL}};
    
    
    /* First, try opening the sys logger device */
    status = DVC_Dev_Open (label, (DV_DEV_HANDLE *)&handle);
    
    /* Check to see if device opened */
    if (status == NU_SUCCESS)
    {
        /* Successfully opened - get IOCTL base of device */
        status = DVC_Dev_Ioctl ((DV_DEV_HANDLE)handle,
                                DV_IOCTL0, 
                                (VOID *)&SysLog_IOCTL0, sizeof(DV_IOCTL0_STRUCT));
                                
        /* Ensure base successfully returned */
        if (status == NU_SUCCESS)
        {
            /* Pass component logging name to device via IOCTL */
            status = DVC_Dev_Ioctl ((DV_DEV_HANDLE)handle,
                                    SysLog_IOCTL0.base + SYSLOG_SET_NAME_CMD, 
                                    (VOID *)name, strlen(name));
        }
    }
    
    /* Check for failures */
    if (status != NU_SUCCESS)
    {
        /* Just return an appropriate "invalid" handle for failure cases - allowed by other syslog APIs */
        handle = SYSLOG_INVALID_HANDLE;
    }
    
    /* Return handle to caller */
    return (handle);
}


/***********************************************************************
*
*   FUNCTION
*
*       SysLogSetMask
*
*   DESCRIPTION
*
*       Set mask for a given handle to allow filtering of logged messages.
*
*   CALLED BY
*
*       Middleware
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       handle                              Logging session handle returned from SysLogOpen
*       logmask                             32-bit mask applied against logged data
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID            SysLogSetMask(SYSLOG_HANDLE handle, UINT32 logmask)
{
    /* Check for invalid handle */
    if (handle != SYSLOG_INVALID_HANDLE)
    {
        /* Set logging mask via IOCTL */
        DVC_Dev_Ioctl ((DV_DEV_HANDLE)handle,
                       SysLog_IOCTL0.base + SYSLOG_SET_MASK_CMD, 
                       (VOID *)&logmask, sizeof(UINT32));
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       SysLog
*
*   DESCRIPTION
*
*       Allows logging of data for given session handle
*
*   CALLED BY
*
*       Middleware
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       handle                              Logging session handle returned by SysLogOpen
*       msg                                 Pointer NULL terminated string to log
*       loglevel                            32-bit value used to facilitate filtering (using logmask)
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID            SysLog(SYSLOG_HANDLE handle, const char * msg, UINT32 loglevel)
{
    UINT32  bytes_written;
    

    /* Check for invalid handle */
    if (handle != SYSLOG_INVALID_HANDLE)
    {
        /* Write string to device - pass loglevel in size
           (NULL terminated string being written - don't need size) */
        DVC_Dev_Write((DV_DEV_HANDLE)handle, (VOID *)msg, loglevel, 0, &bytes_written);
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       SysLogClose
*
*   DESCRIPTION
*
*       Closes the specified logging session
*
*   CALLED BY
*
*       Middlware
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       handle                              Handle returned by SysLogOpen
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID            SysLogClose(SYSLOG_HANDLE handle)
{
    /* Check for invalid handle */
    if (handle != SYSLOG_INVALID_HANDLE)
    {
        /* Close this device session */
        DVC_Dev_Close((DV_DEV_HANDLE)handle);
    }
}
