
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
*       i2cmc_start.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the service to start a transfer on I2C bus.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Start_Transfer                Starts a transfer on I2C bus.
*
*       I2CMC_Send_Start                    Sends START signal and address
*                                           on I2C bus.
*
*       I2CMC_Ack_Timed_Out                 Checks for ACK signal timeout.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"

#if ( NU_I2C_SUPPORT_POLLING_MODE )
static       BOOLEAN      I2CMC_Ack_Timed_Out(UNSIGNED prev_clock);
#endif /* #if ( NU_I2C_SUPPORT_POLLING_MODE ) */

#if ( NU_I2C_SUPPORT_FINE_CONTROL_API )
/*************************************************************************
* FUNCTION
*
*       I2CMC_Start_Transfer
*
* DESCRIPTION
*
*       This API function is used to initiate a transfer on the network.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       slave                               Node structure containing node
*                                           details.
*
*       rw                                  Read/Write.
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
STATUS I2CMC_Start_Transfer  (I2C_HANDLE i2c_handle,
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
    
        /* Set the API mode to fine control API. */
        i2c_cb->i2c_api_mode = I2C_FINE_CONTROL_API;
    
        /* Send the address on the bus. */
        status = I2CMC_Send_Start(i2c_handle,
                                  slave, rw, (BOOLEAN)NU_FALSE);
    }
 
    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
#endif /* #if ( NU_I2C_SUPPORT_FINE_CONTROL_API ) */

/*************************************************************************
* FUNCTION
*
*       I2CMC_Send_Start
*
* DESCRIPTION
*
*       This function is used to send START/RESTART signal as per I2C
*       protocol and the slave address on the I2C bus.
*
* INPUTS
*
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*       slave                               Node structure containing node
*                                           details.
*
*       rw                                  I2C Read/Write operation.
*
*       restart                             Should a RESTART signal be sent.
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
STATUS I2CMC_Send_Start      (
                              I2C_HANDLE i2c_handle,
                              I2C_NODE      slave,
                              UINT8         rw,
                              BOOLEAN       restart)
{   I2C_CB          *i2c_cb;
    UNSIGNED        current_clock = 0;
    UINT16          slave_address;
    STATUS          status = NU_SUCCESS;
    I2CS_Get_CB(i2c_handle, &i2c_cb);
    /* Check if node has the functionality of a master. */
    if(i2c_cb->i2c_node_address.i2c_node_type & I2C_MASTER_NODE)
    {
        /* Check if no transfer is in progress for the node. */
        if (i2c_cb->i2c_node_state == I2C_NODE_IDLE)
        {
            /* Check if this is a 7-bit slave address. */
            if (slave.i2c_address_type == I2C_7BIT_ADDRESS)
            {
                /* Synthesize address byte. */
                slave_address = (slave.i2c_slave_address << 1) |
                    ((rw == I2C_READ) ? I2C_READ : I2C_WRITE);

                /* Set node state to waiting address ACK. */
                i2c_cb->i2c_node_state = I2C_WAITING_ADDRESS_ACK;

                /* Check if RESTART signal should be sent before
                   the address. */
                if (restart == NU_TRUE)
                {
                    /* Call the driver service to transmit the address
                       byte following the transmission of a START signal. */
                    status = I2CC_Ioctl_Driver
                        (i2c_handle, I2C_SEND_RESTART_ADDRESS,
                        &slave_address);
                }

                /* START signal should be sent before the address. */
                else
                {
                    /* Call the driver service to transmit the address
                       following the transmission of a START signal. */
                    status = I2CC_Ioctl_Driver
                                    (i2c_handle, I2C_SEND_START_ADDRESS,
                                                          &slave_address);
                }

#if ( NU_I2C_SUPPORT_POLLING_MODE )

                /* Check if I2C is operating in polling mode. */
                if (status == NU_SUCCESS)
                {
                    /* Get the current value of the system timer. */
                    current_clock = NU_Retrieve_Clock();

                    /* Poll for slave address acknowledgment.
                       If slave doesn't acknowledge for a specified
                       interval of time, then abort it. */
                    do
                    {
                        /* Check if slave address is acknowledged. */
                        status = I2CC_Ioctl_Driver
                                (i2c_handle, I2C_CHECK_ADDRESS_ACK, NU_NULL);

                    } while ((status != NU_SUCCESS) &&
                       (I2CMC_Ack_Timed_Out(current_clock) == NU_FALSE));

                    /* Check if the slave has acknowledged before
                       timeout. */
                    if (status == NU_SUCCESS)
                    {
                        /* Set node state to indicate slave address
                           acknowledged. */
                        i2c_cb->i2c_node_state = I2C_SLAVE_ACKED;
                    }

                    else
                    {
                        /* Set the status to indicate that slave
                           has timed out. */
                        status = I2C_SLAVE_TIME_OUT;
                    }
                }

#endif      /* NU_I2C_SUPPORT_POLLING_MODE */

            }

            /* Check if this is a 10-bit slave address. */
            else if (slave.i2c_address_type == I2C_10BIT_ADDRESS)
            {

                /* Save low 8 bits of 10-bit address. */
                i2c_cb->i2c_10bit_address2 =
                                          (UINT8)slave.i2c_slave_address;

                /* Synthesize first address byte of 10-bit address. */
                slave_address = I2C_10BIT_ADDRESS_CODE |
                    ((slave.i2c_slave_address >> 8) << 1) |
                    ((rw == I2C_READ) ? I2C_READ :I2C_WRITE);

                /* Set node state to transmitting address state. */
                i2c_cb->i2c_node_state = I2C_TRANSMITTING_ADDRESS;

                /* Check if restart signal should be sent before
                   the address. */
                if (restart == NU_TRUE)
                {
                    /* Call the driver service to transmit the address
                       byte following the transmission of a START
                       signal. */
                    status = I2CC_Ioctl_Driver
                                (i2c_handle, I2C_SEND_RESTART_ADDRESS,
                                 &slave_address);
                }

                else
                {
                    /* Call the driver service to transmit the address
                       following the transmission of a START signal. */
                    status = I2CC_Ioctl_Driver
                                (i2c_handle, I2C_SEND_START_ADDRESS,
                                 &slave_address);
                }

#if ( NU_I2C_SUPPORT_POLLING_MODE )

                /* Check if I2C is operating in polling mode. */
                if (status == NU_SUCCESS)
                {
                    /* Get the current value of the system timer. */
                    current_clock = NU_Retrieve_Clock();

                    /* Poll for slave address acknowledgment. */
                    do
                    {
                        /* Check if address has been acknowledged
                           successfully. */
                        status = I2CC_Ioctl_Driver
                            (i2c_handle, I2C_CHECK_ADDRESS_ACK, NU_NULL);

                    } while ((status != NU_SUCCESS) &&
                       (I2CMC_Ack_Timed_Out(current_clock) == NU_FALSE));

                    /* Check if slave has acknowledged before timeout. */
                    if (status == NU_SUCCESS)
                    {
                        /* Send 2nd address byte. */
                        status = I2CMS_Send_Address_Byte2(i2c_handle);

                        /* Poll for the second address byte
                           acknowledgment. */
                        if (status == NU_SUCCESS)
                        {
                            /* Get the current value of the clock. */
                            current_clock = NU_Retrieve_Clock();

                            /* Poll for 2nd address byte acknowledgment. */
                            do
                            {
                                /* Check if second address byte
                                   reception has been acknowledged? */
                                status = I2CC_Ioctl_Driver
                                            (i2c_handle,
                                            I2C_CHECK_ADDRESS_ACK,
                                            NU_NULL);

                            } while ((status != NU_SUCCESS) &&
                                (I2CMC_Ack_Timed_Out(current_clock)
                                == NU_FALSE));

                            /* Check if the slave has acknowledged
                               before timeout. */
                            if (status == NU_SUCCESS)
                            {
                                /* Set node state to indicate slave
                                   address ACKed. */
                                i2c_cb->i2c_node_state = I2C_SLAVE_ACKED;
                            }

                            else
                            {
                                /* Set the status to indicate that
                                   slave has timed out. */
                                status = I2C_SLAVE_TIME_OUT;
                            }
                        }
                    }

                    else
                    {
                        /* Set the status to indicate that slave
                           has timed out. */
                        status = I2C_SLAVE_TIME_OUT;
                    }
                }

#endif      /* NU_I2C_SUPPORT_POLLING_MODE */

            }

            /* Address type is not valid. */
            else
            {
                /* Set status to invalid address type.*/
                status = I2C_INVALID_ADDRESS_TYPE;
            }
        }

        /* Node is not idle. */
        else
        {
            /* Set status to indicate that a transfer operation
               (read/write) is already in progress for the node. */
            status = I2C_TRANSFER_IN_PROGRESS;
        }
    }

    else
    {
        /* Set the status to indicate that the node doesn't have the
           functionality of master. */
        status = I2C_NODE_NOT_MASTER;
    }

    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(current_clock);

    /* Return the completion status of the service. */
    return (status);
}

