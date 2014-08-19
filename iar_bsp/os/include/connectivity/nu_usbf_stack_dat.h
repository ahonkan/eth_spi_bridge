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
*       nu_usbf_stack_dat.h
*
*
* COMPONENT
*
*       Stack Component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the shared data structure declarations for
*       the stack.
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

#ifndef _NU_USB_STACK_DAT_H
#define _NU_USB_STACK_DAT_H

/* =====================  Global data =================================  */

extern const NU_USB_STACK_DISPATCH usbf_stack_dispatch;

extern USBF_CTRL_REQ_WRKR usbf_ctrl_request_worker[];

extern UINT8 usbf_num_supported_ctrl_requests;

/* ===================================================================== */

#endif      /* _NU_USBF_STACK_DAT_H_ */

/* ======================  End Of File  ================================ */

