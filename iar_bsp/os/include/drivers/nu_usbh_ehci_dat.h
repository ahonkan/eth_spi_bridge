/************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*       nu_usbh_ehci_dat.h
*
* COMPONENT
*
*       EHCI Driver / Nucleus USB Host Software
*
* DESCRIPTION
*
*       This file contains the exported data declarations for the EHCI
*       driver
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
*       nu_usbh_hw_ext.h    All Base hardware dependencies
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_EHCI_DAT_H_
#define _NU_USBH_EHCI_DAT_H_

/* ==================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

extern const NU_USBH_EHCI_DISPATCH usbh_ehci_dispatch;

/* ==================================================================== */

#endif /* _NU_USBH_EHCI_DAT_H_ */

/* =======================  End Of File  ============================== */
