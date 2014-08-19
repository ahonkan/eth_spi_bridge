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
*       i2cms_mode.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the mode (interrupt or polling) related
*       functions for Nucleus I2C master.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMS_Get_Mode                      Gets the mode (polling or
*                                           interrupt) of the Nucleus
*                                           I2C device.
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
*       I2CMS_Get_Mode
*
* DESCRIPTION
*
*       This function returns the driver mode (polling or interrupt) of
*       the Nucleus I2C device.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*      *i2c_driver_mode                     Pointer to return the driver
*                                           mode.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_INVALID_PARAM_POINTER           Pointer given to return
*                                           the driver mode is null.
*
*************************************************************************/
STATUS  I2CMS_Get_Mode (I2C_HANDLE i2c_handle,
                        I2C_DRIVER_MODE *i2c_driver_mode)
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

    /* Check if driver mode return pointer is not null. */
    if (i2c_driver_mode != NU_NULL)
    {
        /* Get the driver mode. */
        *i2c_driver_mode = i2c_cb->i2c_driver_mode;
    }

    else
    {
        /* Set status to indicate that pointer to return
           driver mode was null. */
        status = I2C_INVALID_PARAM_POINTER;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* !(NU_I2C_NODE_TYPE == I2C_SLAVE_NODE) */
