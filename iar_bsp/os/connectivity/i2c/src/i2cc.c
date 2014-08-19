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
*       i2cc.c
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains the common API routines for Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CC_Receive_Data                   Receives data from remote
*                                           node.
*
*       I2CC_Get_Node_State                 Get current node state.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*       i2cbm_extr.h                        Function prototypes for
*                                           I2C buffer management.
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master component.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2cbm_extr.h"
#include    "connectivity/i2cm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CC_Receive_Data
*
* DESCRIPTION
*
*       This function reads the data received from the remote node to the
*       application.
*
* INPUTS
*
*       i2c_handle                             Nucleus I2C device handle.
*
*      *data                                Data pointer.
*
*       length                              Data length.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_BUFFER_EMPTY                    Data buffer is empty.
*
*       I2C_BUFFER_NOT_EMPTY                Buffer has some more bytes
*                                           even after the reading.
*
*       I2C_DATA_RECEPTION_FAILED           Data reception failed.
*
*       I2C_BUFFER_HAS_LESS_DATA            Requested data is more than
*                                           the buffer has.
*
*************************************************************************/
STATUS  I2CC_Receive_Data      (I2C_HANDLE   i2c_handle,
                                UINT8       *data,
                                UNSIGNED_INT length)
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

    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Get the data received from the input buffer. */
    status = I2CBM_Get_Input_Buffer(i2c_cb, data, length);

    /* Check if it is multi transfer and another slave needs to be
       processed. */
    if(i2c_cb->i2c_mtr_queue.i2c_slave_count > 1)
    {
        status = I2CMC_Process_Next_Slave(i2c_handle);
    }

    else
    {
        /* Set the node state to idle. */
        i2c_cb->i2c_node_state = I2C_NODE_IDLE;
    }
 
    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CC_Get_Node_State
*
* DESCRIPTION
*
*       This API function can be used to get the current state of the node.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*      *node_state                          Location where node state
*                                           will be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*************************************************************************/
STATUS  I2CC_Get_Node_State    (I2C_HANDLE i2c_handle,
                                UINT8     *node_state)
{
    I2C_CB         *i2c_cb;

    STATUS      status = NU_SUCCESS;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
    /* Check for valid parameters. */
    NU_I2C_PTRCHK_RETURN(node_state);

    /* Check if device handle is valid and get the port control block
       and device control block. */
    I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Get and return current node state. */
    *node_state = i2c_cb->i2c_node_state;
    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
