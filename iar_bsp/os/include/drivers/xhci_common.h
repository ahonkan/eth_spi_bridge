
/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       xhci_common.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains header for the wrapper functions used by the xhci
*       driver to interface with the DM.
*
* DATA STRUCTURES
*
*	None
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nucleus.h
*       power_core.h
*       nu_services.h
*       nu_drivers.h
*       reg_api.h
*
**************************************************************************/
#ifndef NU_USBH_XHCI_COMMON_H
#define NU_USBH_XHCI_COMMON_H

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* IOCTL command handlers for XHCI controller driver.*/
STATUS XHCI_Handle_Initialize(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_IO_Request(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Open_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Close_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Modify_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Flush_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Enable_Int(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Disable_Int(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Execute_ISR(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Open_SS_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Update_device(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Initialize_Device(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Uninitialize_Device(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Unstall_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Disable_Pipe(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Reset_Bandwidth(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Get_CB(USBH_XHCI_SESSION_HANDLE*, VOID**, INT);
STATUS XHCI_Handle_Get_Speed(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Is_Current_Available(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Release_Power(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Handle_Request_Power_Down(USBH_XHCI_SESSION_HANDLE*, VOID*, INT);
STATUS XHCI_Get_Target_Info(const CHAR * key, USBH_XHCI_TGT_INFO *usbh_xhci_tgt);
STATUS USBH_XHCI_PWR_Drvr_Register_CB(DV_DEV_ID device_id, VOID *context);
STATUS USBH_XHCI_PWR_Drvr_Unregister_CB(DV_DEV_ID device_id, VOID *context);
STATUS USBH_XHCI_Set_State(VOID *inst_handle, PM_STATE_ID *state);
STATUS XHCI_Pwr_Pre_Park( VOID *inst_handle );
STATUS XHCI_Pwr_Post_Park( VOID *inst_handle );
STATUS XHCI_Pwr_Pre_Resume( VOID *inst_handle );
STATUS XHCI_Pwr_Post_Resume( VOID *inst_handle );
#endif /* NU_USBH_XHCI_COMMON_H. */