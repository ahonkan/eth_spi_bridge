/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

**************************************************************************
*
* FILE NAME                         
*
*       nu_usbh_mouse_dat.h  
*  
*
* COMPONENT
*
*       Nucleus USB Host Mouse Driver.
*
* DESCRIPTION
*
*       This file contains definitions related to HID mouse user driver.
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

*       None.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBH_MOUSE_DAT_H_
#define _NU_USBH_MOUSE_DAT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
/* Global pointer. */
extern NU_USBH_MOUSE    *NU_USBH_MOUSE_Cb_Pt;

extern const NU_USBH_HID_USER_DISPATCH USBH_Mouse_Dispatch;

/* ==================================================================== */
#endif /* _NU_USBH_MOUSE_DAT_H_ */

/* ======================  End Of File  =============================== */

