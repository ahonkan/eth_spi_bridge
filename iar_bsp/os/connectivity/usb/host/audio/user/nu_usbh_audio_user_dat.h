/**************************************************************************
*
*               Copyright 2012  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************
***************************************************************************
*
* FILE NAME                                    
*     nu_usbh_audio_user_dat.h                                          
*
* COMPONENT
*     Nucleus USB Host AUDIO user driver.
*
* DESCRIPTION
*     This file contains definitions for dispatch table of Audio host user
*     driver.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     None.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_AUD_USER_DAT_H_
#define _NU_USBH_AUD_USER_DAT_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* ============================== Globals  ============================= */
extern NU_USBH_AUD_USER  *NU_USBH_AUD_USER_Cb_Pt;
extern const NU_USBH_AUD_USER_DISPATCH usbh_audio_user_dispatch;

/* ===================================================================== */
#endif /* _NU_USBH_AUD_USER_DAT_H_ */
