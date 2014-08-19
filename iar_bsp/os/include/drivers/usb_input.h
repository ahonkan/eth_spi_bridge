/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2005
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*   usb_input.h
*
* DESCRIPTION
*
*   This file contains prototypes and externs for usb_input.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
*  nucleus.h
*  nu_usb.h
*  nu_ui.h
*
**************************************************************************/
#ifndef _USB_INPUT_H_
#define _USB_INPUT_H_

#include    "nucleus.h"
#include    "connectivity/nu_usb.h"
#include    "ui/nu_ui.h"

/* Mouse parameters. */
#define USBH_MOUSE_X_INTERCEPT               266
#define USBH_MOUSE_Y_INTERCEPT               -30
#define USBH_MOUSE_X_SLOPE                   -48
#define USBH_MOUSE_Y_SLOPE                   48

/* Function prototypes. */
INT32  USB_Mouse_Wakeup(mouseRcd *rcd, INT32 md);
INT32  USB_Keyboard_Wakeup(VOID);
INT32  USB_Mouse_Pos(mouseRcd *rcd);
STATUS USB_Mouse_Event_Handler (NU_USBH_MOUSE * cb, VOID *handle,
                                        const NU_USBH_MOUSE_EVENT *event);

#endif /* _USB_INPUT_H_ */

