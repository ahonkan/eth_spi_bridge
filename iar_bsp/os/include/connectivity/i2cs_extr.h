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
*       i2cs_extr.h
*
* COMPONENT
*
*       I2CS - I2C Slave
*
* DESCRIPTION
*
*       This file contains the function prototypes for Nucleus I2C
*       slave.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2cs_defs.h                         Main definition file for
*                                           Nucleus I2C slave.
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2CS_EXTR_H
#define     I2CS_EXTR_H

#include    "connectivity/i2cs_defs.h"

STATUS  I2CSC_Response_to_Read  (I2C_HANDLE     i2c_handle,
                                 UINT8         *data,
                                 UNSIGNED_INT   length);

/* Function prototypes for local slave address getting/setting. */
STATUS  I2CSS_Slave_Get_Address (I2C_HANDLE     i2c_handle,
                                 UINT16        *address,
                                 UINT8         *address_type);
STATUS  I2CSS_Slave_Set_Address (I2C_HANDLE     i2c_handle,
                                 UINT16         slave_address,
                                 UINT8          address_type);

#endif      /* !I2CS_EXTR_H */
