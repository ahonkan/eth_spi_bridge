/**************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       usbh_hwctrl_dv_interface.h
*
* COMPONENT
*
*       Generic USB Host Controller Driver
*
* DESCRIPTION
*
*       This file contains definition of generic data structures of USB
*       host controller drivers.
*
* DATA STRUCTURES
*
*       USBH_HWCTRL_TGT_INFO
*       USBH_HWCTRL_INSTANCE_HANDLE
*       USBH_HWCTRL_SESSION_HANDLE
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nucleus.h
*
**************************************************************************/
#ifndef USBH_HWCTRL_DV_INTERFACE_H
#define USBH_HWCTRL_DV_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#include "nucleus.h"

#define USBH_MAX_INSTANCES                  3                   

/* Open Modes. */
#define USBH_HWCTRL_OPEN_MODE               0x1
#define POWER_OPEN_MODE                     0x2
#define UII_OPEN_MODE                       0x4

/* USBH controller Power Base. */
#define USBH_HWCTRL_POWER_BASE              (NU_USB_IOCTL_BASE + TOTAL_USB_IOCTLS + 32)

/* USBH controller UII Base. */
#define USBH_HWCTRL_UII_BASE                (USBH_HWCTRL_POWER_BASE + POWER_IOCTL_TOTAL)

/* Power States. */
#define USBH_HWCTRL_OFF                     0
#define USBH_HWCTRL_SLEEP                   1
#define USBH_HWCTRL_ON                      2

/* USBH HW total power states. */
#define USBH_HWCTRL_TOTAL_STATE_COUNT       3
#define USBH_HWCTRL_MIN_OP                  1

/* Maximum label count for USBH HW controller. */
#define USBH_HWCTRL_MAX_LABEL_CNT           5

/* USBH controller DV interface error codes. */
#define     USBH_HWCTRL_PMS_ERROR           -12
#define     USBH_HWCTRL_NO_INST_AVAILABLE   -13
#define     USBH_HWCTRL_SESS_NOT_FOUND      -14
#define     USBH_HWCTRL_REGISTRY_ERROR      -15

/* Sizes for USB host controller driver specific memory pools. */
#define     USBH_HWCTRL_CACHED_POOL_SIZE    CFG_NU_OS_DRVR_USB_HOST_HWCTRL_CACHED_POOL_SIZE
#define     USBH_HWCTRL_UNCACHED_POOL_SIZE  CFG_NU_OS_DRVR_USB_HOST_HWCTRL_UNCACHED_POOL_SIZE

/* This structure contains the target specific information for USBH HW 
 * controller.
 */
typedef struct _usbh_hwctrl_tgt_info
{
    CHAR                    name[10];
    UINT32                  base_address;
    INT                     irq;
    INT                     irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    UINT32                  total_current;
}USBH_HWCTRL_TGT_INFO;

/* This structure serves as a instance handle for USBH HW controller
 * driver.
 */
typedef struct _usbh_hwctrl_instance_handle
{
    USBH_HWCTRL_TGT_INFO   *tgt_info;
    VOID                   *config_path;
    VOID                    (*setup_func)(VOID);
    VOID                    (*cleanup_func)(VOID);
    DV_DEV_ID               device_id;
    BOOLEAN                 device_in_use;
    VOID                   *usbh_reserved;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

}USBH_HWCTRL_INSTANCE_HANDLE;

/* This structure serves as a session handle for USBH HW controller driver.
 */
typedef struct _usbh_hwctrl_session_handle
{
    VOID                        *usbh_hwctrl_cb;
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
    NU_MEMORY_POOL               usbh_hwctrl_cached_pool;
    NU_MEMORY_POOL               usbh_hwctrl_uncached_pool;
    VOID                        *cached_mem_ptr;
    VOID                        *uncached_mem_ptr;
#endif
    USBH_HWCTRL_INSTANCE_HANDLE *usbh_hwctrl_inst_handle;
    UINT32                       open_modes;
    BOOLEAN                      is_clock_enabled;

    UINT8                       padding[3];
}USBH_HWCTRL_SESSION_HANDLE;

STATUS USBH_HWCTRL_Register(const CHAR*, INT, DV_DEV_ID*);
STATUS USBH_HWCTRL_Unregister(const CHAR*, INT, DV_DEV_ID);

/* Device Manager APIs definition. */
STATUS USBH_HWCTRL_Open(VOID*, DV_DEV_LABEL[], INT, VOID**);
STATUS USBH_HWCTRL_Close(VOID*);
STATUS USBH_HWCTRL_Ioctl(VOID*, INT, VOID*, INT);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))
STATUS   USBH_HWCTRL_Hibernate (const CHAR * key);
STATUS   USBH_HWCTRL_Resume(const CHAR * key);
#endif


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* USBH_HWCTRL_DV_INTERFACE_H */

/* ======================  End Of File  ================================ */
