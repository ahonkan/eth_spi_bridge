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
*       syslogger_defs.h
*
*   DESCRIPTION
*
*       This file contains internal defines, etc for the
*       system logger driver
*
*   DATA STRUCTURES
*
*       
*
*   DEPENDENCIES
*
*       
*
***********************************************************************/
#ifndef SYSLOGGER_DEFS_H
#define SYSLOGGER_DEFS_H

/* Define the maximum number of sessions available for the system logger driver */
#define SYSLOGGER_MAX_SESSIONS          30

/* Define maximum component length name */
#define SYSLOGGER_MAX_NAME_LENGTH       16

/* Define default logmask value (log everything) */
#define SYSLOGGER_DEFAULT_LOGMASK       0xFFFFFFFF

/* Define data structure for system logging sessions */
typedef struct
{
    INT         inuse;
    CHAR        name[SYSLOGGER_MAX_NAME_LENGTH];
    UINT32      logmask;
    
} SYSLOGGER_SESSION;

/* External function definitions */
extern STATUS  SysLogger_Open_Medium(VOID);
extern STATUS  SysLogger_Close_Medium(VOID);
extern STATUS  SysLogger_Write_Medium(const CHAR * buf_ptr, UINT32 size);
extern STATUS  SysLogger_Read_Medium(CHAR * buf_ptr, UINT32 size, UINT32 * size_read);

#endif  /* SYSLOG_DEFS_H */
