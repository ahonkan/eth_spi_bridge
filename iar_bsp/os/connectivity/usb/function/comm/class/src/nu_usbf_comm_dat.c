/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       nu_usbf_comm_dat.c
*
* COMPONENT
*
*       Nucleus USB Function software : Communication Class Driver
*
* DESCRIPTION
*
*       This file defines the dispatch table for Communication Class
*       Driver.
*
* DATA STRUCTURES
*
*       usbf_comm_dispatch                  Communication Class Driver
*                                           dispatch table.
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
*************************************************************************/

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

/* =====================  Global data ================================  */
/* Global pointer for the Function Communication class driver control block */
NU_USBF_COMM   *NU_USBF_COMM_Cb_Pt;

const NU_USBF_COMM_DISPATCH usbf_comm_dispatch =
{
    {
        {
            {
                _NU_USBF_COMM_Delete,
                _NU_USB_Get_Name,           /* does not override.       */
                _NU_USB_Get_Object_Id       /* does not override.       */
            },

            /* An interface driver need not have Examine_Device function
             * pointer and similarly a device driver need not have
             * Examine_Interface function pointer.
             */
            _NU_USB_DRVR_Examine_Intf,
            NU_NULL,
            _NU_USB_DRVR_Get_Score,

            /* Only one of these will be called by stack depending on
             * whether the class driver is a interface/device driver.
             */
            NU_NULL,
            _NU_USBF_COMM_Initialize_Intf,
            _NU_USBF_COMM_Disconnect
        },
        _NU_USBF_COMM_Set_Intf,
        _NU_USBF_COMM_New_Setup,
        _NU_USBF_COMM_New_Transfer,
        _NU_USBF_COMM_Notify
    }

    /* New Services of Communication driver if any, go here. */
};

/* ======================  End Of File  =============================== */
