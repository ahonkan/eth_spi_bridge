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
*       i2cmc_bread.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the implementation of byte by byte reading
*       for Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Read_Byte                     Reads data byte from local
*                                           buffer.
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"

#if ( NU_I2C_SUPPORT_FINE_CONTROL_API )
/*************************************************************************
* FUNCTION
*
*       I2CMC_Read_Byte
*
* DESCRIPTION
*
*       This function reads a data byte from I2C slave.
*
* INPUTS
*
*       i2c_handle                             Nucleus I2C device handle.
*
*      *data                                Pointer to the location where
*                                           data will be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_DATA_RX_FAILED                  Data reception failed.
*
*************************************************************************/
STATUS  I2CMC_Read_Byte      (I2C_HANDLE i2c_handle, UINT8 *data)
{
    I2C_CB         *i2c_cb;
    
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
    /* Check for valid parameters. */
    NU_I2C_PTRCHK_RETURN(data);

    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Set API mode. */
    i2c_cb->i2c_api_mode = I2C_FINE_CONTROL_RX_API;

    /* Receive data byte from the slave. */
    status = DVC_Dev_Read(i2c_cb->i2c_dv_handle, 
                        data, 1 /* num bytes */, 
                        0, /* offset, not used */
                        NU_NULL /* bytes_read_ptr , not used */);

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
#endif /* #if ( NU_I2C_SUPPORT_FINE_CONTROL_API ) */
