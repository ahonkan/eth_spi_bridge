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
*       nu_usbh_com_eth_dat.h
*
*
* COMPONENT
*       Nucleus USB host software : Communication class user driver.
*
* DESCRIPTION
*       This file contains definitions for dispatch table of Communication 
*       host ethernet user driver.
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
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_ETH_DAT_H_
#define _NU_USBH_COM_ETH_DAT_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */
extern NU_USBH_COM_ETH *NU_USBH_COM_ETH_Cb_Pt;
/* ============================== Global  ============================= */
extern const NU_USBH_COM_USER_DISPATCH usbh_com_eth_dispatch;

/* ===================================================================== */
#endif /* _NU_USBH_COM_ETH_DAT_H_ */
