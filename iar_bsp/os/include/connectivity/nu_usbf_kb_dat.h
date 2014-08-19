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
*       nu_usbf_kb_dat.h
*
*
* COMPONENT
*
*       Nucleus USB Function Software : HID Keyboard User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table and global data
*       of HID Keyboard User Driver.
*
* DATA STRUCTURES

*       None.
*
* FUNCTIONS

*       None.
*
* DEPENDENCIES

*       None.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_KB_DAT_H_
#define _NU_USBF_KB_DAT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
extern NU_USBF_KB   *NU_USBF_KB_Cb_Pt;
extern const NU_USBF_KB_DISPATCH    usbf_kb_dispatch;
extern const UINT8                  nu_usbf_kb_keysasciitable[];
extern const UINT8                  nu_usbf_kb_usageidtable[];

/* ==================================================================== */
/* ==================================================================== */
#endif /* _NU_USBF_KB_DAT_H_ */

/* ======================  End Of File  =============================== */
