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
*       ohci_common.h
*
* COMPONENT
*
*       USB OHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains header for the wrapper functions used by the OHCI
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
#ifndef NU_USBH_OHCI_COMMON_H
#define NU_USBH_OHCI_COMMON_H

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* IOCTL command handlers for OHCI controller driver.*/
STATUS OHCI_Handle_Initialize(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Uninitialize(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_IO_Request(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Open_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Close_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Modify_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Flush_Pipe(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Enable_Int(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Disable_Int(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Execute_ISR(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Get_CB(USBH_OHCI_SESSION_HANDLE*, VOID**, INT);
STATUS OHCI_Handle_Get_Speed(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Is_Current_Available(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Release_Power(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Handle_Request_Power_Down(USBH_OHCI_SESSION_HANDLE*, VOID*, INT);
STATUS OHCI_Get_Target_Info(const CHAR * key, USBH_OHCI_TGT_INFO *usbh_ohci_tgt);

#endif  /* NU_USBH_OHCI_COMMON_H */ 
