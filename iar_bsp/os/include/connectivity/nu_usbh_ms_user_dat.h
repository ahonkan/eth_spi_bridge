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
*       nu_usbh_ms_user_dat.h
*
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver.
*
* DESCRIPTION
*
*       This file contains declaration for USBH MS User driver
*       dispatch table.
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

#ifndef     _NU_USBH_MS_USER_DAT_H_
#define     _NU_USBH_MS_USER_DAT_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "common/stack/inc/nu_usb.h"
#endif
#include "connectivity/nu_usb.h"
/* ====================  Data Types  =================================== */

extern NU_USBH_MS_USER  *NU_USBH_MS_USER_Cb_Pt;
extern const NU_USBH_USER_DISPATCH usbh_ms_user_dispatch;
#if INCLUDE_SCSI    /* SCSI */
extern const NU_USBH_MS_USER_DISPATCH usbh_ms_scsi_dispatch;
#endif
#if INCLUDE_UFI   /* UFI */
extern const NU_USBH_MS_USER_DISPATCH usbh_ms_ufi_dispatch;
#endif
#if INCLUDE_SFF8020I    /* SFF-8020 */
extern const NU_USBH_MS_USER_DISPATCH usbh_ms_8020_dispatch;
#endif
#if INCLUDE_SFF8070I    /* SFF-8070i */
extern const NU_USBH_MS_USER_DISPATCH usbh_ms_8070_dispatch;
#endif
/* ===================================================================== */

#endif      /* _NU_USBH_MS_USER_DAT_H_ */

/* ====================  End Of File  ================================== */
