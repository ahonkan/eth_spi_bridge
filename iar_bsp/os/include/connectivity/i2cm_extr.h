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
*       i2cm_extr.h
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the function prototypes for Nucleus I2C
*       master.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       i2cm_defs.h                         Main definition file for
*                                           Nucleus I2C master.
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C.
*
*************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     I2CM_EXTR_H
#define     I2CM_EXTR_H

#include    "connectivity/i2cm_defs.h"
#include    "connectivity/i2c_extr.h"

/* Function prototypes for Nucleus I2C master functions. */

STATUS  I2CMC_Read               (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8        *data,
                                  UNSIGNED_INT  length);
STATUS  I2CMC_Write              (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8        *data,
                                  UNSIGNED_INT  length);
STATUS  I2CMC_Multi_Transfer     (I2C_HANDLE    i2c_handle,
                                  I2C_NODE     *slaves,
                                  UINT8        *tx_data,
                                  UINT8        *rx_data,
                                  UNSIGNED_INT *lengths,
                                  UINT8        *rw,
                                  UINT8         slave_count);
STATUS  I2CMC_Process_Next_Slave (I2C_HANDLE    i2c_handle);
STATUS  I2CMC_Start_Transfer     (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8         rw);
STATUS  I2CMC_Stop_Transfer      (I2C_HANDLE    i2c_handle);

STATUS  I2CMC_Write_Byte         (I2C_HANDLE    i2c_handle,
                                  UINT8         data);
STATUS  I2CMC_Read_Byte          (I2C_HANDLE    i2c_handle,
                                  UINT8        *data);
STATUS  I2CMC_Restart            (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8         rw);
STATUS  I2CMC_Send_Start         (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8         rw,
                                  BOOLEAN       restart);
STATUS  I2CMC_Send_Stop          (I2C_HANDLE    i2c_handle);
STATUS  I2CMS_Send_Address_Byte2 (I2C_HANDLE    i2c_handle);
STATUS  I2CMS_Set_Baudrate       (I2C_HANDLE    i2c_handle,
                                  UINT16        baudrate);
STATUS  I2CMS_Get_Baudrate       (I2C_HANDLE    i2c_handle,
                                  UINT16       *baudrate);
STATUS  I2CMC_Process_Write      (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8        *data,
                                  UNSIGNED_INT  length,
                                  BOOLEAN       restart);
STATUS I2CMC_Process_Read        (I2C_HANDLE    i2c_handle,
                                  I2C_NODE      slave,
                                  UINT8        *data,
                                  UNSIGNED_INT  length,
                                  BOOLEAN       restart);
STATUS I2CMC_Process_Transfer    (I2C_HANDLE    i2c_handle,
                                  BOOLEAN       restart);
STATUS  I2CMC_Set_Slave_Address  (I2C_HANDLE    i2c_handle,
                                  BOOLEAN       reset);
STATUS  I2CMC_Config_HW_Master   (I2C_HANDLE    i2c_handle,
                                  UINT8         hw_master_address,
                                  UINT8         hw_master_slave);

#endif      /* !I2CM_EXTR_H */
