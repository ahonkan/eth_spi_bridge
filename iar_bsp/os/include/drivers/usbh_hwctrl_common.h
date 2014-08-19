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
*       usbh_hwctrl_common.h
*
* COMPONENT
*
*       Generic USB Host Controller Driver
*
* DESCRIPTION
*
*       This file contains header for the wrapper functions used by the USBH
*       controller driver to interface with the DM.
*
* DATA STRUCTURES
*
*       None.
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
#ifndef NU_USBH_HWCTRL_COMMON_H
#define NU_USBH_HWCTRL_COMMON_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* IOCTL command handlers for USBH hardware controller driver.*/
STATUS USBH_HWCTRL_Handle_Initialize(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Uninitialize(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_IO_Request(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Open_Pipe(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Close_Pipe(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Modify_Pipe(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Flush_Pipe(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Enable_Int(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Disable_Int(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
STATUS USBH_HWCTRL_Handle_Get_Role(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );
STATUS USBH_HWCTRL_Handle_Start_Session(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );
STATUS USBH_HWCTRL_Handle_End_Session(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );
STATUS USBH_HWCTRL_Handle_Notify_Role_Switch(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );
#endif
STATUS USBH_HWCTRL_Handle_Execute_ISR(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Get_CB(USBH_HWCTRL_SESSION_HANDLE*, VOID**, INT);
STATUS USBH_HWCTRL_Handle_Get_Speed(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Is_Current_Available(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Release_Power(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Handle_Request_Power_Down(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT);
STATUS USBH_HWCTRL_Get_Target_Info(const CHAR * key, USBH_HWCTRL_TGT_INFO *usbh_hwctrl_tgt);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* NU_USBH_HWCTRL_COMMON_H */ 
