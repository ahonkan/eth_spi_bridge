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
*       i2cmc_stop.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the service that is responsible for stopping
*       a transfer on the I2C network.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMC_Stop_Transfer                 Stops transfer on the network.
*
*       I2CMC_Send_Stop                     Sends STOP signal on I2C bus.
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
*       I2CMC_Stop_Transfer
*
* DESCRIPTION
*
*       This API function is responsible for ending the transfer that was
*       in progress with a slave.
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
*       I2C_NODE_IS_NOT_MASTER              The node is not master.
*
*       I2C_TRANSFER_STOP_FAILED            Transfer on I2C bus could not
*                                           be stopped.
*
*************************************************************************/
STATUS  I2CMC_Stop_Transfer  (I2C_HANDLE i2c_handle)
{
    STATUS      status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Send I2C STOP signal. */
    status = I2CMC_Send_Stop(i2c_handle);

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
#endif /* #if ( NU_I2C_SUPPORT_FINE_CONTROL_API ) */

/*************************************************************************
* FUNCTION
*
*       I2CMC_Send_Stop
*
* DESCRIPTION
*
*       This function is responsible for sending STOP signal as per I2C
*       protocol to stop the transfer on the I2C bus.
*
* INPUTS
*
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_NODE_NOT_MASTER                 The node is not master.
*
*       I2C_TRANSFER_STOP_FAILED            Transfer on I2C bus could not
*                                           be stopped.
*
*************************************************************************/
STATUS  I2CMC_Send_Stop      (I2C_HANDLE    i2c_handle)
{
    STATUS           status;
    I2C_CB          *i2c_cb;
    
    I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Check if node has the functionality of a master. */
    if(i2c_cb->i2c_node_address.i2c_node_type & I2C_MASTER_NODE)
    {
        /* Transfer stop signal. */
        status = I2CC_Ioctl_Driver(i2c_handle, I2C_SEND_STOP, NU_NULL);
        /* Set node state to idle. */
        i2c_cb->i2c_node_state = I2C_NODE_IDLE;
    }

    else
    {
        /* Set the status to indicate that the node doesn't have the
           functionality of master. */
        status = I2C_NODE_NOT_MASTER;
    }

    /* Return the completion status of the service. */
    return (status);
}
