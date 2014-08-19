/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
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
*
*       nu_usbf_ndis_user_dat.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Remote NDIS User Driver
*
* DESCRIPTION
*
*   This file contains definitions for dispatch table of NDIS User Driver.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*       None
*
***********************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_NDIS_USER_DAT_H_
#define _NU_USBF_NDIS_USER_DAT_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
extern const NU_USBF_NDIS_USER_DISPATCH usbf_ndis_user_dispatch;

extern NU_USBF_NDIS_USER		*NU_USBF_NDIS_USER_Cb_Pt;

#if (NF_VERSION_COMP < NF_2_0)
extern NU_USBF_NDIS_DEVICE		NU_USBF_NDIS_DEV_CB;
#endif

/* ==================================================================== */

#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_NDIS_USER_DAT_H_*/

/* ======================  End Of File  =============================== */
