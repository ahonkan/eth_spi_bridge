/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
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
*     nu_usbh_com_mdm_dat.c
*
* COMPONENT
*     Nucleus USB host software : Communication user driver.
*
* DESCRIPTION
*     This file defines the dispatch table for Communication Modem user
*     driver.
*
* DATA STRUCTURES
*     usbh_com_mdm_dispatch    Communication Modem user driver dispatch 
*                              table.
*
* FUNCTIONS
*     None
*
* DEPENDENCIES
*     nu_usb.h              All USB definitions. 
*
**************************************************************************/

/* ======================  USB Include Files ==========================  */
#include "connectivity/nu_usb.h"

/* ======================= Global data ================================  */

NU_USBH_COM_MDM *NU_USBH_COM_MDM_Cb_Pt = NU_NULL;

/* Based on the Communication base user driver */

const NU_USBH_COM_USER_DISPATCH usbh_com_mdm_dispatch = {
    {
        {
            {
                _NU_USBH_COM_MDM_Delete,
                _NU_USB_Get_Name,
                _NU_USB_Get_Object_Id
            },

            NU_NULL,
            _NU_USBH_COM_MDM_Disconnect_Handler
        },

        _NU_USBH_USER_Wait,
        _NU_USBH_USER_Open_Device,
        _NU_USBH_USER_Close_Device,
        _NU_USBH_USER_Remove_Device
    },
    _NU_USBH_COM_MDM_Connect_Handler,
    _NU_USBH_COM_MDM_Intr_Handler
};
