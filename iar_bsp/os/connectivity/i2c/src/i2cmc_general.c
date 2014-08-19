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
*       i2cmc_general.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the services that are based upon the general
*       call address functionality as defined by I2C protocol.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Set_Slave_Address             Writes programmable part of
*                                           the slave address.
*
*       I2CMC_Config_HW_Master              Configures the hardware master
*                                           for the slave address.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2cm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CMC_Set_Slave_Address
*
* DESCRIPTION
*
*       This API directs the remote slaves to write their programmable
*       part of the slave address by hardware.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       reset                               Whether the slave should also
*                                           reset along with address
*                                           change.
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
*************************************************************************/
STATUS  I2CMC_Set_Slave_Address        (I2C_HANDLE i2c_handle,
                                        BOOLEAN    reset)
{
    I2C_CB         *i2c_cb;

    STATUS          status;
    I2C_NODE        slave;
       UINT8   general_call_data;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);

 
    /* Check if the slave will be reset before its address is
       configured by the hardware. */
    if (reset == NU_TRUE)
    {
        general_call_data = I2CM_GC_RESET_PROGRAM_ADDRESS;
    }

    /* No, slave will not be reset. It will only reprogram
       its address. */
    else
    {
        general_call_data = I2CM_GC_PROGRAM_ADDRESS;
    }

    /* Set API mode to automatic API. */
    i2c_cb->i2c_api_mode = I2C_AUTOMATIC_TX_API;

    /* Set slave address for general call. */
    slave.i2c_slave_address = I2CM_GENERAL_CALL_ADDRESS;
    slave.i2c_address_type  = I2C_7BIT_ADDRESS;

    /* Process the I2C write operation. */
    status = I2CMC_Process_Write(i2c_handle, slave,
                                 &general_call_data, 1,
                                 (BOOLEAN)NU_FALSE);

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMC_Config_HW_Master
*
* DESCRIPTION
*
*       This API function configures the the hardware master for the slave
*       address to which it will be writing the data.
*
* INPUTS
*
*       i2c_handle                             Nucleus I2C device handle.
*
*       hw_master_address                   Address of the hardware master.
*
*       hw_master_slave                     Address of the slave to which
*                                           hardware master will be
*                                           writing the data.
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
*************************************************************************/
STATUS  I2CMC_Config_HW_Master         (I2C_HANDLE i2c_handle,
                                        UINT8      hw_master_address,
                                        UINT8      hw_master_slave)
{
    I2C_CB         *i2c_cb;

    STATUS          status = NU_SUCCESS;
    I2C_NODE        slave;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if the given hardware master addresses is invalid. */
    if ((hw_master_address < I2C_MIN_7BIT_VALID_ADDRESS) ||
        (hw_master_address > I2C_MAX_7BIT_VALID_ADDRESS))
    {
        /* Set status to indicate that this is reserved address. */
        status = I2C_INVALID_SLAVE_ADDRESS;
    }

    /* Check if the given slave address to hardware master is invalid. */
    else if ((hw_master_slave < I2C_MIN_7BIT_VALID_ADDRESS) ||
             (hw_master_slave > I2C_MAX_7BIT_VALID_ADDRESS))
    {
        /* Set status to indicate that this is reserved address. */
        status = I2C_INVALID_SLAVE_ADDRESS;
    }

    else
    {
        /* Check if device handle is valid and get the port control block
           and device control block. */
         I2CS_Get_CB(i2c_handle, &i2c_cb);
    }

    /* Check if control blocks retrieved successfully. */
    if (status == NU_SUCCESS)
    {
        UINT8   dump_address_for_hw_master;

        /* Set API mode to automatic API. */
        i2c_cb->i2c_api_mode = I2C_AUTOMATIC_TX_API;

        slave.i2c_slave_address = hw_master_address;
        slave.i2c_address_type  = I2C_7BIT_ADDRESS;

        /* Set the slave address to dump to the hardware master. */
        dump_address_for_hw_master = hw_master_slave << 1;

        /* Process the I2C write operation. */
        status = I2CMC_Process_Write(i2c_handle, slave,
                                     &dump_address_for_hw_master, 1,
                                     (BOOLEAN)NU_FALSE);
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
