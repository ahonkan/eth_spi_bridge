/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

************************************************************************
*
* FILE NAME
*
*       nu_usbf_comm_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Communication Class Driver
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       Communication Class Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbf_comm_imp.h                  Internal Definitions.
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_COMM_EXT_H_
#define _NU_USBF_COMM_EXT_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */
#include "connectivity/nu_usbf_comm_imp.h"

/* ==================================================================== */
/* ==================================================================== */
/* NU_USBF_COMM Constructor. */
/* The prototype may take more parameters depending on implementation. */
STATUS NU_USBF_COMM_Create (NU_USBF_COMM *cb, CHAR *name,
                                                    NU_MEMORY_POOL *pool);

STATUS NU_USBF_COMM_Send_Notification(NU_USBF_DRVR *cb,
                                      USBF_COMM_USER_NOTIFICATION *notif,
                                      VOID *handle);

/* Initialization Callback function for USB Function Communication
 * Class.
 */
STATUS nu_os_conn_usb_func_comm_class_init(CHAR *path, INT startstop);

/* Get handle to the Function COMM class driver */
STATUS NU_USBF_COMM_Init_GetHandle (VOID** handle);

/* ====================  COMM Services ========================= */

STATUS  _NU_USBF_COMM_Examine_Intf (NU_USB_DRVR * cb,
                                    NU_USB_INTF_DESC * intf);

STATUS  _NU_USBF_COMM_Examine_Device (NU_USB_DRVR *cb,
                                      NU_USB_DEVICE_DESC * dev);

STATUS  _NU_USBF_COMM_Get_Score (NU_USB_DRVR * cb,
                                 UINT8 *score_out);

STATUS  _NU_USBF_COMM_Initialize_Intf (NU_USB_DRVR * cb,
                                       NU_USB_STACK * stack,
                                       NU_USB_DEVICE * dev,
                                       NU_USB_INTF * intf);

STATUS  _NU_USBF_COMM_Disconnect (NU_USB_DRVR * cb,
                                  NU_USB_STACK * stack,
                                  NU_USB_DEVICE * dev);

STATUS _NU_USBF_COMM_Set_Intf (NU_USB_DRVR * cb,
                               NU_USB_STACK * stack,
                               NU_USB_DEVICE * device,
                               NU_USB_INTF * intf,
                               NU_USB_ALT_SETTG * alt_settting);

STATUS  _NU_USBF_COMM_New_Setup (NU_USB_DRVR * cb,
                                 NU_USB_STACK * stack,
                                 NU_USB_DEVICE * device,
                                 NU_USB_SETUP_PKT * setup);

STATUS  _NU_USBF_COMM_New_Transfer (NU_USB_DRVR * cb,
                                    NU_USB_STACK * stack,
                                    NU_USB_DEVICE * device,
                                    NU_USB_PIPE * pipe);

STATUS  _NU_USBF_COMM_Notify (NU_USB_DRVR * cb,
                      NU_USB_STACK * stack,
                      NU_USB_DEVICE * device,
                      UINT32 event);

STATUS NU_USBF_COMM_Cancel_Io(
            NU_USBF_COMM *cb,
            VOID * handle);

STATUS _NU_USBF_COMM_Delete (VOID *cb);

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif      /* _NU_USBF_COMM_EXT_H_ */

/* ======================  End Of File  =============================== */
