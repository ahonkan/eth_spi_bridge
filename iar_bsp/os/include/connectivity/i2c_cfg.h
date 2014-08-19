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
*       i2c_cfg.h
*
* COMPONENT
*
*       Nucleus I2C Configuration
*
* DESCRIPTION
*
*       This file contains the configuration options for Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/

/* Check to avoid multiple file inclusion. */
#ifndef     I2C_CFG_H
#define     I2C_CFG_H

/* DEFINE:      NU_I2C_ERROR_CHECKING
   DEFAULT:     NU_I2C_ERROR_CHECKING is set to 1.
   DESCRIPTION: This option allows the user to enable/disable the
                error checking for the parameters.
                It is not allowed to set value of this macro directly, 
                use I2C metadata option "err_check_enable" to enable/disable 
                this option.
   NOTE:        1) Nucleus I2C, Nucleus I2C Driver and Nucleus I2C
                   demonstration must be built after changing this
                   options. */
#define     NU_I2C_ERROR_CHECKING           CFG_NU_OS_CONN_I2C_ERR_CHECK_ENABLE

/* DEFINE:      NU_I2C_SUPPORT_POLLING_MODE
   DEFAULT:     NU_I2C_SUPPORT_POLLING_MODE is set to 0.
   DESCRIPTION: This option provides the option to support polling in
                Nucleus I2C. If none of the device uses polling setting
                this option to '0' would greatly help in reducing the
                code size.
                It is not allowed to set value of this macro directly, 
                use I2C metadata option "polling_mode_enable" to enable/disable 
                this option.
   NOTE:        1) Nucleus I2C, Nucleus I2C Driver and Nucleus I2C
                   demonstration must be built after changing this
                   options.
                2) Setting this define to '1' does not mean that the
                   devices will start operating in the polling mode.
                   It only enables Nucleus I2C to support the drives
                   running in polling mode. Actual operating mode of
                   the driver should be specified at the
                   initialization time through i2c_driver_mode field
                   of the initialization structure. */
#define     NU_I2C_SUPPORT_POLLING_MODE     CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE

/* DEFINE:      NU_I2C_SUPPORT_FINE_CONTROL_API
   DEFAULT:     NU_I2C_SUPPORT_FINE_CONTROL_API is set to 0.
   DESCRIPTION: This option provides the option to support fine control API.
                It is not allowed to set value of this macro directly, 
                use I2C metadata option "fine_control_api_enable" to enable/disable 
                this option.   
   NOTE:        1) Nucleus I2C, Nucleus I2C Driver and Nucleus I2C
                   demonstration must be built after changing this
                   options.
*/
#define     NU_I2C_SUPPORT_FINE_CONTROL_API CFG_NU_OS_CONN_I2C_FINE_CONTROL_API_ENABLE

/* DEFINE:      NU_I2C_ACK_WAIT
   DEFAULT:     NU_I2C_ACK_WAIT is set to 1 timer tick.
   DESCRIPTION: This option allows the user to configure the time period
                the master should wait before aborting the wait for an
                acknowledgment from the slave.
   NOTE:        1) Nucleus I2C, Nucleus I2C Driver and Nucleus I2C
                   demonstration must be built after changing this
                   options.
                2) This option is valid for I2C master device only.
                3) This option is valid only in the polling mode. */
#define     NU_I2C_ACK_WAIT                 1

/* Possible values for NU_I2C_NODE_TYPE. */

#define     I2C_SLAVE_NODE                 0x01
#define     I2C_MASTER_NODE                0x02
#define     I2C_MASTER_SLAVE               (I2C_SLAVE_NODE | I2C_MASTER_NODE)

/* DEFINE:      NU_I2C_NODE_TYPE
   DEFAULT:     NU_I2C_NODE_TYPE is set to I2C_MASTER_SLAVE.
   DESCRIPTION: This option allows the user to scale the code as per the
                functionality (master or slave or both) the node will
                be able to provide.
   NOTE:        1) Nucleus I2C, Hardware Driver for Nucleus I2C and
                   Nucleus I2C demonstration must be built after
                   changing this options. */
#define     NU_I2C_NODE_TYPE                I2C_MASTER_SLAVE

/* Default baud rate for Nucleus I2C (in kbps). */
#define     I2C_DEFAULT_BAUDRATE            100

/* Default port ID for Nucleus I2C application. */
#define     I2C_DEFAULT_PORT                0

/* Default driver ID for Nucleus I2C application. */
#define     I2C_DEFAULT_DEVICE              0

/* Default slave address for the node. */

#define     I2C_DEFAULT_SLAVE_ADDRESS       0x22
#define     I2C_DEFAULT_ADDRESS_TYPE        I2C_7BIT_ADDRESS
#define     I2C_DEFAULT_NODE_TYPE           I2C_MASTER_SLAVE

#endif      /* !I2C_CFG_H */
