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
*       i2c_extern.h
*
* COMPONENT
*
*       Nucleus I2C
*
* DESCRIPTION
*
*       This file provides interface to Nucleus I2C hardware driver.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2cs_defs.h                         Constants for Nucleus I2C
*                                           slave component.
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C common services
*                                           component.
*
*       i2cbm_extr.h                        Function prototypes for
*                                           Nucleus I2C buffer management.
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/

/* Check to see if the file has been included already. */
#ifndef     I2C_EXTERN_H
#define     I2C_EXTERN_H

/* Define to make sure that Nucleus I2C hardware driver is able to access
   the internal services of Nucleus I2C. */
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cs_defs.h"
#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2cbm_extr.h"
#include    "connectivity/i2cm_extr.h"

#define     I2C_MORE_TRANSFER_LEFT(i2c_cb)  \
            i2c_cb->i2c_mtr_queue.i2c_slave_count > 1

#endif      /* !I2C_EXTERN_H */
