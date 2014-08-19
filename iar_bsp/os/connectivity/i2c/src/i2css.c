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
*       i2css.c
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
*       I2CSS_Slave_Get_Address             Gets the slave address and
*                                           type of address.
*
*       I2CSS_Slave_Set_Address             Sets the slave address and
*                                           type of address.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*       i2cs_extr.h                         Function prototypes for
*                                           Nucleus I2C slave.
*
*************************************************************************/
#define    NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2cs_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CSS_Slave_Get_Address
*
* DESCRIPTION
*
*       This API function can be used to know the details of the local
*       slave address of a node that provides the functionality of an
*       I2C slave.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*      *address                             Location where slave address
*                                           will be returned.
*
*      *address_type                        Location where slave address
*                                           type (7-bit/10-bit) will be
*                                           returned.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_NODE_NOT_SLAVE                  The node doesn't provide the
*                                           functionality of a slave.
*
*************************************************************************/
STATUS  I2CSS_Slave_Get_Address (I2C_HANDLE i2c_handle,
                                 UINT16    *address,
                                 UINT8     *address_type)
{
    I2C_CB         *i2c_cb;
    
    STATUS          status = NU_SUCCESS;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check for valid parameters. */    
    NU_I2C_PTRCHK_RETURN(address);
    NU_I2C_PTRCHK_RETURN(address_type);
    
    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);

    /* Check if node provides the functionality of slave. */
    if (i2c_cb->i2c_node_address.i2c_node_type & I2C_SLAVE_NODE)
    {
        /* Get slave information from control block. */

        *address = i2c_cb->i2c_node_address.i2c_slave_address;
        *address_type = i2c_cb->i2c_node_address.i2c_address_type;
    }

    /* Node is pure master. */
    else
    {
        /* Set status to indicate that the node doesn't have the
           slave functionality. */
        status = I2C_NODE_NOT_SLAVE;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CSS_Slave_Set_Address
*
* DESCRIPTION
*
*       This API function will allow the user to set the local slave
*       address of a node that provides the functionality of an I2C slave.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       slave_address                       Slave address of the node.
*
*       address_type                        Type of the slave address.
*                                           Possible values for 7bit/10bit
*                                           are respectively;
*                                           I2C_7BIT_ADDRESS
*                                           I2C_10BIT_ADDRESS
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_NODE_NOT_SLAVE                  The node doesn't provide the
*                                           functionality of a slave.
*
*       I2C_INVALID_SLAVE_ADDRESS           Address of the slave node is
*                                           not valid.
*
*************************************************************************/
STATUS  I2CSS_Slave_Set_Address (I2C_HANDLE i2c_handle,
                                 UINT16     slave_address,
                                 UINT8      address_type)
{
    I2C_CB         *i2c_cb;
    STATUS          status;
#if ( NU_I2C_ERROR_CHECKING )
    I2C_NODE        slave;
#endif

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
#if ( NU_I2C_ERROR_CHECKING )
    
    slave.i2c_address_type = address_type;
    slave.i2c_slave_address = slave_address;
    
    status = I2CS_Check_Slave_Address(slave);
    if ( status == NU_SUCCESS )
#endif
    {
        /* Check if device handle is valid and get the port control block
           and device control block. */
         I2CS_Get_CB(i2c_handle, &i2c_cb);
    
        /* Check if node provides the functionality of slave. */
        if (i2c_cb->i2c_node_address.i2c_node_type & I2C_SLAVE_NODE)
        {
            /* Set slave address. */
            i2c_cb->i2c_node_address.i2c_slave_address = slave_address;
    
            /* Set slave address type. */
            i2c_cb->i2c_node_address.i2c_address_type = address_type;
    
            /* Transmit address to the node. */
            status = I2CC_Ioctl_Driver(i2c_handle,
                       I2C_SET_SLAVE_ADDRESS, NU_NULL);
        }
    
        /* Node is pure master. */
        else
        {
            /* Set status to indicate that the node doesn't have the
               slave functionality. */
            status = I2C_NODE_NOT_SLAVE;
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
