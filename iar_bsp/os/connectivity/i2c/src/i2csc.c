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
*       i2csc.c
*
* COMPONENT
*
*       I2CS - I2C Slave
*
* DESCRIPTION
*
*       This file contains APIs that are used only with slave node.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CSC_Response_to_Read              Responds to the read command.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*       i2cs_extr.h                         Function prototypes for
*                                           Nucleus I2C slave.
*
*       i2cbm_extr.h                        Function prototypes for
*                                           Nucleus I2C buffer management.
*
*************************************************************************/
#define    NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2cs_extr.h"
#include    "connectivity/i2cbm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CSC_Response_to_Read
*
* DESCRIPTION
*
*       This API function provides the user with the facility to set the
*       data for sending in response to the read command from the master.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*      *data                                Pointer to the data to be
*                                           written in response to read
*                                           command.
*
*       length                              Number of bytes to send in
*                                           response.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_INVALID_DRIVER_MODE             Driver mode is not set to
*                                           interrupt driver.
*
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS  I2CSC_Response_to_Read  (I2C_HANDLE    i2c_handle,
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

    /* Check if device handle is valid and get the port control block
       and device control block. */
    I2CS_Get_CB(i2c_handle, &i2c_cb);
    
#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    /* Only call this function if I2C is operating in interrupt driven mode. */
    status = I2CBM_Put_Output_Buffer(i2c_cb, data, length);
#else
    /* Set status to indicate that the driver mode is not right
       for the slave. */
    status = I2C_INVALID_DRIVER_MODE;
    NU_UNUSED_PARAM(i2c_cb);
#endif
 
    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
