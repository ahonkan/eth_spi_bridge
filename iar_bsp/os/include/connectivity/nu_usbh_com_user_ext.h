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
*       nu_usbh_com_user_ext.h
*
*
* COMPONENT
*
*       Nucleus USB host software : Communication user driver.
*
* DESCRIPTION
*
*       This file contains definitions for external interfaces exposed by
*       Communication Base user driver.
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
*       nu_usbh_com_user_imp.h              Internal definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_USER_EXT_H_
#define _NU_USBH_COM_USER_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Making internals visible */
#include "connectivity/nu_usbh_com_user_imp.h"

/* ========================= Functions Prototypes ====================== */
/* NU_USBH_COM_USER Constructor. */

STATUS _NU_USBH_COM_USER_Create (
       NU_USBH_COM_USER*          pcb_user_drvr,
       CHAR*                      p_name,
       NU_MEMORY_POOL*            p_memory_pool,
       UINT8                      subclass,
       NU_USBH_COM_USER_HDL*      p_handler,
       NU_USBH_COM_USER_DISPATCH* usbh_com_user_dispatch);

STATUS _NU_USBH_COM_USER_Delete (
       VOID*                      pcb_user_drvr);

STATUS NU_USBH_COM_USER_Wait (
       NU_USBH_COM_USER*          pcb_user_drvr,
       UNSIGNED                   suspend,
       VOID**                     handle_out);

STATUS NU_USBH_COM_USER_Send_Data (
       NU_USBH_COM_USR_DEVICE*    pcb_curr_device,
       UINT8*                     p_file_data,
       UINT32                     file_size);

STATUS NU_USBH_COM_USER_Get_Data (
       NU_USBH_COM_USR_DEVICE*    pcb_curr_device,
       UINT8*                     p_file_data,
       UINT32                     file_size,
       UINT32*                    actual_length);

STATUS NU_USBH_COM_USER_Register (
       NU_USBH_COM_USER*          pcb_user_drvr,
       NU_USBH_COM*               pcb_com_drvr);

STATUS NU_USBH_COM_USER_Start_Polling (
       NU_USBH_COM_USR_DEVICE*    pcb_curr_device);

STATUS NU_USBH_COM_USER_Stop_Polling (
       NU_USBH_COM_USR_DEVICE*    pcb_curr_device);

STATUS NU_USBH_COM_USER_Create_Polling(
       NU_USBH_COM_USER*          pcb_user_drvr,
       NU_USBH_COM_USR_DEVICE*    pcb_curr_device);

STATUS NU_USBH_COM_USER_Delete_Polling(
       NU_USBH_COM_USR_DEVICE*    pcb_curr_device);

STATUS NU_USBH_COM_USER_Resume_Device (VOID *session);

STATUS NU_USBH_COM_USER_Suspend_Device (VOID *session);


/* Add the prototypes for extra services provided by COM_USER here */

#endif /* _NU_USBH_COM_USER_EXT_H_ */
