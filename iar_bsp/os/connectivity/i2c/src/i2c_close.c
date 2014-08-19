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
*       i2c_close.c
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains API function that is responsible for closing
*       Nucleus I2C device.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CC_Close                          Closes a Nucleus I2C device.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*       i2c_osal_extr.h                     Function prototypes for
*                                           OS abstraction services.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"
#include    "connectivity/i2c_osal_extr.h"


/*************************************************************************
* FUNCTION
*
*       I2CC_Close
*
* DESCRIPTION
*
*       This function closes the specified Nucleus I2C device.
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
*       I2C_SHUTDOWN_ERROR                  Error in closing Nucleus I2C
*                                           device.
*
*       I2C_TRANSFER_STOP_FAILED            Transfer on I2C bus could not
*                                           be stopped.
*
*       I2C_DEVICE_IN_USE                   Device is being used by some
*                                           other user.
*
*************************************************************************/
STATUS  I2C_Close  (I2C_HANDLE i2c_handle)
{
    I2C_CB         *i2c_cb;

    STATUS          status = NU_SUCCESS;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if device handle is valid and get the port control block
       and device control block. */
    I2CS_Get_CB(i2c_handle, &i2c_cb);

    if (i2c_cb->i2c_dev_user_count == 0)
    {
        /* Revert back to user mode. */
        NU_USER_MODE();

        return I2C_SHUTDOWN_ERROR;
    }

    /* Decrease number of I2C device users. */
    i2c_cb->i2c_dev_user_count--;

    /* Check if no more users are using the device. */
    if (i2c_cb->i2c_dev_user_count == 0)
    {
        /* Check if node is master and has any active channel. */
        if ((i2c_cb->i2c_node_state != I2C_NODE_IDLE) &&
            ((i2c_cb->i2c_node_address.i2c_node_type) & I2C_MASTER_NODE))
        {
            /* Stop transfer by the node. */
            status = I2CMC_Send_Stop(i2c_handle);
        }

        /* Close the hardware driver. */
        status |= DVC_Dev_Close(i2c_cb->i2c_dv_handle);

        /* Delete all the registered callbacks including the default
           callbacks. */
        status |= I2CMS_Delete_All_Callbacks(i2c_cb);

        /* Deallocate resources occupied by the device. */
        status |= I2C_OSAL_Deallocate_Resources(i2c_cb);

        /* Set the specified device pointer to Null. */
        i2c_cb->device_opened = 0;
        i2c_cb->is_opened = 0;
        i2c_cb->i2c_dv_id = -1;
    }
    /* Device is being used by some other user. */
    else
    {
        /* Set the status to indicate that the device is not closed
           because some other user is using the device. */
        status = I2C_DEVICE_IN_USE;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
