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

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_subsys_dat.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the shared data structure definitions of the USB
*       Function Software subsystem.
*
* DATA STRUCTURES
*
*       usbf_subsys_dispatch                Dispatch Table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h
*
**************************************************************************/

/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/* =====================  Global data =================================  */

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
const NU_USBF_SUBSYS_DISPATCH usbf_subsys_dispatch = {

    _NU_USB_SUBSYS_Delete,
    _NU_USB_SUBSYS_Lock,
    _NU_USB_SUBSYS_Unlock
};
#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/* ======================  End Of File  ================================ */

