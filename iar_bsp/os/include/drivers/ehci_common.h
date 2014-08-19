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
*       ehci_common.h
*
* COMPONENT
*
*       USB EHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains header for the wrapper functions used by the EHCI
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
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
**************************************************************************/
#ifndef NU_USBH_EHCI_COMMON_H
#define NU_USBH_EHCI_COMMON_H

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* IOCTL command handlers for EHCI controller driver.*/
STATUS EHCI_Handle_Initialize(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Uninitialize(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_IO_Request(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Open_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Close_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Modify_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Flush_Pipe(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Enable_Int(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Disable_Int(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Execute_ISR(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Get_CB(USBH_EHCI_SESSION_HANDLE*, VOID**, INT);
STATUS EHCI_Handle_Get_Speed(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Is_Current_Available(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Release_Power(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Handle_Request_Power_Down(USBH_EHCI_SESSION_HANDLE*, VOID*, INT);
STATUS EHCI_Get_Target_Info(const CHAR * key, USBH_EHCI_TGT_INFO *usbh_ehci_tgt);
#endif  /* NU_USBH_EHCI_COMMON_H */ 
