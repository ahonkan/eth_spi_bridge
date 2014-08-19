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

************************************************************************
*
* FILE NAME
*
*       nu_usbf_comm_dat.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Communication Class Driver
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table of Communication
*       Class Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_COMM_DAT_H_
#define _NU_USBF_COMM_DAT_H_

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

/* Control block for Function COMM Driver used in NMI */
extern NU_USBF_COMM *NU_USBF_COMM_Cb_Pt;

extern const NU_USBF_COMM_DISPATCH usbf_comm_dispatch;

/* ==================================================================== */

#ifdef      __cplusplus
}                                           /* End of C declarations    */
#endif

#endif      /* _NU_USBF_COMM_DAT_H_ */

/* ======================  End Of File  =============================== */
