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
*       i2cmc_write.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the service for writing any amount of data
*       to a specified slave.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Write                         Writes data to a specified
*                                           slave.
*
*       I2CMC_Process_Write                 Processes write request.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*       i2cbm_extr.h                        Function prototypes for
*                                           Nucleus I2C buffer management.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"
#include    "connectivity/i2cbm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CMC_Write
*
* DESCRIPTION
*
*       This API function writes any specified amount of data to a
*       given slave.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       slave                               Node structure containing node
*                                           address details.
*
*      *data                                Data pointer.
*
*       length                              Number of data bytes.
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
*       I2C_DATA_TRANSMISSION_FAILED        Data transmission failed.
*
*       I2C_INVALID_ADDRESS_TYPE            Type of the slave address is
*                                           neither 7-bit nor 10-bit.
*
*       I2C_INVALID_DRIVER_MODE             Driver mode is not set to
*                                           either interrupt driven or
*                                           polling mode.
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
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS  I2CMC_Write         (I2C_HANDLE    i2c_handle,
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
        i2c_cb->i2c_api_mode = I2C_AUTOMATIC_TX_API;
    
        /* Process the I2C write operation. */
        status = I2CMC_Process_Write(i2c_handle, slave,
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
*       I2CMC_Process_Write
*
* DESCRIPTION
*
*       This API function writes any specified amount of data to a
*       given slave.
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
*      *data                                Data pointer.
*
*       length                              Number of data bytes.
*
*       restart                             Whether the write operation
*                                           will follow a RESTART signal.
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
*       I2C_DATA_TRANSMISSION_FAILED        Data transmission failed.
*
*       I2C_INVALID_ADDRESS_TYPE            Type of the slave address is
*                                           neither 7-bit nor 10-bit.
*
*       I2C_INVALID_DRIVER_MODE             Driver mode is not set to
*                                           either interrupt driven or
*                                           polling mode.
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
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS I2CMC_Process_Write  (
                             I2C_HANDLE i2c_handle,
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

        /* Initialize the transfer on the network and send the
           slave address. */
        status = I2CMC_Send_Start(i2c_handle, slave,
                                                      I2C_WRITE, restart);

        /* Check if transfer initiated successfully. */
        if (status == NU_SUCCESS)
        {
            UINT8          *tx_buffer = data;
            UNSIGNED_INT    i = length;

            /* Set node as receiver. */
            (VOID)I2CC_Ioctl_Driver
                                 (i2c_handle, I2C_SET_NODE_MODE_TX, NU_NULL);
           /* Send length */
            (VOID)I2CC_Ioctl_Driver
                                 (i2c_handle, I2C_SEND_TRANSFER_COUNT, (VOID*)length);

            /* Transmit all the data bytes to the slave. */
            while(i--)
            {
                /* Transmit data byte to the slave. */
                status = DVC_Dev_Write(i2c_cb->i2c_dv_handle,
                                tx_buffer,
                                1 /* num bytes */,
                                0, /* offset, not_used */
                                NU_NULL /* bytes_written_ptr, not used */);

                /* Check if data byte transmitted to slave
                   successfully. */
                if (status == NU_SUCCESS)
                {
                    /* Check if acknowledgment of the data received. */
                    status = I2CC_Ioctl_Driver
                                   (i2c_handle, I2C_CHECK_DATA_ACK, NU_NULL);

                    /* Check if acknowledgment received successfully. */
                    if (status == NU_SUCCESS)
                    {
                        /* Point to next byte for transmission. */
                        tx_buffer++;
                    }
                }
            }

            /* Check if all data hasn't been transmitted successfully. */
            if(status !=  NU_SUCCESS)
            {
                status = I2C_DATA_TRANSMISSION_FAILED;
            }

        }

        /* Check if it is multi transfer and another slave needs to be
           processed. */
        if(i2c_cb->i2c_mtr_queue.i2c_slave_count > 1)
        {
            /* Process the transfer with the next slave. */
            status = I2CMC_Process_Next_Slave(i2c_handle);
        }

        /* No more transfers left. */
        else
        {
            STATUS stop_transfer_status;

            /* Make sure the bus is released. */
            stop_transfer_status = I2CMC_Send_Stop(i2c_handle);

            /* Check if bus not released/transfer not completed
               successfully. */
            if(stop_transfer_status != NU_SUCCESS)
            {
                /* Set status to indicate that the transfer couldn't
                   be stopped successfully. */
                status  = I2C_TRANSFER_STOP_FAILED;
            }
        }

#else

    /* Check if no transfer is in progress for the node. */
    if (i2c_cb->i2c_node_state == I2C_NODE_IDLE)
    {
        /* Place the data in the output buffer. */
        status = I2CBM_Put_Output_Buffer(i2c_cb, data, length);

        /* Check if data is put correctly in the buffer. */
        if (status == NU_SUCCESS)
        {
            /* Record the slave address for the current transfer. */
            i2c_cb->i2c_io_buffer.i2cbm_slave_address = 
                                             slave.i2c_slave_address;

            /* Initialize the transfer on the network and send
               the slave address. */
            status = I2CMC_Send_Start(i2c_handle, slave,
                                      (UINT8)I2C_WRITE, restart);
        }
    }

    /* Node is not idle. */
    else
    {
        /* Set status to indicate that a transfer operation
           (read/write) is already in progress for the node. */
        status = I2C_TRANSFER_IN_PROGRESS;
    }

#endif      /* #if ( NU_I2C_SUPPORT_POLLING_MODE ) */

    /* Return the completion status of the service. */
    return (status);
}
