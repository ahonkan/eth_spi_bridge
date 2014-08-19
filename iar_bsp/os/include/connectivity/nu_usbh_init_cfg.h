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
************************************************************************

*************************************************************************
*
* FILE NAME
*
*       nu_usbh_init_cfg.h
*
* COMPONENT
*
*       Nucleus USB Host Initialization
*
* DESCRIPTION
*
*       Contains configurable parameters information surrounding the
*       host stack initialization.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_INIT_CFG_H
#define _NU_USBH_INIT_CFG_H

/* ==================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* If optimizations are enabled then use different values for macros. */
#if (NU_USB_OPTIMIZE_FOR_SIZE)

#define     NU_USBH_TASK_STACK_SIZE         (4  * NU_MIN_STACK_SIZE)
#define     NU_USBH_HUB_STACK_SIZE          (16 * NU_MIN_STACK_SIZE)
#define     NU_USBH_HISR_STACK_SIZE         (4  * NU_MIN_STACK_SIZE)

#else /* If no optimization is required. */

#define     NU_USBH_TASK_STACK_SIZE         (8 * 1024)
#define     NU_USBH_HUB_STACK_SIZE          (8 * 1024)
#define     NU_USBH_HISR_STACK_SIZE         (2 * 1024)

#endif /* #if (NU_USB_OPTIMIZE_FOR_SIZE) */

#define     NU_USBH_STACK_PRIORITY          8
#define     NU_USBH_HUB_PRIORITY            10
#define     NU_USBH_HISR_PRIORITY           1

/* ==================================================================== */

#endif /* _NU_USBH_INIT_CFG_H    */

/* =======================  End Of File  ============================== */

