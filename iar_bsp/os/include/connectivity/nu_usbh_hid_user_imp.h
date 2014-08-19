/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME   
*
*       nu_usbh_hid_user_imp.h  
*                     
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for USB Host HID User layer.
*
* DATA STRUCTURES
*
*       nu_usbh_hid_user                    USB Host HID User Control
*                                           Block.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/

#ifndef _NU_USBH_HID_USER_IMP_H_
#define _NU_USBH_HID_USER_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif


/* =================== Data Structures ===============================  */

/* The Control Block. */

struct nu_usbh_hid_user
{
    NU_USBH_USER cb;

    UINT8 num_usages;
    
    /* To align this structure on 32-bit boundary */
    UINT8 pad[3];
    
}
;

/* ===================================================================  */

#endif      /* _NU_USBH_HID_USER_IMP_H_ */

/* ====================== End of File ================================  */

