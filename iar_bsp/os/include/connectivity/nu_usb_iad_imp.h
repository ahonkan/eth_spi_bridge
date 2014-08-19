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
 *        nu_usb_iad_imp.h
 *
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *       This file contains the Control Block and other internal data 
 *       structures and definitions for the Interface Association
 *       Descriptor Component of Nucleus USB Software.
 *
 * 
 * DATA STRUCTURES 
 *      nu_usb_iad        Interface association block control block.
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None 
 * 
 *************************************************************************/

#ifndef _NU_USB_IAD_IMP_H_
#define _NU_USB_IAD_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

#define MISC_DEVICE_CLASS 0xEF
#define MULTI_INTERFACE_FUNC_CLASS MISC_DEVICE_CLASS

#define COMMON_CLASS 0x02
#define MULTI_INTERFACE_FUNC_SUBCLASS COMMON_CLASS

#define INTERFACE_ASSOC_DESC 0x01
#define MULTI_INTERFACE_FUNC_PROTOCOL INTERFACE_ASSOC_DESC 

/* ====================  Data Structures ============================== */

struct nu_usb_iad
{
    UINT8 first_intf;
    UINT8 last_intf;
    DATA_ELEMENT pad[2];
    NU_USB_IAD_DESC *iad_desc;
    NU_USB_CFG *cfg;
    NU_USB_DEVICE *device;
};

/* ==================================================================== */

#endif /* _NU_USB_IAD_IMP_H_ */

/* ======================  End Of File  =============================== */
