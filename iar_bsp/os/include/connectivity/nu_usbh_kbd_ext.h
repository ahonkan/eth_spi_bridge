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
*       nu_usbh_kbd_ext.h              
*                                          
*
* COMPONENT
*
*       Nucleus USB Host Keyboard Driver.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       Keyboard Driver.
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
*       nu_usbh_kbd_imp.h                   Internal Definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBH_KBD_EXT_H_
#define _NU_USBH_KBD_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#include "connectivity/nu_usbh_kbd_imp.h"

/* ==================================================================== */

/* NU_USBH_KBD user API. */
STATUS NU_USBH_KBD_Create ( NU_USBH_KBD     *cb,
                            CHAR            *name,
                            NU_MEMORY_POOL  *pool);

STATUS _NU_USBH_KBD_Connect (NU_USB_USER    *cb,
                             NU_USB_DRVR    *class_driver,
                             VOID           *handle);

STATUS _NU_USBH_KBD_Disconnect (NU_USB_USER     *cb,
                                NU_USB_DRVR     *class_driver,
                                VOID            *handle);
STATUS _NU_USBH_KBD_Delete (VOID *cb);

STATUS _NU_USBH_KBD_Notify_Report(  NU_USBH_HID_USER    *cb,
                                    NU_USBH_HID         *driver,
                                    VOID                *handle,
                                    UINT8               *report_data,
                                    UINT16              report_len);

STATUS _NU_USBH_KBD_Get_Usages (    NU_USBH_HID_USER    *cb,
                                    NU_USBH_HID         *drvr,
                                    NU_USBH_HID_USAGE   *usage_out,
                                    UINT8               num_usages);

STATUS NU_USBH_KBD_Reg_Event_Handler (  NU_USBH_KBD                 *cb,
                                        NU_USBH_KBD_Event_Handler   func);

STATUS NU_USBH_KBD_Reg_State_Handlers (NU_USBH_KBD*           drvr_cb,
                                       VOID*                  context,
                                       NU_USBH_KBD_CALLBACK*  call_back);

STATUS NU_USBH_KBD_Init_GetHandle(VOID  **handle);

/*=================== Device Manager Interface ========================*/

STATUS  NU_USBH_KBD_DM_Open (VOID* dev_handle);

STATUS  NU_USBH_KBD_DM_Close (VOID* dev_handle);

STATUS  NU_USBH_KBD_DM_Read (VOID *session_handle,
                                        VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_read_ptr);

STATUS  NU_USBH_KBD_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr);

STATUS  NU_USBH_KBD_DM_IOCTL (VOID *session_handle,
                                        INT cmd,
                                        VOID *data,
                                        INT length);

/* ==================================================================== */
#endif /* _NU_USBH_KBD_EXT_H_ */

/* ======================  End Of File  =============================== */

