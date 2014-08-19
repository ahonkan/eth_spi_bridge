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
*       i2cc_ioctl.c
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains the services to provide interface to I/O control
*       type routines of the driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CC_Ioctl_Driver                   Perform I/O configuration
*                                           operations on the controller.
*
*       I2CC_Check_Ack                      Checks the acknowledgment
*                                           command from a slave.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CC_Ioctl_Driver
*
* DESCRIPTION
*
*       This API function provides an interface to the i2c_driver_ioctl
*       function which performs I/O operations on the controller.
*
* INPUTS
*
*       i2c_handle                             Nucleus I2C device handle.
*
*       operation_code                      Code to perform I/O operation.
*                                           Possible values include but
*                                           not limited to
*                                           I2C_CHECK_ADDRESS_ACK
*                                           I2C_CHECK_DATA_ACK
*                                           I2C_CHECK_NACK
*                                           I2C_CHECK_BUS_FREE
*                                           I2C_CHECK_DATA
*                                           I2C_CHECK_RESTART
*                                           I2C_CHECK_STOP
*                                           I2C_SEND_NACK
*                                           I2C_SEND_ACK
*                                           I2C_SEND_START_ADDRESS
*                                           I2C_SEND_RESTART_ADDRESS
*                                           I2C_SEND_ADDRESS2
*                                           I2C_SEND_DATA
*                                           I2C_SEND_STOP
*                                           I2C_SET_BAUDRATE
*                                           I2C_SET_SLAVE_ADDRESS
*                                           I2C_SET_NODE_MODE_RX
*                                           I2C_SET_NODE_MODE_TX
*
*      *operation_data                      Pointer to the data required
*                                           to perform the operation or
*                                           pointer where the data of an
*                                           operation will be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_BUS_BUSY                        I2C bus is occupied by some
*                                           I2C master.
*
*       I2C_INVALID_BAUDRATE                Invalid baud rate value for
*                                           the device.
*
*       I2C_INVALID_IOCTL_OPERATION         Unsupported operation
*                                           requested for I/O control.
*
*       I2C_SLAVE_NOT_ACKED                 Slave did not acknowledge.
*
*************************************************************************/
STATUS  I2CC_Ioctl_Driver (I2C_HANDLE i2c_handle,
                           UINT8      operation_code,
                           VOID      *operation_data)
{
    I2C_CB         *i2c_cb;

    STATUS      status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Call the driver I/O control function. */
    status = DVC_Dev_Ioctl (i2c_cb->i2c_dv_handle,
                                       (i2c_cb->i2c_ioctl_base+operation_code),
                                       operation_data, 4);
 
    /* Revert back to user mode. */
    NU_USER_MODE();
    
    /* Return the completion status of the service. */
    return (status);
}

#if ( NU_I2C_SUPPORT_FINE_CONTROL_API )
/*************************************************************************
* FUNCTION
*
*       I2CC_Check_Ack
*
* DESCRIPTION
*
*       This API function can be used to poll for the acknowledgment
*       reception of data.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_NO_RESTART_SIGNAL               RESTART signal not received.
*
*       I2C_NO_STOP_SIGNAL                  STOP signal not received.
*
*       I2C_POLLING_ABORTED                 Polling aborted.
*
*       I2C_SLAVE_NOT_ACKED                 Slave did not acknowledge.
*
*************************************************************************/
STATUS  I2CC_Check_Ack         (I2C_HANDLE i2c_handle)
{
     STATUS      status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Poll for data/address byte acknowledgment. */
    do
    {
        /* Check if the addressed slave has acknowledged. */
        status = I2CC_Ioctl_Driver(i2c_handle,
                                I2C_CHECK_DATA_ACK, NU_NULL);

        /* Check if acknowledgment not received. */
        if (status != NU_SUCCESS)
        {
            /* Check if STOP signal received to stop polling. */
            status = I2CC_Ioctl_Driver(i2c_handle,
                                     I2C_CHECK_STOP, NU_NULL);

            /* Check if STOP signal received. */
            if (status == NU_SUCCESS)
            {
                /* Set status to indicate polling aborted. */
                status = I2C_POLLING_ABORTED;
                break;
            }

            /* No STOP signal on the network. */
            else
            {
                /* Check if RESTART signal received to stop polling. */
                status = I2CC_Ioctl_Driver
                         (i2c_handle, I2C_CHECK_RESTART, NU_NULL);

                /* Check if RESTART signal received. */
                if (status == NU_SUCCESS)
                {
                    /* Set status to indicate polling aborted. */
                    status = I2C_POLLING_ABORTED;
                    break;
                }
            }
        }
    } while (status != NU_SUCCESS);

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
#endif /* #if ( NU_I2C_SUPPORT_FINE_CONTROL_API ) */
