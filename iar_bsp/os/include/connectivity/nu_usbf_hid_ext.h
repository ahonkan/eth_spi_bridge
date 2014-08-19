/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_hid_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Software : HID Class Driver
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       HID Class Driver.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbf_hid_imp.h                   Internal Definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_HID_EXT_H_
#define _NU_USBF_HID_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#include "connectivity/nu_usbf_hid_imp.h"

/* ====================  Function Prototypes ========================== */
/* NU_USBF_HID Constructor. */
/* The prototype may take more parameters depending on implementation */

STATUS NU_USBF_HID_Create (NU_USBF_HID *cb,
                           CHAR *name,
                           NU_MEMORY_POOL *pool);

STATUS NU_USBF_HID_Init_GetHandle(VOID  **handle);


/* ====================  HID Services ========================= */

STATUS  _NU_USBF_HID_Initialize_Intf (NU_USB_DRVR * cb,
                                    NU_USB_STACK * stack,
                                    NU_USB_DEVICE * dev,
                                    NU_USB_INTF * intf);

STATUS  _NU_USBF_HID_Disconnect (NU_USB_DRVR * cb,
                                 NU_USB_STACK * stack,
                                 NU_USB_DEVICE * dev);

STATUS _NU_USBF_HID_Set_Intf (NU_USB_DRVR * cb,
                             NU_USB_STACK * stack,
                             NU_USB_DEVICE * device,
                             NU_USB_INTF * intf,
                             NU_USB_ALT_SETTG * alt_settting);

STATUS  _NU_USBF_HID_New_Setup(NU_USB_DRVR * cb,
                               NU_USB_STACK * stack,
                               NU_USB_DEVICE * device,
                               NU_USB_SETUP_PKT * setup);

STATUS  _NU_USBF_HID_New_Transfer (NU_USB_DRVR * cb,
                                   NU_USB_STACK * stack,
                                   NU_USB_DEVICE * device,
                                   NU_USB_PIPE * pipe);

STATUS  _NU_USBF_HID_Notify (NU_USB_DRVR * cb,
                             NU_USB_STACK * stack,
                             NU_USB_DEVICE * device,
                             UINT32 event);

STATUS _NU_USBF_HID_Delete (VOID *cb);

/* Add the prototypes for extra services provided by HID here */
STATUS NU_USBF_HID_Send_Report(NU_USBF_HID *HID,
                               UINT8 *report,
                               UINT32 len,
                               VOID *handle);

/* ==================================================================== */

/* ==================================================================== */
#endif /* NU_USBF_HID_EXT */

/* ======================  End Of File  =============================== */

