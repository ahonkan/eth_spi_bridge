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
*       i2cbm_extr.h
*
* COMPONENT
*
*       I2CBM - I2C Buffer Management
*
* DESCRIPTION
*
*       This file contains the function prototypes for Nucleus I2C I/O
*       buffer management.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2c_defs.h                          Main internal definition file
*                                           for Nucleus I2C.
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2CBM_EXTR_H
#define     I2CBM_EXTR_H

#include    "connectivity/i2c_defs.h"

/* Function prototypes for using Nucleus I2C I/O buffers. */

STATUS  I2CBM_Put_Output_Buffer (I2C_CB *i2c_cb, UINT8 *data,
                                 UNSIGNED_INT length);
STATUS  I2CBM_Get_Output_Buffer (I2C_CB *i2c_cb, UINT8 *data);
STATUS  I2CBM_Put_Input_Buffer  (I2C_CB *i2c_cb, UINT8  data);
STATUS  I2CBM_Get_Input_Buffer  (I2C_CB *i2c_cb, UINT8 *data,
                                     UNSIGNED_INT length);

#endif      /* !I2CBM_EXTR_H */
