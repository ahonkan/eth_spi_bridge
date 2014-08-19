/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
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
*       ftp_cfg.h                                      
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP client/server
*       configuration.
*
*   DESCRIPTION
*
*       This file contains configuration definitions for the FTP client and
*       FTP server. No functions are contained in this file. Only data
*       structures to be customized by the application developer are
*       included.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*
*************************************************************************/

#ifndef FTP_CFG_H
#define FTP_CFG_H

#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/**************************************************************************
                          FTP CLIENT CONFIGURABLES
**************************************************************************/

/* Function definition Macro's */
/* This macro returns the current year, You can define this macro to be
   your own function that return the current year */
#define FTP_GET_CURRENT_YEAR             2006

#define FTPC_REPLY_BUFFER_SIZE           512
#define FTPC_GENERIC_BUFF_SIZE           256
#define FTPC_TRANSFER_BUFF_SIZE          1024

/* number of seconds to wait for expected response from the server,
   on either the control connection or the data connection, before
   closing the session */
#define FTPC_INACT_TIMEOUT              (300 * TICKS_PER_SECOND)

/* number of seconds to wait after attempting to close the control
   connection before terminating the connection and reporting an error */
#define FTPC_CCONN_TIMEOUT              (30 * TICKS_PER_SECOND)

/* number of seconds to wait after attempting to send or receive data
   before the connection is closed and an error is reported */
#define FTPC_DATACT_TIMEOUT             (300 * TICKS_PER_SECOND)

#define FTPC_MAX_RECV_PACKET             1500

/**************************************************************************
                          FTP SERVER CONFIGURABLES
**************************************************************************/

/* Define FTP_USE_ANONYMOUS if you want to enable the use of anonymous
   FTP connections. The password for anonymous users is currently an
   email address. No checking is implemented other than the form of
   a standard email address. Anonymous FTP is enabled by default(NU_TRUE). */
#define FTP_USE_ANONYMOUS               NU_TRUE

/* Set the maximum length of a username and password as defined in the
   following FTP_Password_List[]. */
#define FTP_MAX_ID_LENGTH               32
#define FTP_MAX_PW_LENGTH               32

#define FTPS_COMMAND_BUFFER_SIZE        8
#define FTPS_REPLY_BUFFER_SIZE          1460
#define FTPS_GENERIC_BUFF_SIZE          256

#define FTPS_TRANSFER_BUFF_SIZE         512

/* Nucleus Run-level Initialization timeout for FTP Server */
#define FTPS_RUNLEVEL_WAIT_TIMEOUT      NU_SUSPEND

/* Amount of time the FTP server will wait for the underlying File System
   to come up before it aborts the initialization */
#define FTPS_INIT_TIMEOUT               NU_SUSPEND

/* number of seconds the server will wait until it closes a control
   connection for which there is no activity */
#define FTPS_INACT_TIMEOUT              (300 * TICKS_PER_SECOND)

/* number of seconds the server will wait after attempting to send
   or receive connection related data like commands before
   terminating the connection */
#define FTPS_DATA_TIMEOUT               (300 * TICKS_PER_SECOND)

/* number of seconds the server will wait after attempting to send
   or receive file transfer related data before terminating
   the connection */
#define FTPS_DATA_TASK_TIMEOUT          (30 * TICKS_PER_SECOND)

/* Connection tasks related values */
#define CLEANER_QUEUE_SIZE              10
#define FTPS_SERVER_DEFAULT_PORT        21
#define FTPS_SERVER_MAX_PENDING         10 /* 5 is the typical max value */
#define FTPS_MASTER_STACK_SIZE          5500
#define FTPS_CLEANER_STACK_SIZE         2500
#define FTPS_CONTROL_STACK_SIZE         5500
#define FTPS_DATA_STACK_SIZE            4500
#define MAX_FTPS_CONNECTIONS            120 /* max. TCP/IP connects is 120 */

/* Connection priority  time slice defines */
#define FTPS_MASTER_PRIORITY            CFG_NU_OS_NET_PROT_FTP_MASTER_TASK_PRIORITY
#define FTPS_CONTROL_PRIORITY           FTPS_MASTER_PRIORITY + 2
#define FTPS_DATA_PRIORITY              FTPS_MASTER_PRIORITY + 4
#define DATA_TIME_SLICE                 (TICKS_PER_SECOND >> 2) /* 1/4th of a second */

#if (FTPS_REPLY_BUFFER_SIZE < 1460)
#error FTPS_REPLY_BUFFER_SIZE must be atleast 1460
#endif

/* Default drive for file system */
#define FTPS_DEFAULT_DRIVE              CFG_NU_OS_NET_PROT_FTP_DEFAULT_DRIVE

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
