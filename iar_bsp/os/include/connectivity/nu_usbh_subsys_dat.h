/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/************************************************************************ 
 * 
 * FILE NAME

 *      nu_usbh_subsys_dat.h
 * 
 * COMPONENT 
 *      Nucleus USB Host Software
 * 
 * DESCRIPTION 
 *      This file contains the dispatch table declaration for Nucleus USB
 *      Host subsystem.
 *
 * 
 * DATA STRUCTURES 
 *      None 
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBH_SUBSYS_DAT_H_
#define _NU_USBH_SUBSYS_DAT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
extern const NU_USBH_SUBSYS_DISPATCH usbh_subsys_dispatch;

/* ==================================================================== */
#endif /* _NU_USBH_SUBSYS_DAT_H_ */

/* ======================  End Of File  =============================== */

