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
*        nu_usb_cfg_imp.h
* 
*    COMPONENT 
*
*        Nucleus USB Software 
* 
*    DESCRIPTION 
*
*        This file contains the Control Block and other internal data 
*        structures and definitions for Configuration Component of
*        Nucleus USB Software.
*
*    DATA STRUCTURES 
*
*        nu_usb_cfg        Configuration control block.
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
#ifndef _NU_USB_CFG_IMP_H_
#define _NU_USB_CFG_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ====================  Data Structures ============================== */

struct nu_usb_cfg
{
    NU_USB_CFG_DESC *desc;
    NU_USB_OTG_DESC *otg_desc; 
    INT             numIADs;
    NU_USB_IAD      iad[NU_USB_MAX_IADS];
    INT             numIntfs;
    NU_USB_INTF     intf[NU_USB_MAX_INTERFACES];
    BOOLEAN         is_active;
    UINT8           *class_specific;
    UINT32          length;
    UINT32          load;
    NU_USB_DEVICE   *device;
};

/* ==================================================================== */

#endif /* _NU_USB_CFG_IMP_H_ */

/* ======================  End Of File  =============================== */

