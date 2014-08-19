/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_hid_dat.c
*
* COMPONENT
*
*       Nucleus USB Function software : HID Class Driver
*
* DESCRIPTION
*
*       This file defines the dispatch table for HID Class Driver.
*
* DATA STRUCTURES
*
*       usbf_hid_dispatch                   HID Class Driver dispatch
*                                           table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb.h"

/* =====================  Global data ================================  */

/* Global pointer for the class driver to be used in runlevel
 * initialization.
 */
NU_USBF_HID *NU_USBF_HID_Cb_Pt;

const NU_USBF_HID_DISPATCH usbf_hid_dispatch =
{
        /*USBF_CLASSDRIVER_DISPATCH*/
    {
            /* USB_CLASSDRIVER_DISPATCH*/
        {
            {
                /*USB_DISPATCH*/
                _NU_USBF_HID_Delete,
                _NU_USB_Get_Name,              /* does not override. */
                _NU_USB_Get_Object_Id          /* does not override. */
            },

            _NU_USB_DRVR_Examine_Intf,
            NU_NULL,
            _NU_USB_DRVR_Get_Score,
            NU_NULL,
            _NU_USBF_HID_Initialize_Intf,
            _NU_USBF_HID_Disconnect
        },

        _NU_USBF_HID_Set_Intf,
        _NU_USBF_HID_New_Setup,
        _NU_USBF_HID_New_Transfer,
        _NU_USBF_HID_Notify
    }
};


/* ======================  End Of File  =============================== */
