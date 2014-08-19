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
*       serial_dv_interface.h
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
#ifndef SERIAL_DV_INTERFACE_H
#define SERIAL_DV_INTERFACE_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


/* Max number of serial labels */
#define SERIAL_MAX_LABEL_CNT              5
#define SERIAL_MAX_INSTANCES              CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED
#define SERIAL_MAX_SESSIONS               (1 * CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED)

/* SERIAL Error codes */
#define SERIAL_ERROR                     -1
#define SERIAL_TOO_MANY_LABELS           -2
#define SERIAL_NO_INSTANCE_AVAILABLE     -3
#define SERIAL_REGISTRY_ERROR            -4
#define SERIAL_ALREADY_OPEN              -5
#define SERIAL_SESSION_NOT_AVAILABLE     -6

/*********************************/
/* Data Structures               */
/*********************************/
typedef struct _serial_attr
{
    UINT32                  baud_rate;
    UINT32                  data_bits;
    UINT32                  stop_bits;
    UINT32                  parity;
    UINT32                  tx_mode;
    UINT32                  rx_mode;
    UINT32                  flow_ctrl;

} SERIAL_ATTR;

typedef struct  _serial_instance_handle_struct
{
    UINT32                  open_modes;
    DV_DEV_ID               dev_id;
    UINT32                  serial_io_addr;
    UINT32                  serial_clock;
    INT                     serial_tx_vector;
    INT                     serial_tx_irq_priority;
    ESAL_GE_INT_TRIG_TYPE   serial_tx_irq_type;
    INT                     serial_rx_vector;
    INT                     serial_rx_irq_priority;
    ESAL_GE_INT_TRIG_TYPE   serial_rx_irq_type;
    CHAR                    serial_ref_clock[NU_DRVR_REF_CLOCK_LEN];
    BOOLEAN                 device_in_use;
    SERIAL_ATTR             attrs;
    BOOLEAN                 tx_en_shadow;
    BOOLEAN                 tx_intr_en_shadow;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
    VOID                    *serial_reserved;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    NU_HISR                 rx_hisr;
    VOID*                   rx_hisr_stk_ptr;
    UINT16                  rx_wd_timeout;
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

} SERIAL_INSTANCE_HANDLE;

typedef struct  _serial_session_handle_struct
{
    SERIAL_INSTANCE_HANDLE  *instance_ptr;
    SERIAL_SESSION          *ser_mw_ptr;

} SERIAL_SESSION_HANDLE;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE))

/* Hibernate-Resume structure */
typedef struct  _serial_hib_res_struct
{
    PM_STATE_ID           device_state;
    DV_DEV_ID             device_id;
    VOID                  *device_reserved_1;
    VOID                  *device_reserved_2;
    VOID                  *device_reserved_3;
} SERIAL_HIB_RES;
#endif 

/* Useful macros to get base address */
#define SERIAL_BASE_FROM_I_HANDLE(inst_ptr) (((SERIAL_INSTANCE_HANDLE*)inst_ptr)->serial_io_addr)

/*********************************/
/* MACROS                        */
/*********************************/
#define SERIAL_OPEN_MODE            0x1

/* UII Hisr stack size */
#define SERIAL_UII_HISR_STK_SIZE    2048

/*********************************/
/*   POWER STATE MACROS          */
/*********************************/
/* Power States */
#define SERIAL_OFF                  0
#define SERIAL_ON                   1

/* Uart total power states */
#define SERIAL_TOTAL_STATE_COUNT    2

/***********************/
/* Serial Power Base   */
/***********************/
#define SERIAL_POWER_BASE           (IOCTL_SERIAL_BASE + SERIAL_CLASS_CMD_DELIMITER)

/***********************/
/* Serial UII Base     */
/***********************/
#define SERIAL_UII_BASE             (SERIAL_POWER_BASE + POWER_IOCTL_TOTAL)

/***********************/
/*  Serial IOCTL Base  */
/***********************/
#define IOCTL_SERIAL_BASE           (DV_IOCTL0+1)

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS  Serial_Dv_Register (const CHAR *key, SERIAL_INSTANCE_HANDLE *instance_handle);
STATUS  Serial_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS  Serial_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[],
                       INT label_cnt, VOID* *session_handle);
STATUS  Serial_Dv_Close(VOID *sess_handle);
STATUS  Serial_Dv_Ioctl(VOID *session_ptr, INT ioctl_cmd, VOID *data, INT length);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !SERIAL_DV_INTERFACE_H */
