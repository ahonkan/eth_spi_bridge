/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_ms_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the Mass
*       storage class driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_ms_imp.h                    Definitions for workhorse of
*                                           mass storage function driver.
*
**************************************************************************/

#ifndef _NU_USBF_MS_EXT_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_MS_EXT_H

/* =========================  USB Include Files ======================== */
#include    "connectivity/nu_usbf_ms_imp.h"

/* =========================  Function Prototypes ====================== */
STATUS NU_USBF_MS_Create_Task_Mode (
       NU_USBF_MS                *cb,
       CHAR                      *name,
       NU_MEMORY_POOL            *mem_pool);

STATUS NU_USBF_MS_Create (
       NU_USBF_MS                *cb,
       CHAR                      *name);
       
STATUS NU_USBF_MS_Bind_Interface (NU_USBF_MS *cb);

STATUS NU_USBF_MS_Unbind_Interface (NU_USBF_MS *cb);

STATUS _NU_USBF_MS_Delete (
       VOID                      *cb);

/* Set configuration callback. */
STATUS _NU_USBF_MS_Set_Interface (
       NU_USB_DRVR               *cb,
       NU_USB_STACK              *stack,
       NU_USB_DEVICE             *dev,
       NU_USB_INTF               *intf,
       NU_USB_ALT_SETTG          *alt_settg);

/* New Class specific request. */
STATUS _NU_USBF_MS_Class_Specific (
       NU_USB_DRVR               *cb,
       NU_USB_STACK              *stack,
       NU_USB_DEVICE             *device,
       NU_USB_SETUP_PKT          *setup);

/* New IN or OUT token. */
STATUS _NU_USBF_MS_New_Transfer (
       NU_USB_DRVR               *cb,
       NU_USB_STACK              *stack,
       NU_USB_DEVICE             *device,
       NU_USB_PIPE               *pipe);

/* USB Event notification. */
STATUS _NU_USBF_MS_Notify (
       NU_USB_DRVR               *cb,
       NU_USB_STACK              *stack,
       NU_USB_DEVICE             *device,
       UINT32                     event);

/* New NU_USBF_MS device callback. */
STATUS _NU_USBF_MS_Connect (
       NU_USB_DRVR               *cb,
       NU_USB_STACK              *stack,
       NU_USB_DEVICE             *dev,
       NU_USB_INTF               *intf);

/* Device disconnected callback. */
STATUS _NU_USBF_MS_Disconnect (
       NU_USB_DRVR               *cb,
       NU_USB_STACK              *stack,
       NU_USB_DEVICE             *dev);

STATUS NU_USBF_MS_Start_Cmd_Processing(
       NU_USBF_MS_DEVICE         *pcb_ms_device);

/* Set the data phase direction IN/OUT. */
STATUS NU_USBF_MS_Set_Command_Direction(
	   VOID                      *handle,
       UINT8                      direction);

/* Mass storage Initialization function used in Registry. */
STATUS nu_os_conn_usb_func_ms_class_init(CHAR *path, INT startstop);

/* Class Driver instance retrieval function. */
STATUS NU_USBF_MS_Init_GetHandle(VOID  **handle);

STATUS NU_USBF_MS_RW(VOID*      handle,
                    VOID*       buffer,
                    UINT32      numbyte,
                    OFFSET_T    byte_offset);

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_MS_EXT_H  */

/* ======================  End Of File  ================================ */
