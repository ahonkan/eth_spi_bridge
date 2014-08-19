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
*       i2cms_10bit.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the support functions for transmitting 10-bit
*       slave address from Nucleus I2C master device.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMS_Send_Address_Byte2            Sends second byte of the
*                                           10-bit slave address.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define    NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CMS_Send_Address_Byte2
*
* DESCRIPTION
*
*       This function sends the second byte of the 10-bit slave address.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_ADDRESS_TX_FAILED               Master could not address the
*                                           slave node perhaps due to the
*                                           reason that it lost
*                                           arbitration.
*
*************************************************************************/
STATUS  I2CMS_Send_Address_Byte2  (I2C_HANDLE i2c_handle)
{
    STATUS          status;
    I2C_CB          *i2c_cb;
    UINT16          address2;

    I2CS_Get_CB(i2c_handle, &i2c_cb);
    
    address2 =  i2c_cb->i2c_10bit_address2;

    /* Get low byte of 10-bit address and call the driver
       service to send it. */
    status = I2CC_Ioctl_Driver(i2c_handle,
                                I2C_SEND_ADDRESS2, &address2);

    /* Set node state to waiting address acknowledgment. */
    i2c_cb->i2c_node_state = I2C_WAITING_ADDRESS_ACK;

    /* Return the completion status of the service. */
    return (status);
}
