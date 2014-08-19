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

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_dat.h
*
*
* COMPONENT
*
*       Stack Component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the shared data declarations for the singleton
*       object.
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

#ifndef _NU_USBF_DAT_H_
#define _NU_USBF_DAT_H_

/* ===================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* =====================  Global data =================================  */

/* Singleton instance of Nucleus USB Device. */
extern NU_USBF *nu_usbf;

/* Function Stack Control Block pointer */
extern NU_USBF_STACK   *NU_USBF_Stack_CB_Pt;

/* Data for usbf_hisr */
extern UINT8   usbf_hisr_stack[];

/* ===================================================================== */

#endif      /* _NU_USBF_DAT_H_ */

/* ======================  End Of File  ================================ */

