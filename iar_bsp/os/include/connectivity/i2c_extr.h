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
*       i2c_extr.h
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains the function prototypes for Nucleus I2C
*       services that are common both to a slave and a master.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2c_defs.h                          Main internal definition file
*                                           for Nucleus I2C.
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2C_EXTR_H
#define     I2C_EXTR_H

#include    "connectivity/i2c_defs.h"

INT NU_I2C_Open (DV_DEV_LABEL *i2c_controller_name, I2C_HANDLE *p_i2c_handle, I2C_INIT  *i2c_init);

/* Function prototype for Nucleus I2C initialization. */
STATUS  I2C_Start               (I2C_HANDLE    *i2c_handle,
                                 I2C_INIT      *i2c_init);

/* Function prototype for Nucleus I2C close up. */
STATUS  I2C_Close               (I2C_HANDLE     i2c_handle);

/* Function prototype for receiving data from the buffer. */
STATUS  I2CC_Receive_Data       (I2C_HANDLE     i2c_handle,
                                 UINT8         *data,
                                 UNSIGNED_INT   length);

/* Common function prototypes for master/slave. */

STATUS  I2CC_Check_Ack          (I2C_HANDLE     i2c_handle);
STATUS  I2CC_Get_Node_State     (I2C_HANDLE     i2c_handle,
                                 UINT8         *node_state);

/* Function prototype for the interface to I2C driver I/O control
   function. */
STATUS  I2CC_Ioctl_Driver       (I2C_HANDLE     i2c_handle,
                                 UINT8          operation_code,
                                 VOID          *operation_data);

/* Function to activate the specified I2C handler. */
VOID        I2C_Handler             (I2C_CB        *i2c_cb,
                                     BOOLEAN        use_hisr);

/* Entry point for I2C transmission/reception handling routines. */
VOID I2C_Handler_Entry(VOID);

/* Function for getting callback structure for the specified slave
   device. */
I2C_APP_CALLBACKS  *I2CMS_Get_Callbacks_Struct(I2C_CB *i2c_cb,
                                               UINT16  i2c_slave_address);

/* Function for deleting all the callbacks. */
STATUS          I2CMS_Delete_All_Callbacks(I2C_CB *i2c_cb);

/* Lookup a control block matching a I2C controller id */
I2C_CB* I2C_Get_CB (DV_DEV_ID dev_id);

#if         (NU_I2C_ERROR_CHECKING)

STATUS  I2CS_Check_Init_Params  (I2C_INIT      *i2c_init);
STATUS  I2CS_Check_Slave_Address(I2C_NODE       slave_address);

#else

#define     I2CS_Check_Init_Params(x)               NU_SUCCESS
#define     I2CS_Check_Slave_Address(x)             NU_SUCCESS

#endif      /* NU_I2C_ERROR_CHECKING */

#endif      /* !I2C_EXTR_H */
