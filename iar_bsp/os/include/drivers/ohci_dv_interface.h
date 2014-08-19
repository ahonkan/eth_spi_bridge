/**************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       ohci_dv_interface.h
*
*
* COMPONENT
*
*       USB OHCI Controller Driver : Nucleus USB Software.
*
* DESCRIPTION
*
*       This is the platform specific file of Nucleus USB OHCI driver.
*
* DATA STRUCTURES
*
*       OHCI_TGT_INFO
*       OHCI_SESSION_HANDLE
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nucleus.h
*
**************************************************************************/
#include "nucleus.h"
#include "nu_usbh_ohci_ext.h"

#ifndef _USBH_OHCI_H
#define _USBH_OHCI_H

#define OHCI_PM

#define OHCI_MAX_INSTANCES              3
 
/* Followind defines pool sizes to be used for OHCI. */
#define OHCI_CACHED_POOL_SIZE           ( 16 * 1024 )
#define OHCI_UNCACHED_POOL_SIZE         ( 16 * 1024 )

/* Open Mode */
#define USBHHW_OPEN_MODE                0x1
#define POWER_OPEN_MODE                 0x2
#define UII_OPEN_MODE                   0x4

/* OHCI Power Base */
#define OHCI_POWER_BASE                 (NU_USB_IOCTL_BASE + TOTAL_USB_IOCTLS + 32)

/* OHCI UII Base */
#define OHCI_UII_BASE                  (OHCI_POWER_BASE + POWER_IOCTL_TOTAL)

/* Power States */
#define OHCI_OFF                        0
#define OHCI_SLEEP                      1
#define OHCI_ON                         2

/* EMAC total power states */
#define OHCI_TOTAL_STATE_COUNT          3
#define OHCI_MIN_OP                     1

/* Maximum label count for OHCI. */
#define OHCI_MAX_LABEL_CNT              5

/* OHCI DV interface error codes. */
#define     OHCI_PMS_ERROR              -12
#define     OHCI_NO_INST_AVAILABLE      -13
#define     OHCI_SESS_NOT_FOUND         -14
#define     OHCI_REGISTRY_ERROR         -15
#define     OHCI_DEVICE_PWR_STATE_OFF   -16


/* This structure contains the target specific information for OHCI
 * controller.
 */
typedef struct _usbh_ohci_tgt_info
{
    CHAR                    name[10];
    UINT32                  base_address;
    INT                     irq;
    INT                     irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    UINT32                  total_current;
}USBH_OHCI_TGT_INFO;

/* This structure serves as a instance handle for OHCI function controller
 * driver.
 */
typedef struct _usbh_ohci_instance_handle
{
    USBH_OHCI_TGT_INFO     *tgt_info;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))
    VOID                   *config_path;
#endif
    VOID                    (*setup_func)(VOID);
    VOID                    (*cleanup_func)(VOID);
    DV_DEV_ID               device_id;
    BOOLEAN                 device_in_use;
    VOID                    *usbh_reserved;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

}USBH_OHCI_INSTANCE_HANDLE;

/* This structure serves as a session handle for OHCI controller driver.
 */
typedef struct _usbh_ohci_session_handle
{
    NU_USBH_OHCI                ohci_cb;
    NU_MEMORY_POOL              ohci_cached_pool;
    NU_MEMORY_POOL              ohci_uncached_pool;
    USBH_OHCI_INSTANCE_HANDLE  *ohci_inst_handle;
    VOID                       *cached_mem_ptr;
    VOID                       *uncached_mem_ptr;
    UINT32                      open_modes;
    BOOLEAN                     is_clock_enabled;
    BOOLEAN                     is_device_init;

    UINT8                       padding[2];
}USBH_OHCI_SESSION_HANDLE;


STATUS USBH_OHCI_Register(const CHAR*, INT, DV_DEV_ID*);
STATUS USBH_OHCI_Unregister(const CHAR*, INT, DV_DEV_ID);

/* Device Manager APIs definition. */
STATUS USBH_OHCI_Open(VOID*, DV_DEV_LABEL[], INT, VOID**);
STATUS USBH_OHCI_Close(VOID*);
STATUS USBH_OHCI_Read(VOID*, VOID*, UINT32, OFFSET_T, UINT32*);
STATUS USBH_OHCI_Write(VOID*, const VOID*, UINT32, OFFSET_T, UINT32*);
STATUS USBH_OHCI_Ioctl(VOID*, INT, VOID*, INT);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))
STATUS OHCI_HWCTRL_Hibernate (const CHAR * key);
STATUS OHCI_HWCTRL_Resume(const CHAR * key);
#endif

/* IOCTL command handlers for OHCI controller driver.*/
extern STATUS OHCI_Handle_Initialize(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Uninitialize(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_IO_Request(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Open_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Close_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Modify_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Flush_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Enable_Int(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Disable_Int(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Execute_ISR(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Get_CB(USBH_OHCI_SESSION_HANDLE*, VOID**, INT);
extern STATUS OHCI_Handle_Get_Speed(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Is_Current_Available(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Release_Power(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Handle_Request_Power_Down(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS OHCI_Get_Target_Info(const CHAR * key, USBH_OHCI_TGT_INFO *usbh_ohci_tgt);

#endif /* _USBH_OHCI_H */

/* ======================  End Of File  ================================ */
