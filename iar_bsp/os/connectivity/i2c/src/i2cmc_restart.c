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
*       i2cmc_restart.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the service that provides the facility of
*       sending RESTART signal from an I2C master node.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Restart                       Restarts the transfer for
*                                           communication with another
*                                           slave.
*
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
*       I2CMC_Restart
*
* DESCRIPTION
*
*       This API function transmits the address of the slave on the I2C
*       bus for initiating a read/write restart operation without
*       releasing the bus. Either the same slave may be accessed in a
*       different mode (read/write) or another slave may be addressed
*       but the bus is not released during this process.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle
*
*       slave                               Node structure containing
*                                           node address details.
*
*       rw                                  Read/Write
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_ADDRESS_TX_FAILED               Master could not address the
*                                           slave node perhaps due to the
*                                           reason that it lost
*                                           arbitration.
*
*       I2C_BUS_BUSY                        I2C bus is busy.
*
*       I2C_INVALID_ADDRESS_TYPE            Type of the slave address is
*                                           neither 7-bit nor 10-bit.
*
*       I2C_NODE_NOT_MASTER                 The node is not master.
*
*       I2C_SLAVE_NOT_ACKED                 Slave didn't acknowledge.
*
*       I2C_SLAVE_TIME_OUT                  Slave timed out.
*
*       I2C_TRANSFER_IN_PROGRESS            An I2C transfer is already
*                                           in progress.
*
*************************************************************************/
STATUS  I2CMC_Restart        (I2C_HANDLE i2c_handle,
                              I2C_NODE   slave,
                              UINT8      rw)
{
    I2C_CB         *i2c_cb;
    
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
    /* Check for valid parameters. */
    NU_I2C_VALCHK_RETURN(((rw == I2C_READ) || (rw == I2C_WRITE)));
    
#if ( NU_I2C_ERROR_CHECKING )
    /* Check slave address and slave address type. */
    status = I2CS_Check_Slave_Address(slave);
    if ( status == NU_SUCCESS )
#endif
    {
        /* Check if device handle is valid and get the port control block
           and device control block. */
         I2CS_Get_CB(i2c_handle, &i2c_cb);
    
        /* Set node state to idle. */
        i2c_cb->i2c_node_state = I2C_NODE_IDLE;
    
        /* Send the address on the bus. */
        status = I2CMC_Send_Start(i2c_handle, slave,
                                  rw, (BOOLEAN)NU_TRUE);
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
#endif /* #if ( NU_I2C_SUPPORT_FINE_CONTROL_API ) */
