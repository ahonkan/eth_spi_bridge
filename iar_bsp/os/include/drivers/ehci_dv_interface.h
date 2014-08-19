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
*       ehci_dv_interface.h
*
*
* COMPONENT
*
*       USB EHCI Controller Driver : Nucleus USB Software.
*
* DESCRIPTION
*
*       This is the platform specific file of Nucleus USB EHCI driver.
*
* DATA STRUCTURES
*
*       EHCI_TGT_INFO
*       USBH_EHCI_INSTANCE_HANDLE
*       EHCI_SESSION_HANDLE
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_usbh_ehci_ext.h
*
**************************************************************************/
#include "nucleus.h"
#include "nu_usbh_ehci_ext.h"

#ifndef _USBH_EHCI_H
#define _USBH_EHCI_H

#define EHCI_PM 

#define EHCI_MAX_INSTANCES              3
 
/* Followind defines pool sizes to be used for EHCI. */
#define EHCI_CACHED_POOL_SIZE           ( 16 * 1024 )
#define EHCI_UNCACHED_POOL_SIZE         ( 32 * 1024 )

/* Open Mode */
#define USBHHW_OPEN_MODE                0x1
#define POWER_OPEN_MODE                 0x2
#define UII_OPEN_MODE                   0x4

/* EHCI Power Base */
#define EHCI_POWER_BASE                 (NU_USB_IOCTL_BASE + TOTAL_USB_IOCTLS + 32)

/* EHCI UII Base */
#define EHCI_UII_BASE                  (EHCI_POWER_BASE + POWER_IOCTL_TOTAL)

/* Power States */
#define EHCI_OFF                        0    
#define EHCI_SLEEP                      1 
#define EHCI_ON                         2 

/* EMAC total power states */
#define EHCI_TOTAL_STATE_COUNT          3
#define   EHCI_MIN_OP                   1

/* Maximum label count for EHCI. */
#define EHCI_MAX_LABEL_CNT              5

/* EHCI DV interface error codes. */
#define     EHCI_PMS_ERROR              -12
#define     EHCI_NO_INST_AVAILABLE      -13
#define     EHCI_SESS_NOT_FOUND         -14
#define     EHCI_REGISTRY_ERROR         -15
#define     EHCI_INVLD_PWR_STATE        -16

/* This structure contains the target specific information for EHCI 
 * controller.
 */
typedef struct _usbh_ehci_tgt_info
{
    CHAR                    name[10];
    UINT32                  base_address;
    INT                     irq;
    INT                     irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    UINT32                  total_current;
}USBH_EHCI_TGT_INFO;

/* This structure serves as a instance handle for EHCI function controller
 * driver.
 */
typedef struct _usbh_ehci_instance_handle
{
    USBH_EHCI_TGT_INFO     *tgt_info;
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

}USBH_EHCI_INSTANCE_HANDLE;

/* This structure serves as a session handle for EHCI controller driver.
 */
typedef struct _usbh_ehci_session_handle
{
    NU_USBH_EHCI                ehci_cb;
    NU_MEMORY_POOL              ehci_cached_pool;
    NU_MEMORY_POOL              ehci_uncached_pool;
    USBH_EHCI_INSTANCE_HANDLE  *ehci_inst_handle;
    VOID                       *cached_mem_ptr;
    VOID                       *uncached_mem_ptr;
    UINT32                      open_modes;
    BOOLEAN                     is_clock_enabled;
    BOOLEAN                     is_device_init;
    UINT8                       padding[2];
}USBH_EHCI_SESSION_HANDLE;


STATUS USBH_EHCI_Register(const CHAR*, INT, DV_DEV_ID*);
STATUS USBH_EHCI_Unregister(const CHAR*, INT, DV_DEV_ID);

/* Device Manager APIs definition. */
STATUS USBH_EHCI_Open(VOID*, DV_DEV_LABEL[], INT, VOID**);
STATUS USBH_EHCI_Close(VOID*);
STATUS USBH_EHCI_Read(VOID*, VOID*, UINT32, OFFSET_T, UINT32*);
STATUS USBH_EHCI_Write(VOID*, const VOID*, UINT32, OFFSET_T, UINT32*);
STATUS USBH_EHCI_Ioctl(VOID*, INT, VOID*, INT);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))
STATUS EHCI_HWCTRL_Hibernate (const CHAR * key);
STATUS EHCI_HWCTRL_Resume(const CHAR * key);
#endif

/* IOCTL command handlers for EHCI controller driver.*/
extern STATUS EHCI_Handle_Initialize(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Uninitialize(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_IO_Request(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Open_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Close_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Modify_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Flush_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Enable_Int(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Disable_Int(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Execute_ISR(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Get_CB(USBH_EHCI_SESSION_HANDLE*, VOID**, INT);
extern STATUS EHCI_Handle_Get_Speed(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Is_Current_Available(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Release_Power(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Handle_Request_Power_Down(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS EHCI_Get_Target_Info(const CHAR * key, USBH_EHCI_TGT_INFO *usbh_ehci_tgt);
#endif /* _USBH_EHCI_H */

/* ======================  End Of File  ================================ */
