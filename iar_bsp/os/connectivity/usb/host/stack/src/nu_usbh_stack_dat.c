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
*        nu_usbh_stack_dat.c
*
* COMPONENT
*       Nucleus USB Host Stack
*
* DESCRIPTION
*       This file defines the dispatch table for Nucleus USB Host Stack. 
*       Some of the entries uses functionality of NU_USB_STACK basic 
*       implementation, others are extended or ignored.
*
*
* DATA STRUCTURES
*       usbh_stack_dispatch    Host Stack dispatch table.
*
* FUNCTIONS
*       None
*
* DEPENDENCIES 
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_STACK_DAT_C
#define USBH_STACK_DAT_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

/* Host Stack Dispatch Table */
const NU_USB_STACK_DISPATCH usbh_stack_dispatch = {
    {
     _NU_USBH_STACK_Delete,     /* overrides */
     _NU_USB_Get_Name,          /* does not override. */
     _NU_USB_Get_Object_Id,     /* does not override. */
     },
    _NU_USBH_STACK_Add_Hw,
    _NU_USBH_STACK_Remove_Hw,
    _NU_USBH_STACK_Register_Drvr,
    _NU_USBH_STACK_Deregister_Drvr,
    _NU_USBH_STACK_Stall_Endp,
    _NU_USBH_STACK_Unstall_Endp,
    _NU_USBH_STACK_Is_Endp_Stalled,
    _NU_USBH_STACK_Submit_IRP,
    _NU_USBH_STACK_Flush_Pipe,
    _NU_USBH_STACK_Set_Config,
    _NU_USBH_STACK_Set_Intf,
    _NU_USBH_STACK_Get_Configuration,
    _NU_USBH_STACK_Get_Interface,
    _NU_USBH_STACK_Get_Endpoint_Status,
    _NU_USBH_STACK_Get_Device_Status,
    NU_NULL,
    _NU_USBH_STACK_Get_Interface_Status,
    _NU_USBH_STACK_Lock,
    _NU_USBH_STACK_Unlock,
    _NU_USBH_STACK_Is_Valid_Device,
    _NU_USBH_STACK_Start_Session,
    _NU_USBH_STACK_End_Session,
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    _NU_USBH_STACK_Cancel_IRP,
    _NU_USBH_STACK_Function_Suspend 
#else
    _NU_USBH_STACK_Cancel_IRP
#endif
};

/************************************************************************/

#endif /* USBH_STACK_DAT_C */
/* ======================  End Of File  =============================== */
