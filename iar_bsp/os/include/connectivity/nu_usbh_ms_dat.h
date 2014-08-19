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
*       nu_usbh_ms_dat.h
*
*
* COMPONENT
*
*       Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table of mass
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
*       None.
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_MS_DAT_H_
#define     _NU_USBH_MS_DAT_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers,                          \
       use #include "common/stack/inc/nu_usb.h"
#endif

/* ====================  Data Types  =================================== */

extern NU_USBH_MS *NU_USBH_MS_Cb_Pt;

extern const NU_USB_DRVR_DISPATCH usbh_ms_dispatch;

/* ===================================================================== */

#endif      /* _NU_USBH_MS_DAT_H_ */

/* ====================  End Of File  ================================== */
