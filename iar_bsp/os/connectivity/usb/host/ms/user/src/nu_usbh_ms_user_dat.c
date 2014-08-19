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
*       nu_usbh_ms_user_dat.c
*
*
* COMPONENT
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver.
*
* DESCRIPTION
*
*       This file contains defination of USBH MS User driver
*       dispatch table and subclass wrapper's dispatch tables.
*
* DATA STRUCTURES
*
*       NU_USBH_USER_DISPATCH               Dispatch table.
*       NU_USBH_MS_USER_DISPATCH            subclass wrapper dispatch
*                                           table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB definitions.
*
**************************************************************************/
/* ==================  USB Include Files ==============================  */
#include "connectivity/nu_usb.h"

/* =======================  Global Data ================================ */

NU_USBH_MS_USER *NU_USBH_MS_USER_Cb_Pt;

const NU_USBH_USER_DISPATCH usbh_ms_user_dispatch =
{
    {                                       /* NU_USB_USER_DISPATCH      */
        {                                   /* NU_USB_DISPATCH           */
            NU_USBH_MS_USER_Delete,        /* Overrides.                */
            _NU_USB_Get_Name,               /* Does not override.        */
            _NU_USB_Get_Object_Id           /* Does not override.        */
        },
        _NU_USBH_MS_USER_Connect,
        _NU_USBH_MS_USER_Disconnect
    },
    _NU_USBH_USER_Wait,
    _NU_USBH_USER_Open_Device,
    _NU_USBH_USER_Close_Device,
    _NU_USBH_USER_Remove_Device
};

#if INCLUDE_SCSI    /* SCSI */
const NU_USBH_MS_USER_DISPATCH usbh_ms_scsi_dispatch =
{
        NU_USBH_MS_SCSI_Request,
        NU_USBH_MS_SCSI_Inquiry,
        NU_USBH_MS_SCSI_Test_Unit_Ready,
        NU_USBH_MS_SCSI_Read_Capacity,
        NU_USBH_MS_SCSI_Request_Sense,
        NU_USBH_MS_SCSI_Read10,
        NU_USBH_MS_SCSI_Write10
};
#endif
#if INCLUDE_UFI   /* UFI */
const NU_USBH_MS_USER_DISPATCH usbh_ms_ufi_dispatch =
{
        NU_USBH_MS_UFI_Request,
        NU_USBH_MS_UFI_Inquiry,
        NU_USBH_MS_UFI_Test_Unit_Ready,
        NU_USBH_MS_UFI_Read_Capacity,
        NU_USBH_MS_UFI_Request_Sense,
        NU_USBH_MS_UFI_Read10,
        NU_USBH_MS_UFI_Write10
};
#endif
#if INCLUDE_SFF8020I    /* SFF-8020 */
const NU_USBH_MS_USER_DISPATCH usbh_ms_8020_dispatch =
{
        NU_USBH_MS_8020_Request,
        NU_USBH_MS_8020_Inquiry,
        NU_USBH_MS_8020_Test_Unit_Ready,
        NU_USBH_MS_8020_Read_Capacity,
        NU_USBH_MS_8020_Request_Sense,
        NU_USBH_MS_8020_Read10,
        NU_USBH_MS_8020_Write10
};
#endif
#if INCLUDE_SFF8070I    /* SFF-8070i */
const NU_USBH_MS_USER_DISPATCH usbh_ms_8070_dispatch =
{
        NU_USBH_MS_8070_Request,
        NU_USBH_MS_8070_Inquiry,
        NU_USBH_MS_8070_Test_Unit_Ready,
        NU_USBH_MS_8070_Read_Capacity,
        NU_USBH_MS_8070_Request_Sense,
        NU_USBH_MS_8070_Read10,
        NU_USBH_MS_8070_Write10

};
#endif

/* ======================  End Of File  ================================ */
