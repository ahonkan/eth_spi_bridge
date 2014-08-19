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
*        nu_usbh_hub_dat.c 
*
* COMPONENT
*
*        Nucleus USB Host Stack
*
* DESCRIPTION
*       This file provides the definition of dispatch table for Hub class
*       driver.
*
* DATA STRUCTURES
*       usbh_hub_dispatch       Dispatch Table for HUB Driver.
*
* FUNCTIONS
*       None
* 
* DEPENDENCIES 
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_HUB_DAT_C
#define USBH_HUB_DAT_C

/* ==============  Standard Include Files ============================  */
#include    "connectivity/nu_usb.h"

/* =====================  Global data ================================  */

const NU_USBH_HUB_DISPATCH usbh_hub_dispatch = {
    {
     _NU_USBH_HUB_Delete,       /* overrides. */
     _NU_USB_Get_Name,          /* does not override. */
     _NU_USB_Get_Object_Id      /* does not override. */
     },
    _NU_USB_DRVR_Examine_Intf,  /* doesn't override */
    NU_NULL,                    /* Hub is an interface driver */
    _NU_USB_DRVR_Get_Score,     /* doesn't override */
    NU_NULL,                    /* Hub is an interface driver */
    USBH_HUB_Initialize_Intf,
    _NU_USBH_HUB_Disconnect
};

/************************************************************************/

#endif /* USBH_HUB_DAT_C */
/* ======================  End Of File  =============================== */
