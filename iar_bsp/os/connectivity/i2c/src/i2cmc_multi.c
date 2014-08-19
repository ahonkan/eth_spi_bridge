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
*       i2cmc_multi.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the service to do read from or write to
*       multiple slave nodes or multiple times to the same slave node.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Multi_Transfer                Allows to read and write to
*                                           multiple nodes using restart.
*
*       I2CMC_Process_Transfer              Process the  complete I2C
*                                           read/write transfer.
*
*       I2CMC_Process_Next_Slave            Processes the transfer for the
*                                           next slave in multi transfer
*                                           request.
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
*       I2CMC_Multi_Transfer
*
* DESCRIPTION
*
*       This API function provides the user with the facility of reading/
*       writing to multiple slaves sequentially or changing the direction
*       of data for the same slave. It performs this function using the
*       RESTART service as described in I2C protocol. All the arrays
*       passed as parameters to this function must be declared in global
*       scope and none of them must be reutilized until the transfer
*       request for the last specified slave has completed. The user is
*       responsible to keep track of the last slave transfer.
*
* INPUTS
*
*       i2c_handle                             Nucleus I2C device handle.
*
*      *slaves                              Node structure array for
*                                           slave address details.
*
*      *tx_data                             Pointer to data output buffer
*                                           containing data for all slaves
*                                           for which write operation is
*                                           to be performed.
*
*      *rx_data                             Pointer to data input buffer
*                                           where data received for the
*                                           slaves for which read operation
*                                           is to be performed, will be
*                                           returned.
*
*      *lengths                             Array of number of data bytes
*                                           for each corresponding node.
*
*      *rw                                  Read/Write for each
*                                           corresponding node.
*
*       slave_count                         Number of slaves on the bus.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_ACK_TX_FAILED                   Acknowledgment transmission
*                                           failure.
*
*       I2C_ADDRESS_TX_FAILED               Master could not address the
*                                           slave node perhaps due to the
*                                           reason that it lost
*                                           arbitration.
*
*       I2C_BUFFER_FULL                     Data buffer is full.
*
*       I2C_BUS_BUSY                        I2C bus is busy.
*
*       I2C_DATA_RECEPTION_FAILED           Data reception failed.
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
*       I2C_INVALID_OPERATION               Invalid operation type. It
*                                           should be I2C_READ or I2C_WRITE.
*
*       I2C_INVALID_PARAM_POINTER           Null given instead of a
*                                           variable pointer.
*
*       I2C_INVALID_SLAVE_COUNT             Slave count for multi transfer
*                                           is wrong.
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
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS  I2CMC_Multi_Transfer     (I2C_HANDLE    i2c_handle,
                                  I2C_NODE     *slaves,
                                  UINT8        *tx_data,
                                  UINT8        *rx_data,
                                  UNSIGNED_INT *lengths,
                                  UINT8        *rw,
                                  UINT8         slave_count)
{
    I2C_CB         *i2c_cb;
    STATUS          status;
#if ( NU_I2C_ERROR_CHECKING )
    INT             counter;
#endif

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
    /* Check for valid parameters. */
    NU_I2C_PTRCHK_RETURN(slaves);
    NU_I2C_PTRCHK_RETURN(tx_data);
    NU_I2C_PTRCHK_RETURN(rx_data);
    NU_I2C_PTRCHK_RETURN(lengths);
    NU_I2C_PTRCHK_RETURN(rw);

#if ( NU_I2C_ERROR_CHECKING )

    /* Set initial value of status to be valid. */
    status = NU_SUCCESS;

    /* Check if all parameter values are O.K. */
    counter = 0;

    /* Check if the operation, data lengths and all slave address 
       are correct. */
    while ((counter < slave_count) && (status == NU_SUCCESS))
    {
         /* Check if all the slave addresses are valid. */
        status = I2CS_Check_Slave_Address(slaves[counter]);
        if ( status == NU_SUCCESS )
        {
            /* Check if write operation contains zero data length. */
            if (rw[counter] == I2C_WRITE)
            {
                if((lengths[counter] == 0))
                {
                    status = I2C_INVALID_TX_DATA_LENGTH;
                }
            }
    
            /* Check if read operation contains zero data length. */
            else if (rw[counter] == I2C_READ)
            {
                if((lengths[counter] == 0))
                {
                    status = I2C_INVALID_RX_DATA_LENGTH;
                }
            }
    
            /* The transfer operation is is neither read nor write. */
            else
            {
                /* Set status to indicate that invalid operation
                   specified for some given slave. */
                status = I2C_INVALID_OPERATION;
            }
        }
        counter++;
    }
    if ( status != NU_SUCCESS )
    {
        /* Revert back to user mode. */
        NU_USER_MODE();
        
        return ( status );
    }
#endif

    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Check if a minimum required slaves for multi transfer
       have been given by the user. */
    if(slave_count >= I2CM_MIN_SLAVES_FOR_MTR)
    {
        /* Set the internal pointers to the user arrays. */
        i2c_cb->i2c_mtr_queue.i2c_slave_address = slaves;
        i2c_cb->i2c_mtr_queue.i2c_slave_tx_data = tx_data;
        i2c_cb->i2c_mtr_queue.i2c_slave_rx_data = rx_data;
        i2c_cb->i2c_mtr_queue.i2c_data_length   = lengths;
        i2c_cb->i2c_mtr_queue.i2c_read_write    = rw;
        i2c_cb->i2c_mtr_queue.i2c_slave_count   = slave_count;

        /* Process the transfer request for the first slave. */
        status = I2CMC_Process_Transfer(i2c_handle,
                                        (BOOLEAN)NU_FALSE);
    }

    else
    {
        /* Set status to indicate that the slave count is not valid. */
        status = I2C_INVALID_SLAVE_COUNT;
    }
 
    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMC_Process_Transfer
*
* DESCRIPTION
*
*       This function is responsible for processing a complete I2C
*       read/write transfer.
*
* INPUTS
*
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*       restart                             Whether the transfer will
*                                           follow a RESTART signal.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_ACK_TX_FAILED                   Acknowledgment transmission
*                                           failure.
*
*       I2C_ADDRESS_TX_FAILED               Master could not address the
*                                           slave node perhaps due to the
*                                           reason that it lost
*                                           arbitration.
*
*       I2C_BUFFER_FULL                     Data buffer is full.
*
*       I2C_BUS_BUSY                        I2C bus is busy.
*
*       I2C_DATA_RECEPTION_FAILED           Data reception failed.
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
*       I2C_INVALID_OPERATION               Invalid operation type. It
*                                           should be I2C_READ or I2C_WRITE.
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
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS I2CMC_Process_Transfer    (I2C_HANDLE    i2c_handle,
                                  BOOLEAN       restart)
{
    STATUS        status;
    I2C_CB       *i2c_cb;
    
    I2CS_Get_CB(i2c_handle, &i2c_cb);
    
    /* Check the first slave operation. */
    if (*i2c_cb->i2c_mtr_queue.i2c_read_write == I2C_WRITE)
    {
        /* Set API mode to automatic API. */
        i2c_cb->i2c_api_mode = I2C_AUTOMATIC_TX_API;

        /* Call the I2C write service. */
        status = I2CMC_Process_Write(i2c_handle,
                    *(i2c_cb->i2c_mtr_queue.i2c_slave_address),
                      i2c_cb->i2c_mtr_queue.i2c_slave_tx_data,
                    *(i2c_cb->i2c_mtr_queue.i2c_data_length),
                      restart);
    }

    /* Check the first slave operation. */
    else if (*i2c_cb->i2c_mtr_queue.i2c_read_write == I2C_READ)
    {
        /* Set API mode to automatic API. */
        i2c_cb->i2c_api_mode = I2C_AUTOMATIC_RX_API;

        /* Call the I2C read service. */
        status = I2CMC_Process_Read(i2c_handle,
                    *(i2c_cb->i2c_mtr_queue.i2c_slave_address),
                      i2c_cb->i2c_mtr_queue.i2c_slave_rx_data,
                    *(i2c_cb->i2c_mtr_queue.i2c_data_length),
                      restart);
    }

    /* Invalid transfer operation. */
    else
    {
        /* Set status to indicate that the specified transfer operation
           is not valid. */
        status = I2C_INVALID_OPERATION;
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMC_Process_Next_Slave
*
* DESCRIPTION
*
*       This function is responsible for getting the next slave to which
*       a transfer should be done during a multi transfer request.
*
* INPUTS
*
*      *i2c_port                            Nucleus I2C port control
*                                           block pointer.
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_ACK_TX_FAILED                   Acknowledgment transmission
*                                           failure.
*
*       I2C_ADDRESS_TX_FAILED               Master could not address the
*                                           slave node perhaps due to the
*                                           reason that it lost
*                                           arbitration.
*
*       I2C_BUFFER_FULL                     Data buffer is full.
*
*       I2C_BUS_BUSY                        I2C bus is busy.
*
*       I2C_DATA_RECEPTION_FAILED           Data reception failed.
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
*       I2C_INVALID_OPERATION               Invalid operation type. It
*                                           should be I2C_READ or I2C_WRITE.
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
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS I2CMC_Process_Next_Slave  (I2C_HANDLE    i2c_handle)
                                      
{
    STATUS        status;
    I2C_CB       *i2c_cb;
    
    I2CS_Get_CB(i2c_handle, &i2c_cb);
    
    /* Set the node state to idle. */
    i2c_cb->i2c_node_state = I2C_NODE_IDLE;

    /* Check if the last operation performed was a write operation. */
    if(*i2c_cb->i2c_mtr_queue.i2c_read_write == I2C_WRITE)
    {
        /* Adjust write data pointer to point to the data for the
           next slave.  */
        i2c_cb->i2c_mtr_queue.i2c_slave_tx_data +=
                            (*i2c_cb->i2c_mtr_queue.i2c_data_length);
    }

    /* Check if the last operation performed was a read operation. */
    if(*i2c_cb->i2c_mtr_queue.i2c_read_write == I2C_READ)
    {
        /* Adjust read data pointer to point to the data buffer for the
           next slave. */
        i2c_cb->i2c_mtr_queue.i2c_slave_rx_data +=
                            (*i2c_cb->i2c_mtr_queue.i2c_data_length);
    }

    /* Point to the next slave address. */
    i2c_cb->i2c_mtr_queue.i2c_slave_address++;

    /* Point to the length to transmit/receive to/for the
       next slave address. */
    i2c_cb->i2c_mtr_queue.i2c_data_length++;

    /* Point to the operation (read/write) to be performed
       on the next slave. */
    i2c_cb->i2c_mtr_queue.i2c_read_write++;

    /* Decrement the number of slaves left for the transfer. */
    i2c_cb->i2c_mtr_queue.i2c_slave_count--;

    /* Process the next transfer request. */
    status = I2CMC_Process_Transfer(i2c_handle, (BOOLEAN)NU_TRUE);

    /* Return the completion status of the service. */
    return (status);
}
