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
*       nu_usbf_comm_data_dat.c
*
* COMPONENT
*
*       Nucleus USB Function software : Data Interface Class Driver
*
* DESCRIPTION
*
*       This file defines the dispatch table for Data Interface
*       Class Driver.
*
* DATA STRUCTURES
*
*       usbf_comm_data_dispatch             Communication Class Driver
*                                           dispatch table.
*
* FUNCTIONS
*
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

/* Control block for Function DATA Driver */
NU_USBF_COMM_DATA *NU_USBF_COMM_DATA_Cb_Pt;

const NU_USBF_COMM_DATA_DISPATCH usbf_comm_data_dispatch =
{
    {
        {
            {
                _NU_USBF_COMM_DATA_Delete,
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
            _NU_USBF_COMM_DATA_Init_Intf,
            _NU_USBF_COMM_DATA_Disconnect
        },
        _NU_USBF_COMM_DATA_Set_Intf,
        _NU_USBF_COMM_DATA_New_Setup,
        _NU_USBF_COMM_DATA_New_Transfer,
        _NU_USBF_COMM_DATA_Notify
    }

    /* New Services of Data Interface driver if any, go here. */
};

/* ======================  End Of File  =============================== */
