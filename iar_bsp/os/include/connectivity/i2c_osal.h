/*************************************************************************
*
*                  Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       i2c_osal.h
*
* COMPONENT
*
*       I2C OSAL - I2C OS Abstraction
*
* DESCRIPTION
*
*       This file contains constant definitions and structure declarations
*       to provide minimal OS abstraction for various services of
*       Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2c_cfg.h                           Nucleus I2C configuration
*                                           file.
*       nucleus.h                           Main definition and API file
*                                           for Nucleus PLUS.
*       nu_kernel.h                         Kernel definitions
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2C_OSAL_H
#define     I2C_OSAL_H

#include    "connectivity/i2c_cfg.h"
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"

/* Maximum number of i2c sessions/instances */
#define     I2C_MAX_INSTANCES               CFG_NU_OS_CONN_I2C_MAX_DEVS_SUPPORTED

#define     I2C_LABEL                       {0xd1,0x5b,0x75,0xff,0x61,0x8f,0x45,0xf1,0xaa,0xfc,0x92,0x76,0x2f,0x6f,0x46,0x8e}

#define     I2C_MODE_IOCTL_BASE             0
#define     I2C_DEV_REG_PATH_LENGTH         200

/* Mapping to provide abstraction for memory allocation. */

#define     I2C_HANDLER_MEM_STACK           (NU_MIN_STACK_SIZE * 2)
#define     I2C_HANDLER_PRIORITY            0

/* Extern the multithread protection structure here so that it
   does not need externing in all the files using the above macros. */
extern      NU_PROTECT                      I2C_Protect_Struct;

#if         (PLUS_VERSION_COMP > PLUS_1_15)

/* Define to incorporate ESAL changes in Nucleus I2C Driver. */

#undef      I2C_HANDLER_MEM_STACK
#define     I2C_HANDLER_MEM_STACK           NU_MIN_STACK_SIZE

#endif      /* PLUS_VERSION_COMP > PLUS_1_15 */

#endif      /* !I2C_OSAL_H */
