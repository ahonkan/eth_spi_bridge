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
*     nu_usbh_audio_dat.h
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file contains definition for dispatch table of Nucleus USB Host
*     AUDIO Class Driver.
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
#ifndef _NU_USBH_AUD_DAT_H_
#define _NU_USBH_AUD_DAT_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */
#ifdef __cplusplus
extern "C" {                                 /* C declarations in C++. */
#endif

extern NU_USBH_AUDIO * NU_USBH_Audio_Cb_Pt;

extern const NU_USB_DRVR_DISPATCH usbh_audio_dispatch;
#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif
#endif
