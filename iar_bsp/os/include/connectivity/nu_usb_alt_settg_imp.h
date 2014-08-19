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
*    FILE NAME
*
*        nu_usb_alt_settg_imp.h
* 
*    COMPONENT 
*
*        Nucleus USB Software 
* 
*    DESCRIPTION 
*
*        This file contains the Control Block and other internal data 
*        structures and definitions for Alternate Setting Component of
*        Nucleus USB Software.
* 
*    DATA STRUCTURES 
*
*        nu_usb_alt_settg        Alternate setting control block.
* 
*    FUNCTIONS 
*
*        None 
* 
*    DEPENDENCIES 
*
*        None 
* 
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ALT_SETTG_IMP_H_
#define _NU_USB_ALT_SETTG_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ====================  Data Structures ============================== */

struct nu_usb_alt_settg
{
    NU_USB_INTF_DESC *desc;
    NU_USB_INTF *intf;
    UINT8 *class_specific;
    BOOLEAN is_active;
    UINT32 length;
    NU_USB_ENDP endp[NU_USB_MAX_ENDPOINTS];
    UINT32 load;
};

/* ==================================================================== */

#endif /* _NU_USB_ALT_SETTG_IMP_H_ */

/* ======================  End Of File  =============================== */

