/**************************************************************************
*
*               Copyright 2012  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************
***************************************************************************
* FILE NAME                                    
*     nu_usbh_audio_user_dat.c                                          
*
* COMPONENT
*     Nucleus USB Host AUDIO user driver.
*
* DESCRIPTION
*     This file defines the dispatch table for Audio user driver.
*
* DATA STRUCTURES
*     usbh_audio_user_dispatch    Audio user driver dispatch table.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     nu_usb.h              All USB definitions
*
**************************************************************************/

/* ==============  Standard Include Files =============================  */

#include "connectivity/nu_usb.h"
#include "nu_usbh_audio_user_ext.h"

/* ======================= Global data ================================  */

/* Based on the nucleus usb host user driver. */

NU_USBH_AUD_USER *NU_USBH_AUD_USER_Cb_Pt;

const NU_USBH_USER_DISPATCH usbh_audio_user_dispatch = {
    {
        {
            _NU_USBH_AUD_USER_Delete,
            _NU_USB_Get_Name,
            _NU_USB_Get_Object_Id
        },

        _NU_USBH_AUD_USER_Connect,
        _NU_USBH_AUD_USER_Disconnect
    },

    _NU_USBH_USER_Wait,
    _NU_USBH_USER_Open_Device,
    _NU_USBH_USER_Close_Device,
    _NU_USBH_USER_Remove_Device
};
