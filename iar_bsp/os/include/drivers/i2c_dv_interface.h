/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       i2c_dv_interface.h
*
*   COMPONENT
*
*       SERIAL                              - Serial Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Serial Library Driver module.
*
*************************************************************************/
#ifndef I2C_DV_INTERFACE_H
#define I2C_DV_INTERFACE_H

#include "connectivity/nu_connectivity.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#define I2C_MODE_IOCTL_BASE 0

/* TWI Error codes */
#define I2C_TOO_MANY_LABELS       -1
#define I2C_NO_INSTANCE_AVAILABLE -2
#define I2C_TGT_SETUP_FAILED      -3
#define I2C_NO_SESSION_AVAILABLE  -4

/* Define TWI instance and session info structure */

typedef struct  _i2c_instance_handle_struct
{
    UINT32                  io_addr;
    UINT32                  irq;
    UINT32                  irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    UINT32                  number;
    BOOLEAN                 device_in_use;
    DV_DEV_ID               dev_id;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
    I2C_CB*                 i2c_cb;
    VOID                    *i2c_reserved;
    CHAR                    ref_clock[NU_DRVR_REF_CLOCK_LEN];

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    UINT32                  bus_clock;
    PMI_DEV_HANDLE          pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

} I2C_INSTANCE_HANDLE;

typedef struct  _i2c_session_handle_struct
{
    UINT32      open_modes;
    UINT32      device_misc;
    I2C_INSTANCE_HANDLE *instance_handle;
} I2C_SESSION_HANDLE;

/* Open Modes */
#define I2C_OPEN_MODE                   0x1

#define I2C_GET_BASE_ADDRESS(s_handle)  ((UINT8*)(s_handle->instance_handle->io_addr))
#define I2C_GET_VECTOR(s_handle)        (s_handle->instance_handle->irq)
#define I2C_GET_PRIORITY(s_handle)      (s_handle->instance_handle->irq_priority & 0xff)
#define I2C_GET_CB_FROM_SESSION_HANDLE(s_handle) (s_handle->instance_handle->i2c_cb)

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/* Power States */
#define I2C_OFF                       0
#define I2C_ON                        1

/* I2C total power states */
#define I2C_TOTAL_POWER_STATE_COUNT   2

/* Minimum DVFS OP for I2C to perform correctly */
#define I2C_MIN_DVFS_OP               1

#define I2C_POWER_BASE          (I2C_MODE_IOCTL_BASE + I2C_CLASS_CMD_DELIMITER + 1)

#endif /*CFG_NU_OS_SVCS_PWR_ENABLE*/

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS I2C_Dv_Register (const CHAR * key, I2C_INSTANCE_HANDLE *instance_handle);
STATUS I2C_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS I2C_Dv_Open (VOID *i2c_inst_ptr_void, DV_DEV_LABEL labels_list[],
                    INT labels_cnt, VOID* *session_handle);
STATUS I2C_Dv_Close(VOID *session_ptr_void);
STATUS I2C_Dv_Ioctl(VOID *sess_handle_ptr_void, INT ioctl_cmd, VOID *data, INT length);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !I2C_DV_INTERFACE_H */

