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
*       i2cbm_defs.h
*
* COMPONENT
*
*       I2CBM - I2C Buffer Management
*
* DESCRIPTION
*
*       This file contains constants definitions and data structure
*       declarations for Nucleus I2C buffer management.
*
* DATA STRUCTURES
*
*       I2C_BUFFER                          Data structure for Nucleus I2C
*                                           buffer manipulation.
*
*       I2C_IO_BUFFER                       Data structure for Nucleus I2C
*                                           I/O buffer management.
*
* DEPENDENCIES
*
*       i2c.h                               Main definition and API file
*                                           for Nucleus I2C.
*
*************************************************************************/

/* Check to see if the file has been included already. */
#ifndef     I2CBM_DEFS_H
#define     I2CBM_DEFS_H

#include    "connectivity/i2c.h"

/* Data structure for Nucleus I2C buffer manipulation. */
typedef struct I2C_BUFFER_STRUCT
{
    UINT8          *i2cbm_data_start;       /* Data start pointer.      */
    UINT8          *i2cbm_data_read;        /* Data read pointer.       */
    UINT8          *i2cbm_data_write;       /* Data write pointer.      */
    UNSIGNED_INT    i2cbm_count;            /* Data bytes in buffer.    */

} I2C_BUFFER;

/* Data structure for Nucleus I2C I/O buffer management. */
typedef struct I2C_IO_BUFFER_STRUCT
{
    I2C_BUFFER      i2cbm_tx_buffer;        /* I2C transmission buffer. */
    I2C_BUFFER      i2cbm_rx_buffer;        /* I2C reception buffer.    */
    UNSIGNED_INT    i2cbm_tx_buffer_size;   /* Output buffer size in
                                               bytes.                   */
    UNSIGNED_INT    i2cbm_rx_buffer_size;   /* Input buffer size in
                                               bytes.                   */
    UNSIGNED_INT    i2cbm_bytes_to_receive; /* Number of data bytes to
                                               read from the slave.     */
    UNSIGNED_INT    i2cbm_error_counter;    /* Errors counter for the
                                               I2C buffer.              */
    UINT16          i2cbm_slave_address;    /* I2C slave address for the
                                               current transaction.     */
    UINT8           i2c_padding[2];         /* Padding to align the
                                               structure.               */
} I2C_IO_BUFFER;

#endif      /* !I2CBM_DEFS_H */
