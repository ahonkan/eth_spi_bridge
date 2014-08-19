/************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*        nu_usbh_ehci_dat.c
*
* COMPONENT
*
*       Nucleus USB Host software
*
*
* DESCRIPTION
*
*       This file contains the global data definitions for the EHCI
*       driver
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*       nu_drivers.h
*
************************************************************************/

/* ==============  USB Include Files =================================== */
#include "connectivity/nu_usb.h"
#include "drivers/nu_drivers.h"


const NU_USBH_EHCI_DISPATCH usbh_ehci_dispatch =
{
    {
        {
            _NU_USBH_EHCI_Delete,
            _NU_USB_Get_Name,
            _NU_USB_Get_Object_Id
        }
    }
};


/* =======================  End Of File  ============================== */
