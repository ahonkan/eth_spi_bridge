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
*       i2cm_defs.h
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains constants definitions for Nucleus I2C master.
*
* DATA STRUCTURES
*
*       I2C_MTR_QUEUE                       Data structure for handling
*                                           multi transfer requests.
*
* DEPENDENCIES
*
*       i2c.h                               Nucleus I2C main definition
*                                           and API file.
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2CM_DEFS_H
#define     I2CM_DEFS_H

#include    "connectivity/i2c.h"

/* Constant to specify general call address. */
#define     I2CM_GENERAL_CALL_ADDRESS       0x00

/* Code to specify the general call functionality for programming remote
   slave address. */
#define     I2CM_GC_PROGRAM_ADDRESS         0x04

/* Code to specify the general call functionality for programming remote
   slave address and resetting it. */
#define     I2CM_GC_RESET_PROGRAM_ADDRESS   0x06

/* Minimum number of slaves for multi transfer operation. */
#define     I2CM_MIN_SLAVES_FOR_MTR         2

/* Data structure for handling multi transfer requests. */
typedef struct I2C_MTR_QUEUE_STRUCT
{
    /* Pointer to the user array containing the slave addresses. */
    I2C_NODE        *i2c_slave_address;

    /* Pointer to the user array containing data length for each slave. */
    UNSIGNED_INT    *i2c_data_length;

    /* Pointer to the user data buffer containing transmission
       data for slaves. */
    UINT8           *i2c_slave_tx_data;

    /* Pointer to the user data buffer that will contain reception data
       for slaves. */
    UINT8           *i2c_slave_rx_data;

    /* Pointer to the user array containing read/write flag for each
       slave. */
    UINT8           *i2c_read_write;

    /* Total number of slaves to which transfers are to be processed.
       It is essentially the length for each array. */
    UINT8           i2c_slave_count;

    /* Padding to align the structure. */
    UINT8           i2c_padding[3];

} I2C_MTR_QUEUE;

#endif      /* !I2CM_DEFS_H */
