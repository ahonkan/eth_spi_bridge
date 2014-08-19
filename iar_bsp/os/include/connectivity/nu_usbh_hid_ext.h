/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
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
*       nu_usbh_hid_ext.h 
*          
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       HID Class Driver.
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
*       nu_usbh_hid_imp.h                   Internal Definitions.
*
**************************************************************************/

#ifndef _NU_USBH_HID_EXT_H_
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define _NU_USBH_HID_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif


/* ==================================================================== */

#include "connectivity/nu_usbh_hid_imp.h"

/* ==================================================================== */

/* ==================================================================== */

/* NU_USBH_HID user API. */

STATUS NU_USBH_HID_Create (NU_USBH_HID * cb,
        CHAR * name,
        NU_MEMORY_POOL * pool,
        VOID * hid_task_stack_address,
        UNSIGNED hid_task_stack_size,
        OPTION hid_task_priority);

/* Function services. */

STATUS _NU_USBH_HID_Delete (VOID *cb);

STATUS _NU_USBH_HID_Initialize_Intf (NU_USB_DRVR * cb,
        NU_USB_STACK * stack,
        NU_USB_DEVICE * device,
        NU_USB_INTF * intf);

STATUS _NU_USBH_HID_Disconnect (NU_USB_DRVR * cb,
        NU_USB_STACK * stack,
        NU_USB_DEVICE * device);

STATUS NU_USBH_HID_Get_Country_Code(NU_USBH_HID * cb,
        NU_USBH_HID_USER * user,
        VOID *session,
        UINT8 *country_code_out);

STATUS NU_USBH_HID_Set_Idle(NU_USBH_HID *cb,
        NU_USBH_HID_USER * user,
        VOID *session, UINT8 report_id,
        UINT8 idle_rate);

STATUS NU_USBH_HID_Get_Items(NU_USBH_HID *cb,
        NU_USBH_HID_USER * user,
        VOID *session,
        NU_USBH_HID_ITEM *items,
        UINT8 requested_items,
        UINT8 *avail_items_out);

STATUS NU_USBH_HID_Set_Report(NU_USBH_HID *cb,
        NU_USBH_HID_USER * user,
        VOID *session, UINT8 report_id,
        UINT8 report_type, UINT16 report_length,
        UINT8 *report_data);

STATUS NU_USBH_HID_Get_Report(NU_USBH_HID *cb,
        NU_USBH_HID_USER * user,
        VOID *session, UINT8 report_id,
        UINT8 report_type, UINT16 report_length,
        UINT8 *report_data,
        UINT32 *actual_length);

STATUS NU_USBH_HID_Init_GetHandle(VOID  **handle);

#ifdef          __cplusplus
}                                           /* End of C declarations. */
#endif

/* ==================================================================== */

#endif /* _NU_USBH_HID_EXT_H_ */

/* ======================  End Of File  =============================== */

