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
*       nu_usbf_ms_dat.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the shared data definitions for the Mass
*       Storage class driver.
*
* DATA STRUCTURES
*
*       NU_USBF_DRVR_DISPATCH               USB Function Driver dispatch.
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
/* USB Include Files */
#include    "connectivity/nu_usb.h"

/* =====================  Global data ================================  */
/* Global pointer for the class driver. */
NU_USBF_MS *NU_USBF_MS_Cb_Pt;

/* Creating a global instance for MS driver's dispatch table which is
 * just of the type of USB driver dispatch table. Dispatch initialization.
 */
const NU_USBF_DRVR_DISPATCH usbf_ms_dispatch = {

    /* USBF CLASS DRIVER dispatch. */
    {
        /* USB Class driver dispatch. */
        {
            /* USB dispatch. */
            _NU_USBF_MS_Delete,
            _NU_USB_Get_Name,               /* Does not override. */
            _NU_USB_Get_Object_Id,          /* Does not override. */
        },

        _NU_USB_DRVR_Examine_Intf,
        NU_NULL,
        _NU_USB_DRVR_Get_Score,
         NU_NULL,
        _NU_USBF_MS_Connect,
        _NU_USBF_MS_Disconnect
    },
    _NU_USBF_MS_Set_Interface,
    _NU_USBF_MS_Class_Specific,
    _NU_USBF_MS_New_Transfer,
    _NU_USBF_MS_Notify
};

/************************* End Of File ***********************************/
