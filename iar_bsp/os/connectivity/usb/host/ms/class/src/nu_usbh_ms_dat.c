/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
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
*       nu_usbh_ms_dat.c
*
*
* COMPONENT
*
*       Nucleus USB Host software.
*
* DESCRIPTION
*
*       This file defines the dispatch table for Nucleus USB Host Mass
*       Storage class driver. Some of them uses functionality of
*       NU_USB_DRVR basic implementation, others are extended or ignored.
*
* DATA STRUCTURES
*
*       usbh_ms_dispatch                    Mass Storage Class Driver
*                                           dispatch table.
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

/* ====================  USB Include Files  ============================ */

#include "connectivity/nu_usb.h"

/* ====================  Global Data  ================================== */

/* Global pointer for the class driver. */
NU_USBH_MS *NU_USBH_MS_Cb_Pt;

const NU_USB_DRVR_DISPATCH usbh_ms_dispatch = {
    {
        _NU_USBH_MS_Delete,                 /* Overrides.                */
        _NU_USB_Get_Name,                   /* Does not override.        */
        _NU_USB_Get_Object_Id               /* Does not override.        */
    },

    _NU_USB_DRVR_Examine_Intf,              /* Doesn't override.         */
    /* MS is a class/interface driver.  */
    NU_NULL,
    _NU_USB_DRVR_Get_Score,                 /* Doesn't override.         */
    NU_NULL,                                /* MS is a interface driver. */
    _NU_USBH_MS_Initialize_Intf,
    _NU_USBH_MS_Disconnect
};

/* ====================  End Of File  ================================== */
