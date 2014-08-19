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
*       i2cmc_read.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the service that allows the master to read
*       unlimited number of data bytes from a slave node.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Read                          Reads data from a specified
*                                           slave.
*
*       I2CMC_Process_Read                  Processes read request.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CMC_Read
*
* DESCRIPTION
*
*       This API function provides the user with a single interface to
*       read data from a slave.
*
* INPUTS
*
*       i2c_handle                             Nucleus I2C device handle.
*
*       slave                               Node structure containing node
*                                           address details.
*
*      *data                                Data pointer where data will
*                                           be returned.
*
*       length                              Number of data bytes to read.
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
*       I2C_DATA_RECEPTION_FAILED           Data reception failed.
*
*       I2C_INVALID_ADDRESS_TYPE            Type of the slave address is
*                                           neither 7-bit nor 10-bit.
*
*       I2C_INVALID_DRIVER_MODE             Driver mode is not set to
*                                           either interrupt driven or
*                                           polling mode.
*
*       I2C_INVALID_PARAM_POINTER           Null given instead of a
*                                           variable pointer.
*
*       I2C_NODE_NOT_MASTER                 The node is not master.
*
*       I2C_RX_BUFFER_NOT_ENOUGH            Insufficient receive buffer
*                                           memory space.
*
*       I2C_SLAVE_NOT_ACKED                 Slave did not acknowledge.
*
*       I2C_SLAVE_TIME_OUT                  Slave timed out.
*
*       I2C_TRANSFER_IN_PROGRESS            An I2C transfer is already
*                                           in progress.
*
*       I2C_TRANSFER_STOP_FAILED            Transfer on I2C bus could not
*                                           be stopped.
*
*************************************************************************/
STATUS  I2CMC_Read          (I2C_HANDLE    i2c_handle,
                             I2C_NODE      slave,
                             UINT8        *data,
                             UNSIGNED_INT  length)
{
    I2C_CB         *i2c_cb;
   
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
    /* Check for valid parameters. */
    NU_I2C_PTRCHK_RETURN(data);
    NU_I2C_VALCHK_RETURN(length != 0);

#if ( NU_I2C_ERROR_CHECKING )
    /* Check slave address and slave address type. */
    status = I2CS_Check_Slave_Address(slave);
    if ( status == NU_SUCCESS )
#endif
    {
        /* Check if device handle is valid and get the port control block
           and device control block. */
         I2CS_Get_CB(i2c_handle, &i2c_cb);
    
        /* Set API mode to automatic API. */
        i2c_cb->i2c_api_mode = I2C_AUTOMATIC_RX_API;
    
        /* Process the I2C read operation. */
        status = I2CMC_Process_Read(i2c_handle, slave,
                                    data, length, (BOOLEAN)NU_FALSE);
    }
    
    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMC_Process_Read
*
* DESCRIPTION
*
*       This API function provides the user with a single interface to
*       read data from a slave.
*
* INPUTS
*
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*       slave                               Node structure containing node
*                                           address details.
*
*      *data                                Data pointer where data will
*                                           be returned.
*
*       length                              Number of data bytes to read.
*
*       restart                             Whether the read operation
*                                           will follow a RESTART signal.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_ADDRESS_TX_FAILED               Master could not address the
*                                           slave node perhaps due to the
*                                           reason that it lost
*                                           arbitration.
*
*       I2C_BUS_BUSY                        I2C bus is busy.
*
*       I2C_DATA_RECEPTION_FAILED           Data reception failed.
*
*       I2C_INVALID_ADDRESS_TYPE            Type of the slave address is
*                                           neither 7-bit nor 10-bit.
*
*       I2C_INVALID_DRIVER_MODE             Driver mode is not set to
*                                           either interrupt driven or
*                                           polling mode.
*
*       I2C_INVALID_PARAM_POINTER           Null given instead of a
*                                           variable pointer.
*
*       I2C_NODE_NOT_MASTER                 The node is not master.
*
*       I2C_SLAVE_NOT_ACKED                 Slave did not acknowledge.
*
*       I2C_SLAVE_TIME_OUT                  Slave timed out.
*
*       I2C_TRANSFER_IN_PROGRESS            An I2C transfer is already
*                                           in progress.
*
*       I2C_TRANSFER_STOP_FAILED            Transfer on I2C bus could not
*                                           be stopped.
*
*************************************************************************/
STATUS I2CMC_Process_Read   (
                             I2C_HANDLE    i2c_handle,
                             I2C_NODE      slave,
                             UINT8        *data,
                             UNSIGNED_INT  length,
                             BOOLEAN       restart)
{
    STATUS       status;
    I2C_CB      *i2c_cb;
    
    I2CS_Get_CB(i2c_handle, &i2c_cb);
    
    /* Set status to indicate that I2C operating mode is not
       set properly. */
    status = I2C_INVALID_DRIVER_MODE;

    /* Store the length of the transfer in the transfer size field. */
    i2c_cb->i2c_transfer_size = length;
    
#if ( NU_I2C_SUPPORT_POLLING_MODE )

    if (data != NU_NULL)
    {
        STATUS      stop_transfer_status;

        /* Initiate the transfer on the network by sending the
           slave address. */
        status = I2CMC_Send_Start(i2c_handle, slave,
                                  I2C_READ, restart);

        /* Check if transfer initiated successfully. */
        if (status == NU_SUCCESS)
        {
            UINT8          *rx_buffer = data;
            UNSIGNED_INT    i = length;

            /* Set node as receiver. */
            status = I2CC_Ioctl_Driver
                            (i2c_handle, I2C_SET_NODE_MODE_RX, &length);

            /* Handle single byte transfer. */
            if (length == 1)
            {
                /* Send NACK to make sure that transfer ends at 1 byte. */
                status = I2CC_Ioctl_Driver
                    (i2c_handle, I2C_SEND_NACK, NU_NULL);
            }
            
            /* Receive all the data bytes from the slave. */
            while(i--)
            {
                /* Get data byte from the slave transmitter. */
                        status = DVC_Dev_Read(i2c_cb->i2c_dv_handle, 
                        rx_buffer, 1 /* num bytes */, 
                        0, /* offset, not used */
                        NU_NULL /* bytes_read_ptr , not used */);


                /* Check if byte received successfully. */
                if (status == NU_SUCCESS)
                {
                    /* Increment the receive data pointer to point
                       to the next memory location. */
                    rx_buffer++;

                    /* Check if one byte is left to be read. */
                    if (i > 1)
                    {
                        /* Send acknowledgment of data to the slave. */
                        status = I2CC_Ioctl_Driver
                            (i2c_handle, I2C_SEND_ACK, NU_NULL);
                    }

                    /* Check if no more bytes are to be read. */
                    else if (i == 1)
                    {
                        /* Send acknowledgment of data to the slave. */
                        status = I2CC_Ioctl_Driver
                            (i2c_handle, I2C_SEND_NACK, NU_NULL);
                    }

                    else
                    {
                        /* Empty else for MISRA standard compliance. */
                    }
                }
            }

            /* Check if there is any error in data reception. */
            if (status != NU_SUCCESS)
            {
                status = I2C_DATA_RECEPTION_FAILED;
            }
        }

        /* Check if it is multi transfer and another slave needs to
           be processed. */
        if (i2c_cb->i2c_mtr_queue.i2c_slave_count > 1)
        {
            status = I2CMC_Process_Next_Slave(i2c_handle);
        }

        else
        {
            /* Make sure the bus is released. */
            stop_transfer_status = I2CMC_Send_Stop(i2c_handle);

            /* Check if bus released/transfer completed successfully. */
            if (stop_transfer_status != NU_SUCCESS)
            {
                status  = I2C_TRANSFER_STOP_FAILED;
            }
        }
    }

    else
    {
        /* Set status to indicate invalid data pointer. */
        status = I2C_INVALID_PARAM_POINTER;
    }
    
#else

    /* Check if no transfer is in progress for the node. */
    if (i2c_cb->i2c_node_state == I2C_NODE_IDLE)
    {
        /* Check for the valid length of the data. */
        if (length <= i2c_cb->i2c_io_buffer.i2cbm_rx_buffer_size)
        {
            /* Set the number of bytes to receive. */
            i2c_cb->i2c_io_buffer.i2cbm_bytes_to_receive = length;

            /* Record the slave address for the current transfer. */
            i2c_cb->i2c_io_buffer.i2cbm_slave_address = 
                                             slave.i2c_slave_address;

            /* Initiate the transfer on the network by sending the
               slave address. */
            status = I2CMC_Send_Start(i2c_handle, slave,
                                      (BOOLEAN)I2C_READ, restart);
        }

        else
        {
            /* Set the status to indicate that the given data length
               is more than the buffer can handle. */
            status = I2C_RX_BUFFER_NOT_ENOUGH;
        }
    }

    /* Node is not idle. */
    else
    {
        /* Set status to indicate that a transfer operation
           (read/write) is already in progress for the node. */
        status = I2C_TRANSFER_IN_PROGRESS;
    }

    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(data);

#endif      /* #if ( NU_I2C_SUPPORT_POLLING_MODE ) */

    /* Return the completion status of the service. */
    return (status);
}
