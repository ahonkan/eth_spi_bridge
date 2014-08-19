/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_init_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Initialization
*
* DESCRIPTION
*
*       Contains header information surrounding the function stack
*       initialization.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_init_cfg.h         USB Initialization configuration
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBF_INIT_EXT_H
#define     _NU_USBF_INIT_EXT_H

/* ===================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

#include "connectivity/nu_usbf_init_cfg.h"

/* ====================  Function Prototypes =========================== */

STATUS nu_os_conn_usb_func_stack_init(CHAR *path, INT startstop);

STATUS NU_USBF_Init(NU_MEMORY_POOL *USB_Cached_Pool,
                    NU_MEMORY_POOL *USB_Uncached_Pool);

STATUS NU_USBF_Init_GetHandle(VOID  **handle);

STATUS NU_USBF_Dev_Register(DV_DEV_ID device_id, VOID *context);
STATUS NU_USBF_Dev_Unregister(DV_DEV_ID device_id, VOID *context);

STATUS NU_USBF_DeInit(VOID *context,
                      UINT32 event_id);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
STATUS USBF_Init_Super_Speed_Device(NU_USB_DEVICE *device, NU_USBF_HW *hwctrl_ptr);
#endif

STATUS USBF_Init_High_Speed_Device(NU_USB_DEVICE *device, NU_USBF_HW *hwctrl_ptr);
STATUS USBF_Init_Full_Speed_Device(NU_USB_DEVICE *device, NU_USBF_HW *hwctrl_ptr);

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
STATUS         NU_USBF_Wait_For_Init(UNSIGNED suspend);
#endif /* DV_DEV_DISCOVERY_TASK_ENB */

/* ===================================================================== */

#endif /* _NU_USBF_INIT_EXT_H    */

/* =======================  End Of File  =============================== */

