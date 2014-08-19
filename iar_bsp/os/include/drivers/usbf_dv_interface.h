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
*       usbf_dv_interface.h
*
*   COMPONENT
*
*       USBF                                - USBF Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the USBF Library Driver module.
*
*************************************************************************/
#ifndef USBF_DV_INTERFACE_H
#define USBF_DV_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#define USBF_HW_MAX_INSTANCES           1
#define USBF_HW_MAX_SESSIONS            (3 * USBF_HW_MAX_INSTANCES)

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
/* USB Function Device Connection / Disconnection event. */
#define     USBF_DEVICE_CONNECT         0x1
#define     USBF_DEVICE_DISCONNECT      0x2
#endif

#define     USBF_PNP_TASK_MAX_STACK     512

#define     USBF_MIN_STATE              PC_SYSTEM_INACTIVE

/* Max number of USBF labels */
#define USBF_MAX_LABEL_CNT              5

/* USBF error codes */
#define USBF_REGISTRY_ERROR             -1
#define USBF_ALREADY_OPEN               -2
#define USBF_NO_INST_AVAILABLE          -3

/*********************************/
/* Data Structures               */
/*********************************/

/* This structure serves as a instance handle for 926x function controller
 * driver.
 */
typedef struct _usbf_instance_handle
{
    CHAR                    name[10];
    UINT32                  capability;
    UINT8                   speed;
    UINT32                  base_address;
    UINT8                   num_irq;
    INT                     irq;
    INT                     irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    CHAR                    ref_clock[NU_DRVR_REF_CLOCK_LEN];
    BOOLEAN                 device_in_use;
    BOOLEAN                 is_parked;
    NU_EVENT_GROUP          pwr_state_evt_grp;
    PM_DVFS_HANDLE          usbf_dvfs_handle;
    PM_MIN_REQ_HANDLE       min_op_request_handle;
    NU_TASK                 pnp_task;
    NU_EVENT_GROUP          pnp_event;
    DV_DEV_ID               device_id;
    VOID                   *setup_func;
    VOID                   *cleanup_func;
    VOID                   *usbf_reserved;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
}USBF_INSTANCE_HANDLE;

/* This structure serves as a session handle for 926x function controller
 * driver.
 */


typedef struct _usbf_session_handle
{
    VOID                    *usbf_cb;
    USBF_INSTANCE_HANDLE    *instance_handle;
    NU_USBF_STACK           *usbf_stack_ptr;
    UINT32                  open_modes;
}USBF_SESSION_HANDLE;

/*********************************/
/* MACROS                        */
/*********************************/
#define USBFHW_OPEN_MODE                0x1

/*********************************/
/*   POWER STATE MACROS          */
/*********************************/
/* Power States */
#define USBF_OFF                        0
#define USBF_ON                         1

/* USBF total power states */
#define USBF_TOTAL_STATE_COUNT          2

/***********************/
/* USBF Power Base   */
/***********************/
#define USBF_POWER_BASE             (NU_USB_IOCTL_BASE + TOTAL_USB_IOCTLS + TOTAL_USBF_IOCTLS + 32)

/***********************/
/* USBF UII Base     */
/***********************/
#define USBF_UII_BASE               (USBF_POWER_BASE + POWER_IOCTL_TOTAL)

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS USBF_Dv_Register(const CHAR * key, INT startorstop, DV_DEV_ID *dev_id,
                               USBF_INSTANCE_HANDLE *instance_handle);
STATUS USBF_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS USBF_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[],
                    INT label_cnt, VOID **session_handle);
STATUS USBF_Dv_Close(VOID *session_handle);
STATUS USBF_Dv_Ioctl(void *session_handle, INT ioctl_cmd, VOID *ioctl_data, INT ioctl_data_len);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif      /* USBF_DV_INTERFACE_H */

