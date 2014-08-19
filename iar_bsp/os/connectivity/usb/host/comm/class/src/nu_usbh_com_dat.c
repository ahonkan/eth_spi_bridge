/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
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
*       nu_usbh_com_dat.c
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file initializes the dispatch table for Nucleus USB
*       Communication class host driver. Some of these use functionality of
*       NU_USB_DRVR basic implementation, others are either extended or
*       ignored.
*
* DATA STRUCTURES
*
*       usbh_com_dispatch                   Communication class driver
*                                           dispatch table.
*       usbh_com_data_dispatch              Communication class driver's
*                                           data dispatch table.
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

/* Nucleus USB Include Files */
#include "connectivity/nu_usb.h"

NU_USBH_COM *NU_USBH_COMM_Cb_Pt;

/* Creating a global instance for communication driver control interface's
 * dispatch table which is just of the type of USB driver dispatch table.
 */
const NU_USB_DRVR_DISPATCH usbh_com_dispatch = {
    {
    _NU_USBH_COM_Delete,
    _NU_USB_Get_Name,
    _NU_USB_Get_Object_Id
    },

    _NU_USB_DRVR_Examine_Intf,
    NU_NULL,
    _NU_USB_DRVR_Get_Score,
    NU_NULL,
    _NU_USBH_COM_Initialize_Intf,
    _NU_USBH_COM_Disconnect
};

/* Creating a global instance for communication driver data interface's
 * dispatch table which is just of the type of USB driver dispatch table.
 */
const NU_USB_DRVR_DISPATCH usbh_com_data_dispatch = {
    {
    _NU_USBH_COM_Delete,
    _NU_USB_Get_Name,
    _NU_USB_Get_Object_Id
    },

    _NU_USB_DRVR_Examine_Intf,
    NU_NULL,
    _NU_USB_DRVR_Get_Score,
    NU_NULL,
    _NU_USBH_COM_Data_Initialize_Intf,
    _NU_USBH_COM_Data_Disconnect
};

