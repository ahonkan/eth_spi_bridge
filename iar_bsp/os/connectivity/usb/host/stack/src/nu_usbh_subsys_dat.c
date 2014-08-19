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
*        nu_usbh_subsys_dat.c
*
* COMPONENT
*       Nucleus USB Host software.
*
* DESCRIPTION
*       This file defines the dispatch table for Nucleus USB Host
*       subsystem.
*
*
* DATA STRUCTURES
*       usbh_subsys_dispatch    Control IRP dispatch table.
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_SUBSYS_DAT_C
#define USBH_SUBSYS_DAT_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
/* Host subsystem Dispatch Table */
const NU_USBH_SUBSYS_DISPATCH usbh_subsys_dispatch = {

    _NU_USBH_SUBSYS_Delete,
    _NU_USBH_SUBSYS_Lock,                   /* overrides */
    _NU_USBH_SUBSYS_Unlock                  /* overrides */
};
#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/************************************************************************/

#endif /* USBH_SUBSYS_DAT_C */
/* ======================  End Of File  =============================== */
