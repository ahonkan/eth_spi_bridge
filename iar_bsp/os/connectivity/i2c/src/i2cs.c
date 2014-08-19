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
*       i2cs.c
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains supplemental services for I2C core component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CS_Check_Init_Params              Checks Nucleus I2C
*                                           initialization parameters.
*
*       I2CS_Check_Slave_Address            Checks slave address.
*
*       I2CS_Get_CB                        Get device control block.
*
* DEPENDENCIES
*
*       i2cs_extr.h                         Function prototypes for
*                                           Nucleus I2C slave.
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*************************************************************************/
#define         NU_I2C_SOURCE_FILE

#include        "connectivity/i2cs_extr.h"
#include        "connectivity/i2c_extr.h"


#if         (NU_I2C_ERROR_CHECKING)

/*************************************************************************
* FUNCTION
*
*       I2CS_Check_Init_Params
*
* DESCRIPTION
*
*       This function checks Nucleus I2C initialization parameters.
*
* INPUTS
*
*      *i2c_init                            Nucleus I2C initialization
*                                           structure pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_ADDRESS_TYPE            Address type is neither 7-bit
*                                           nor 10-bit.
*
*       I2C_INVALID_DEVICE_ID               Specified device is not valid.
*
*       I2C_INVALID_DRIVER_MODE             Specified driver mode is not
*                                           valid.
*
*       I2C_INVALID_NODE_TYPE               Node type is not master/slave.
*
*       I2C_INVALID_PORT_ID                 Specified port is not valid.
*
*       I2C_INVALID_SLAVE_ADDRESS           Slave address is not valid as
*                                           it falls into the reserved
*                                           range or is more than the
*                                           maximum possible value.
*
*       I2C_NULL_GIVEN_FOR_INIT             Initialization structure
*                                           pointer is null.
*
*       I2C_NULL_GIVEN_FOR_MEM_POOL         Memory pool pointer is null.
*
*************************************************************************/
STATUS  I2CS_Check_Init_Params   (I2C_INIT *i2c_init)
{
    STATUS      status = NU_SUCCESS;

    /* Check if pointer to initialization structure is not null. */
    if (i2c_init == NU_NULL)
    {
        /* Set status to null pointer. */
        status = I2C_NULL_GIVEN_FOR_INIT;
    }

    /* Check if memory pool pointer is valid. */
    else if (i2c_init->i2c_memory_pool == NU_NULL)
    {
        /* Set status to null memory pointer. */
        status = I2C_NULL_GIVEN_FOR_MEM_POOL;
    }
    /* Check if the specified driver mode is valid. */
    else if (!((i2c_init->i2c_driver_mode == I2C_POLLING_MODE) ||
             (i2c_init->i2c_driver_mode == I2C_INTERRUPT_MODE)))
    {
        /* Set the status to indicate invalid operating mode for
           the driver. */
        status = I2C_INVALID_DRIVER_MODE;
    }

    /* Check if node type is not valid. */
    else if(!((i2c_init->i2c_node_address.i2c_node_type== I2C_SLAVE_NODE)||
           (i2c_init->i2c_node_address.i2c_node_type == I2C_MASTER_NODE) ||
           (i2c_init->i2c_node_address.i2c_node_type == I2C_MASTER_SLAVE)))
    {
        /* Set status to indicate invalid node type. */
        status = I2C_INVALID_NODE_TYPE;
    }

    /* Check if node will have the slave functionality. */
    else if(i2c_init->i2c_node_address.i2c_node_type & I2C_SLAVE_NODE)
    {
        /* Check slave address. */
        status = I2CS_Check_Slave_Address(i2c_init->i2c_node_address);
    }

    else
    {
        /* Empty else for MISRA standard compliance. */
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CS_Check_Slave_Address
*
* DESCRIPTION
*
*       This function checks the validity of the given slave address.
*
* INPUTS
*
*       slave_address                       Slave address to check.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_SLAVE_ADDRESS           The given slave address is not
*                                           valid.
*
*       I2C_INVALID_ADDRESS_TYPE            Type of the slave address is
*                                           neither 7-bit nor 10-bit.
*
*************************************************************************/
STATUS  I2CS_Check_Slave_Address (I2C_NODE slave_address)
{
    STATUS      status = NU_SUCCESS;

    /* Check if address type is 7 bit. */
    if (slave_address.i2c_address_type == I2C_7BIT_ADDRESS)
    {
        /* In case of 7 bit addressing scheme for I2C slave address, Two groups of eight addresses (0000 XXX and 1111 XXX)
        are reserved. However, If it is known that the reserved address is never going to be used for its intended purpose,
        a reserved address can be used for a slave address. */
        
#if CFG_NU_OS_CONN_I2C_ALLOW_RESERVED_ADDRESSES == NU_FALSE

        /* Check if the address falls into valid range. */
        if ((slave_address.i2c_slave_address < I2C_MIN_7BIT_VALID_ADDRESS) ||
            (slave_address.i2c_slave_address > I2C_MAX_7BIT_VALID_ADDRESS))
        {
            /* Set status to indicate that this is reserved address. */
            status = I2C_INVALID_SLAVE_ADDRESS;
        }
#endif
    }

    /* Check if address type is 10-bit. */
    else if (slave_address.i2c_address_type == I2C_10BIT_ADDRESS)
    {
        /* Check if the address falls into valid range. */
        if (slave_address.i2c_slave_address > I2C_MAX_10BIT_VALID_ADDRESS)
        {
            /* Set status to indicate that this is reserved address. */
            status = I2C_INVALID_SLAVE_ADDRESS;
        }
    }

    else
    {
        /* Set status to indicate invalid address type. */
        status = I2C_INVALID_ADDRESS_TYPE;
    }

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* NU_I2C_ERROR_CHECKING */

