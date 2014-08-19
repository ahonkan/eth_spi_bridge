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

***************************************************************************
* FILE NAME                        
*
*       nu_usbh_kbd_dat.h       
*                                 
*
* COMPONENT
*
*       Nucleus USB Host Keyboard Driver.
*
* DESCRIPTION
*
*       This file provides definitions related to HID Class keyboard user
*       driver.
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

/* ==================================================================== */
#ifndef _NU_USBH_KBD_DAT_H_
#define _NU_USBH_KBD_DAT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
extern NU_USBH_KBD  *NU_USBH_KBD_Cb_Pt;
extern const NU_USBH_HID_USER_DISPATCH Usbh_Kbd_Dispatch;

/* ==================================================================== */
#endif /* _NU_USBH_KBD_DAT_H_ */

/* ======================  End Of File  =============================== */

