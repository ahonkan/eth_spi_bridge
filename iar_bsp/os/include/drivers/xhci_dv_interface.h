/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       xhci_dv_interface.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains definitions which are being used to
*       interface with DM.
*
* DATA STRUCTURES
*
*       USBH_XHCI_TGT_INFO
*       USBH_XHCI_INSTANCE_HANDLE
*       USBH_XHCI_SESSION_HANDLE
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nucleus.h
*       power_core.h
*
**************************************************************************/
#include "nucleus.h"
#include "nu_usbh_xhci_ext.h"

#ifndef _USBH_XHCI_DVM_H
#define _USBH_XHCI_DVM_H

#define XHCI_PM

/* Followind defines pool sizes to be used for XHCI. */
#define XHCI_CACHED_POOL_SIZE               ( 32 * 1024 )
#define XHCI_UNCACHED_POOL_SIZE             ( 32 * 1024 )

/* Open Mode */
#define USBHHW_OPEN_MODE                    0x1
#define POWER_OPEN_MODE                     0x2
#define UII_OPEN_MODE                       0x4

/* XHCI Power Base */
#define XHCI_POWER_BASE                     (NU_USB_IOCTL_BASE + TOTAL_USB_IOCTLS + 32)

/* XHCI UII Base */
#define XHCI_UII_BASE                       (XHCI_POWER_BASE + POWER_IOCTL_TOTAL)

/* Power States */
#define XHCI_OFF                            0
#define XHCI_SLEEP                          1
#define XHCI_ON                             2

/* XHCI total power states */
#define XHCI_TOTAL_STATE_COUNT              3
#define XHCI_MIN_OP                         1

/* Maximum label count for XHCI. */
#define XHCI_MAX_LABEL_CNT                  5

/* xHCI DV interface error codes. */
#define XHCI_PMS_ERROR                      -12
#define XHCI_NO_INST_AVAILABLE              -13
#define XHCI_SESS_NOT_FOUND                 -14
#define XHCI_REGISTRY_ERROR                 -15

#define ON_CHIP                             0
#define ON_PCI                              1

#define USB_XHCI_CLASS	                    0x0C033000
#define USB_PCI_MATCH_CLASS                 (1 << 0)

/* This structure contains the target specific information for XHCI
 * controller.
 */
typedef struct _usbh_xhci_tgt_info
{
    CHAR                    name[10];
    UINT32                   hw_location;
    UINT32                  base_address;
    INT                     irq;
    INT                     irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    UINT32                  total_current;

}USBH_XHCI_TGT_INFO;


/* This structure serves as a instance handle for XHCI function controller
 * driver.
 */
typedef struct _usbh_xhci_instance_handle
{
    USBH_XHCI_TGT_INFO   *tgt_info;
    VOID                  (*setup_func)(VOID);
    VOID                  (*cleanup_func)(VOID);
    DV_DEV_ID             device_id;
    BOOLEAN                 device_in_use;
    VOID                    *usbh_reserved;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
    DV_DEV_HANDLE         xhci_pwr_handle;
    INT                   xhci_pwr_ioctl_base_addr;

} USBH_XHCI_INSTANCE_HANDLE;

/* This structure serves as a session handle for XHCI controller driver.
 */
typedef struct _usbh_xhci_session_handle
{
    NU_USBH_XHCI                xhci_cb;
    NU_MEMORY_POOL              xhci_cached_pool;
    NU_MEMORY_POOL              xhci_uncached_pool;
    USBH_XHCI_INSTANCE_HANDLE  *xhci_inst_handle;
    VOID                       *cached_mem_ptr;
    VOID                       *uncached_mem_ptr;
    UINT32                      open_modes;
    BOOLEAN                     is_clock_enabled;

    UINT8                       padding[3];
}USBH_XHCI_SESSION_HANDLE;

STATUS USBH_XHCI_Register(const CHAR*, INT, DV_DEV_ID*);
STATUS USBH_XHCI_Unregister(const CHAR*, INT, DV_DEV_ID);

/* Device Manager APIs definition. */
STATUS USBH_XHCI_Open(VOID*, DV_DEV_LABEL[], INT, VOID**);
STATUS USBH_XHCI_Close(VOID*);
STATUS USBH_XHCI_Read(VOID*, VOID*, UINT32, OFFSET_T, UINT32*);
STATUS USBH_XHCI_Write(VOID*, const VOID*, UINT32, OFFSET_T, UINT32*);
STATUS USBH_XHCI_Ioctl(VOID*, INT, VOID*, INT);

/* IOCTL command handlers for XHCI controller driver.*/
extern STATUS XHCI_Handle_Initialize(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_IO_Request(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Open_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Close_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Modify_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Flush_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Enable_Int(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Disable_Int(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Execute_ISR(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Open_SS_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Update_device(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Initialize_Device(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Uninitialize_Device(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Unstall_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Disable_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Reset_Bandwidth(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Get_CB(USBH_XHCI_SESSION_HANDLE*, VOID**, INT);
extern STATUS XHCI_Handle_Get_Speed(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Is_Current_Available(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Release_Power(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Handle_Request_Power_Down(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
extern STATUS XHCI_Get_Target_Info(const CHAR * key, USBH_XHCI_TGT_INFO *usbh_xhci_tgt);
extern STATUS USBH_XHCI_PWR_Drvr_Register_CB(DV_DEV_ID device_id, VOID *context);
extern STATUS USBH_XHCI_PWR_Drvr_Unregister_CB(DV_DEV_ID device_id, VOID *context);
extern STATUS USBH_XHCI_Set_State(VOID *inst_handle, PM_STATE_ID *state);
extern STATUS XHCI_Pwr_Pre_Park( VOID *inst_handle );
extern STATUS XHCI_Pwr_Post_Park( VOID *inst_handle );
extern STATUS XHCI_Pwr_Pre_Resume( VOID *inst_handle );
extern STATUS XHCI_Pwr_Post_Resume( VOID *inst_handle );
#endif /* _USBH_XHCI_DVM_H */

/* ===================================================================== */
