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
*       nu_usbh_xhci_dat.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table of xHC Controller
*       Component.
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
*       None.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_XHCI_DAT_H_
#define _NU_USBH_XHCI_DAT_H_

/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* ===================================================================== */
extern const NU_USBH_XHCI_DISPATCH usbh_xhci_dispatch;

/* ===================================================================== */
#endif /* _NU_USBH_XHCI_DAT_H_ */

/* ======================  End Of File.  =============================== */
