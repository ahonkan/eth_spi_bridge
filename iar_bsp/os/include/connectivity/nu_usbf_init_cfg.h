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
*       nu_usbf_init_cfg.h
*
* COMPONENT
*
*       Nucleus USB Function Initialization
*
* DESCRIPTION
*
*       Contains configurable parameters information surrounding the
*       function stack initialization.
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

#ifndef _NU_USBF_INIT_CFG_H
#define _NU_USBF_INIT_CFG_H

/* ===================================================================  */

/* Defines for usbf_hisr */
#define     USBF_HISR_PRIORITY      1

/* If optimizations are enabled then use different values for macros. */
#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define     USBF_HISR_STACK_SIZE    (4 * NU_MIN_STACK_SIZE)
#else /* If no optimization is required. */
#define     USBF_HISR_STACK_SIZE    (4 * 1024)
#endif /*#if (NU_USB_OPTIMIZE_FOR_SIZE) */

/* ==================================================================== */

#endif /* _NU_USBH_INIT_CFG_H    */

/* =======================  End Of File  ============================== */
