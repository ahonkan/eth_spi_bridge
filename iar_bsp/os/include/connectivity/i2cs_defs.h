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
*       i2cs_defs.h
*
* COMPONENT
*
*       I2CS - I2C Slave
*
* DESCRIPTION
*
*       This file contains the constants definition for slave
*       functionality of Nucleus I2C.
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
#ifndef     I2CS_DEFS_H
#define     I2CS_DEFS_H

#include    "connectivity/i2c_defs.h"

/* Dummy data for Nucleus I2C. */
#define     I2C_DUMMY_DATA          0xFF

#endif      /* !I2CS_DEFS_H */
