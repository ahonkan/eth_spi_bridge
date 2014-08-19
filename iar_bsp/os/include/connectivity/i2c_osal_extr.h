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
*       i2c_osal_extr.h
*
* COMPONENT
*
*       I2C OSAL - I2C OS Abstraction
*
* DESCRIPTION
*
*       This file contains the function prototypes to provide OS
*       abstraction for Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2c_osal.h                          Main definition file for
*                                           providing OS abstraction for
*                                           Nucleus I2C services.
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2C_OSAL_EXTR_H
#define     I2C_OSAL_EXTR_H

#include    "connectivity/i2c_osal.h"

/* Function prototype for allocating the OS and system resources. */
STATUS  I2C_OSAL_Allocate_Resources   (I2C_INIT *i2c_init,
                                           I2C_CB  *i2c_cb);

/* Function prototype for deallocating the OS and system resources. */
STATUS  I2C_OSAL_Deallocate_Resources (I2C_CB   *i2c_cb);

/* Function prototype for creating Nucleus I2C notification handler. */
STATUS  I2C_OSAL_Create_Handler       (NU_MEMORY_POOL *mem);

#endif      /* I2C_OSAL_EXTR_H */
