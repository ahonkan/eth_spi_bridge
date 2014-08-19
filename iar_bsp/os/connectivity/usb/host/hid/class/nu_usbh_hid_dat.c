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
*       nu_usbh_hid_dat.c 
*      
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file defines the dispatch table for Nucleus USB Host
*       HID class driver. Some of them use functionality of
*       NU_USB_DRVR basic implementation, others are extended or ignored.
*
*
* DATA STRUCTURES
*
*       USBH_HID_Dispatch                   HID Class Driver dispatch
*                                           table.
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

NU_USBH_HID *NU_USBH_HID_Cb_Pt;

const NU_USB_DRVR_DISPATCH USBH_HID_Dispatch =
{
    {
        _NU_USBH_HID_Delete,
        _NU_USB_Get_Name,
        _NU_USB_Get_Object_Id
    },

    _NU_USB_DRVR_Examine_Intf,
    NU_NULL,
    _NU_USB_DRVR_Get_Score,
    NU_NULL,
    _NU_USBH_HID_Initialize_Intf,
    _NU_USBH_HID_Disconnect
};

/* ======================  End Of File  =============================== */

