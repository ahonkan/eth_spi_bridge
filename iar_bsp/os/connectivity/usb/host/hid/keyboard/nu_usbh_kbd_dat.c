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
*       nu_usbh_kbd_dat.c                
*                              
*
* COMPONENT
*
*       Nucleus USB Host Keyboard driver.
*
* DESCRIPTION
*
*       This file defines the dispatch table for Nucleus USB Host keyboard
*       driver. Some of them uses functionality of NU_USBH_USER basic
*       implementation, others are extended or ignored.
*
*
* DATA STRUCTURES
*
*       Usbh_Kbd_Dispatch                   Keyboard Driver dispatch table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb.h"

/* Global pointer. */
NU_USBH_KBD *NU_USBH_KBD_Cb_Pt;

const NU_USBH_HID_USER_DISPATCH Usbh_Kbd_Dispatch =
{
    {
        {
            {
                _NU_USBH_KBD_Delete,        /* Overrides. */
                _NU_USB_Get_Name,           /* Does not override.
                                             */
                _NU_USB_Get_Object_Id       /* Does not override.
                                             */
            },
            _NU_USBH_KBD_Connect,
            _NU_USBH_KBD_Disconnect
        },
        _NU_USBH_USER_Wait,
        _NU_USBH_USER_Open_Device,
        _NU_USBH_USER_Close_Device,
        _NU_USBH_USER_Remove_Device
    },
    _NU_USBH_KBD_Get_Usages,
    _NU_USBH_KBD_Notify_Report
};

/* ======================  End Of File  =============================== */

