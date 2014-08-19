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
*       nu_usbf_mse_dat.c
*
* COMPONENT
*
*       Nucleus USB Function software : HID Mouse User Driver.
*
* DESCRIPTION
*
*       This file defines the dispatch table and other global data for
*       Mouse User Driver.
*
*
* DATA STRUCTURES
*
*       usbf_mse_dispatch                   Mouse User Driver dispatch
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

/* =====================  Global data ================================  */

/* Global pointer to be used for runlevel initialization. */
NU_USBF_MSE *NU_USBF_MSE_Cb_Pt;

/* Dispatch table structure. */
const NU_USBF_MSE_DISPATCH usbf_mse_dispatch =
{
    {
        {
            {
                _NU_USBF_MSE_Delete,
                _NU_USB_Get_Name,              /* does not override. */
                _NU_USB_Get_Object_Id          /* does not override. */
            },

            _NU_USBF_MSE_Connect,
            _NU_USBF_MSE_Disconnect
        },

        _NU_USBF_MSE_New_Command,
        _NU_USBF_MSE_New_Transfer,
        _NU_USBF_MSE_Tx_Done,
        _NU_USBF_MSE_Notify
    },



};

/* ======================  End Of File  =============================== */
