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

***********************************************************************
*
* FILE NAME
*
*       nu_usbf_user_comm_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Function Communication driver
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       ETH User Driver.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       nu_usbf_user_comm_imp.h             Internal Definitions.
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_USER_COMM_EXT_H_
#define _NU_USBF_USER_COMM_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#include "connectivity/nu_usbf_user_comm_imp.h"

/* ==================================================================== */
/* NU_USBF_USER_COMM Constructor. */
/* The prototype may take more parameters depending on implementation. */

STATUS NU_USBF_USER_COMM_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS NU_USBF_USER_COMM_DATA_Discon (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle);

STATUS NU_USBF_USER_COMM_Wait (NU_USBF_USER_COMM * cb,
                           UNSIGNED suspend,
                           VOID **handle_out);

/* ====================  USER_COMM Services =========================== */

STATUS _NU_USBF_USER_COMM_Create (NU_USBF_USER_COMM * cb,
                            CHAR * name,
                            UINT8 bInterfaceSubclass,
                            UINT8 bInterfaceProtocol,
                            BOOLEAN reqrd_data,
                            const VOID *dispatch);

STATUS _NU_USBF_USER_COMM_Delete (VOID *cb);

STATUS _NU_USBF_USER_COMM_Connect (NU_USB_USER * cb,
                                   NU_USB_DRVR * class_driver,
                                   VOID *handle);

STATUS _NU_USBF_USER_COMM_Disconnect (NU_USB_USER * cb,
                                      NU_USB_DRVR * class_driver,
                                      VOID *handle);

STATUS _NU_USBF_USER_COMM_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS _NU_USBF_USER_COMM_DATA_Discon (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle);

STATUS _NU_USBF_USER_COMM_Wait (NU_USBF_USER_COMM * cb,
                           UNSIGNED suspend,
                           VOID **handle_out);

/* Add the prototypes for extra services provided by USER_COMM here */

/* ==================================================================== */
#endif      /* _NU_USBF_USER_COMM_EXT_H_*/

/* ======================  End Of File  =============================== */
