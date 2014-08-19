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
*       nu_usbh_hid_dat.h 
*              
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table of HID
*       class driver.
*
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

#ifndef _NU_USBH_HID_DAT_H_
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define _NU_USBH_HID_DAT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */


extern NU_USBH_HID *NU_USBH_HID_Cb_Pt;
extern const NU_USB_DRVR_DISPATCH USBH_HID_Dispatch;


#ifdef          __cplusplus
}                                           /* End of C declarations.  */
#endif

/* ==================================================================== */

#endif      /* _NU_USBH_HID_DAT_H_ */

/* ======================  End Of File  =============================== */

