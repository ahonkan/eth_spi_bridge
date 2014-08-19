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
* FILE NAME
*
*       nu_usbf_eth_dat.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Ethernet User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for dispatch table of Ethernet
*       User Driver.
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
#ifndef _NU_USBF_ETH_DAT_H_
#define _NU_USBF_ETH_DAT_H_

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
extern const NU_USBF_ETH_DISPATCH usbf_eth_dispatch;

#if (EF_VERSION_COMP < EF_2_0)
extern NU_USBF_ETH_DEV         NU_USBF_ETH_DEV_CB;
#endif

extern NU_USBF_ETH		*NU_USBF_USER_ETH_Cb_Pt;
/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_ETH_DAT_H_ */

/* ======================  End Of File  =============================== */
