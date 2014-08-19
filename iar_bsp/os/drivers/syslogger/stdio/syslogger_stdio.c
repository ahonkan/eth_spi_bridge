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
*       syslogger_stdio.c
*
*   DESCRIPTION
*
*       This file contains function that allow a simple but system level
*       logger driver that logs to standard I/O.
*
*       NOTE:  This component does little to no error checking - it
*              expects the system logger driver to only call the
*              provided services with correct data and at appropriate
*              times (ie error checking is done by system logger driver)
*
*   FUNCTIONS
*
*       SysLogger_Open_Medium
*       SysLogger_Close_Medium
*       SysLogger_Write_Medium
*       SysLogger_Read_Medium
*
***********************************************************************/

/* Include required header files */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/syslog_extern.h"
#include    "drivers/syslogger_defs.h"
#include    <stdio.h>


#if (CFG_NU_OS_DRVR_SYSLOGGER_MEDIUM == 0)
/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Open_Medium
*
*   DESCRIPTION
*
*       This function opens the standard I/O for logging (nothing to do)
*
*   INPUTS
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully opened
*       Other                               Logging medium unsuccessfully opened
*
*************************************************************************/
STATUS  SysLogger_Open_Medium(VOID)
{
    /* Return status to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Close_Medium
*
*   DESCRIPTION
*
*       This function closes standard I/O for logging (nothing to do)
*
*   INPUTS
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully closed
*       Other                               Logging medium unsuccessfully closed
*
*************************************************************************/
STATUS  SysLogger_Close_Medium(VOID)
{
    /* Return status of close */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Write_Medium
*
*   DESCRIPTION
*
*       This function logs data standard I/O
*
*   INPUTS
*       buf_ptr                             Pointer to buffer to log
*       size                                Size of buffer to log
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully written
*       Other                               Logging medium unsuccessfully written
*
*************************************************************************/
STATUS  SysLogger_Write_Medium(const CHAR * buf_ptr, UINT32 size)
{
    /* Write data to standard I/O */
    printf("%s",buf_ptr);
    
    /* Return status of write */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Read_Medium
*
*   DESCRIPTION
*
*       This function reads data from standard I/O
*
*       NOTE:  Currently not used by system logging service
*
*   INPUTS
*       buf_ptr                             Pointer to buffer to put read data
*       size                                Size of buffer to read
*       size_read                           Amount read from medium
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully read
*       Other                               Logging medium unsuccessfully read
*
*************************************************************************/
STATUS  SysLogger_Read_Medium(CHAR * buf_ptr, UINT32 size, UINT32 * size_read)
{
    /* Show zero bytes read */
    *size_read = 0;

    /* Return status of write */
    return (NU_SUCCESS);
}
#endif
