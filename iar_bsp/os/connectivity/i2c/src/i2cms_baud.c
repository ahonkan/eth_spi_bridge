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
*       i2cms_baud.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the baud rate related functions for Nucleus I2C
*       master.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMS_Set_Baudrate                  Sets the baud rate on the
*                                           given master device.
*
*       I2CMS_Get_Baudrate                  Gets the baud rate on the given
*                                           master device.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define    NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"

/* Scale out this function if only the slave functionality is to be used
   always for all I2C device on this target. */

#if         (!(NU_I2C_NODE_TYPE == I2C_SLAVE_NODE))

/*************************************************************************
* FUNCTION
*
*       I2CMS_Set_Baudrate
*
* DESCRIPTION
*
*       This function will set the baud rate for the Nucleus I2C network.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       baudrate                            Baud rate to set for the
*                                           specified I2C device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_INVALID_BAUDRATE                The specified baud rate is
*                                           out of specified range of
*                                           0-400kbps or is not supported
*                                           by the driver.
*
*       I2C_NODE_NOT_MASTER                 The specified device doesn't
*                                           support master functionality.
*
*************************************************************************/
STATUS  I2CMS_Set_Baudrate(I2C_HANDLE i2c_handle, UINT16 baud_rate)
{
    I2C_CB         *i2c_cb;
    
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Check if node provides the functionality of the master. */
    if (i2c_cb->i2c_node_address.i2c_node_type & I2C_MASTER_NODE)
    {
       /* Protect the critical section against multithread access. */
        NU_Protect(&I2C_Protect_Struct);

        status = I2CC_Ioctl_Driver (i2c_handle, I2C_SET_BAUDRATE, (VOID*)(UINT32)baud_rate);
                                  

        /* Release the protection. */
        NU_Unprotect();
        /* Check if baud rate set successfully. */
        if (status == NU_SUCCESS)
        {
            /* Record the set baud rate. */
            i2c_cb->i2c_baudrate = baud_rate;
        }
    }

    /* Node is acting as a slave. */
    else
    {
        /* Set status to indicate that the service is not supported
           on the slave. */
        status = I2C_NODE_NOT_MASTER;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMS_Get_Baudrate
*
* DESCRIPTION
*
*       This function returns the baud rate for the Nucleus I2C network.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       baudrate                            Baud rate set for the
*                                           specified I2C device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_INVALID_PARAM_POINTER           Pointer given to return
*                                           baud rate is null.
*
*       I2C_NODE_NOT_MASTER                 The specified device doesn't
*                                           support master functionality.
*
*************************************************************************/
STATUS  I2CMS_Get_Baudrate(I2C_HANDLE i2c_handle, UINT16 *baudrate)
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

    /* Check if node provides the functionality of the master. */
    if (i2c_cb->i2c_node_address.i2c_node_type & I2C_MASTER_NODE)
    {
        /* Check if baud rate return pointer is not null. */
        if (baudrate != NU_NULL)
        {
            /* Get baud rate for Nucleus I2C transmission. */
            *baudrate = i2c_cb->i2c_baudrate;
        }

        else
        {
            /* Set status to indicate that pointer to return
               baud rate was null. */
            status = I2C_INVALID_PARAM_POINTER;
        }
    }

    /* Node is acting as a slave. */
    else
    {
        /* Set status to indicate that the service is not supported
           on the slave. */
        status = I2C_NODE_NOT_MASTER;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* (NU_I2C_NODE_TYPE == I2C_SLAVE_NODE) */
