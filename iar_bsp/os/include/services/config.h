/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       config.h
*
* COMPONENT
*
*       PX - POSIX
*
* DESCRIPTION
*
*       This file contains the various configurations for Nucleus POSIX.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       nucleus_gen_cfg.h - Nucleus OS configuration values.
*
*************************************************************************/

#ifndef NU_PSX_CONFIG_H
#define NU_PSX_CONFIG_H

#include "nucleus_gen_cfg.h"

/* Nucleus POSIX version number */
#define         POSIX_1_17             1
#define         POSIX_1_18             2
#define         POSIX_1_19             3
#define         POSIX_1_110            4
#define         POSIX_VERSION_COMP     POSIX_1_19

/* NOTE: Do NOT change these values manually.  The macro values here are
   set via the configuration options defined through the Nucleus build and
   configuration system.  Please use the Nucleus build and configuration
   system to adjust these values. */

/* Maximum value of system stack size which can be reserved for the task
   stack.  */
#define     POSIX_THREAD_STACKSIZE_MAX      CFG_NU_OS_SVCS_POSIX_CORE_THREAD_STACK_SIZE_MAX

/* Include support for POSIX devices (file system, networking, stdin)
   within the POSIX system. */
#ifdef CFG_NU_OS_SVCS_POSIX_FS_ENABLE
#define     POSIX_INCLUDE_DEVICES           1
#else
#define     POSIX_INCLUDE_DEVICES           0
#endif /* CFG_NU_OS_SVCS_POSIX_FS_ENABLE */
           

/* Include Asynchronous I/O support in POSIX system. */
#ifdef CFG_NU_OS_SVCS_POSIX_AIO_ENABLE
#define     POSIX_INCLUDE_AIO               1
#else
#define     POSIX_INCLUDE_AIO               0
#endif /* CFG_NU_OS_SVCS_POSIX_AIO_ENABLE */

/* Include Networking support in the POSIX system. */
#ifdef CFG_NU_OS_SVCS_POSIX_NET_ENABLE
#define     POSIX_INCLUDE_NET               1
#else
#define     POSIX_INCLUDE_NET               0
#endif /* CFG_NU_OS_SVCS_POSIX_NET_ENABLE */

/* Include support for the Nucleus Dynamic Down-Load (DDL) system. */
#define     POSIX_INCLUDE_DDL               CFG_NU_OS_SVCS_POSIX_CORE_INCLUDE_DDL_SUPPORT

/* Disables error checking in the Nucleus POSIX system. */
#define     POSIX_NO_ERROR_CHECKING         !CFG_NU_OS_SVCS_POSIX_CORE_ERROR_CHECKING

/* Enables support for serial I/O within the POSIX system.  Note: No
   serial I/O means stdin, stdout, stderr will be redirected to file
   system. */
#define     POSIX_IO                        CFG_NU_OS_SVCS_POSIX_CORE_SERIAL_IO_ENABLED

/* The maximum number of device descriptors that can be opened */
#define     DEV_OPEN_MAX                    CFG_NU_OS_SVCS_POSIX_CORE_DEVICES_OPEN_MAX

/* The maximum number of devices present in the POSIX system. The devices
   are the following: Null-driver, Serial driver and Ethernet driver (Only
   when support for POSIX NET is included). */
#define     MAX_NO_DEVICES                  CFG_NU_OS_SVCS_POSIX_CORE_DEVICES_MAX

/* Settings for the Asynchronous I/O support thread.  */
#define     DEFAULT_AIO_THREAD_PRIORITY     CFG_NU_OS_SVCS_POSIX_AIO_THREAD_PRIORITY
#define     DEFAULT_AIO_THREAD_STACK        CFG_NU_OS_SVCS_POSIX_AIO_THREAD_STACK_SIZE

/* Set this macro to set up the default choice of drive to be mounted. */
#define     POSIX_DRIVE                     CFG_NU_OS_SVCS_POSIX_FS_DEFAULT_DRIVE

/* Check Stack Enabled - This macro determines if the thread stack
   checking functionality is compiled into the POSIX implementation.  The
   checking supports the stack guard area concept, but may add overhead to
   POSIX API calls. */
#define     POSIX_CHECK_STACK_ENABLED       CFG_NU_OS_SVCS_POSIX_CORE_CHECK_STACK_ENABLED

#endif /* NU_PSX_CONFIG_H */