#if ( NU_I2C_SUPPORT_POLLING_MODE )
/*************************************************************************
* FUNCTION
*
*       I2CMC_Ack_Timed_Out
*
* DESCRIPTION
*
*       This function checks if the period to wait for an acknowledgment
*       from slave has expired.
*
* INPUTS
*
*      prev_clock                           Previous clock value.
*
* OUTPUTS
*
*       NU_TRUE                             Acknowledgment waiting
*                                           period timed out.
*
*       NU_FALSE                            Acknowledgment waiting
*                                           period not timed out.
*
*************************************************************************/
static      BOOLEAN I2CMC_Ack_Timed_Out      (UNSIGNED prev_clock)
{
    UNSIGNED    time_elaped;
    BOOLEAN     time_out_status = NU_FALSE;

    /* Check if the current system clock value is
       greater than the previous system clock value. */
    if (NU_Retrieve_Clock() > prev_clock)
    {
        /* Get the difference. */
        time_elaped =  NU_Retrieve_Clock() - prev_clock;
    }

    /* Current system clock value is smaller than the previous
       system clock value. */
    else
    {
        /* Get the difference. */
        time_elaped =  prev_clock - NU_Retrieve_Clock();
    }

    /* Check if the difference is within the configured limits. */
    if (time_elaped > NU_I2C_ACK_WAIT)
    {
        /* Set the time out status to indicate that the time
           out has occurred. */
        time_out_status = NU_TRUE;
    }

    /* Return the completion status of the service. */
    return (time_out_status);
}
#endif      /* NU_I2C_SUPPORT_POLLING_MODE */
