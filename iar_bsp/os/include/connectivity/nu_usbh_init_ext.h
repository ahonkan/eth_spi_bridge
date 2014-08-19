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

*************************************************************************
*
* FILE NAME
*
*       nu_usbh_init_ext.h
*
* COMPONENT
*
*       Nucleus USB Host Initialization
*
* DESCRIPTION
*
*       Contains header information surrounding the host stack
*       initialization.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*       nu_usbh_init_cfg.h         USB Initialization configuration
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_INIT_EXT_H
#define _NU_USBH_INIT_EXT_H

/* ==================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

#include "connectivity/nu_usbh_init_cfg.h"

/* ====================  Function Prototypes ========================== */
STATUS nu_os_conn_usb_host_stack_init(CHAR *path, INT startstop);

STATUS  NU_USBH_Init(NU_MEMORY_POOL *USB_Cached_Pool,
                    NU_MEMORY_POOL *USB_Uncached_Pool);

STATUS NU_USBH_Init_GetHandle(VOID  **handle);

STATUS NU_USBH_Dev_Register(DV_DEV_ID, VOID *);
STATUS NU_USBH_Dev_Unregister(DV_DEV_ID, VOID *);

STATUS NU_USBH_DeInit( VOID *context,
                       UINT32 event_id);
/* ==================================================================== */

#endif /* _NU_USBH_INIT_EXT_H    */

/* =======================  End Of File  ============================== */

