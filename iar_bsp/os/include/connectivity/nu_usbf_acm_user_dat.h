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
*       nu_usbf_acm_user_dat.h
*
*
* COMPONENT
*
*       Nucleus USB Function Software : ACM User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table of ACM User
*       Driver.
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
#ifndef _NU_USBF_ACM_USER_DAT_H_
#define _NU_USBF_ACM_USER_DAT_H_
/* ==================================================================== */

#ifdef __cplusplus
extern "C" {							/* C declarations in C++. */
#endif

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
extern const NU_USBF_ACM_USER_DISPATCH usbf_acm_user_dispatch;

/* Control block for Function ACM User driver used in NMI */
extern NU_USBF_ACM_USER		*NU_USBF_USER_ACM_Cb_Pt;
#if (ACMF_VERSION_COMP < ACMF_2_0)
extern NU_USBF_ACM_DEV         NU_USBF_ACM_DEV_CB;
#endif

/* ==================================================================== */

#ifdef          __cplusplus
}                                            /* End of C declarations   */
#endif

#endif /* _NU_USBF_ACM_USER_DAT_H_ */

/* ======================  End Of File  =============================== */
