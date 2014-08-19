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

**************************************************************************
*
* FILE NAME
*
*       nu_usbh_mouse_imp.h
*
*
* COMPONENT
*
*   Nucleus USB Host Mouse Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for USB Host Mouse driver.
*
*
* DATA STRUCTURES
*
*       nu_usbh_mouse                       USB Host mouse Control Block.
*       NU_USBH_MOUSE_EVENT                 A mouse event structure.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/

#ifndef _NU_USBH_MOUSE_IMP_H_
#define _NU_USBH_MOUSE_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* =====================  #defines ===================================  */

#define MAX_USBH_HID_MOUSE_ITEMS    4

/* =================== Data Structures ===============================  */


typedef struct _nu_usbh_mouse_event
{
    UINT8 buttons;
#define NU_USBH_LEFT_BUTTON_STATE(a) (a) & 0x1
#define NU_USBH_RIGHT_BUTTON_STATE(a)(a) & 0x2
#define NU_USBH_MIDDLE_BUTTON_STATE(a) (a) & 0x4
#define NU_USBH_SIDE_BUTTON_STATE(a) (a) & 0x8

    INT8 x,y;
    INT8 wheel;
}
NU_USBH_MOUSE_EVENT;

typedef struct usbh_item_report_info
{
    UINT32 rep_id;
    UINT32 offset;
}
USBH_ITEM_REPORT_INFO;

/* Per mouse structure. */
typedef struct mouse_device
{
    CS_NODE   node;
    /* number of filled items in the following array. */
    UINT8 num_items;
    NU_USBH_HID_ITEM mouse_items[MAX_USBH_HID_MOUSE_ITEMS];
    VOID * handle;
    /* Does the mouse have a wheel? */
    UINT8 wheel_present;
    NU_USBH_MOUSE_EVENT event;
    USBH_ITEM_REPORT_INFO report_info[4];

    /* To align this structure on 32-bit boundary */
    UINT8 pad[2];

}NU_USBH_MOUSE_DEVICE;

typedef struct nu_usbh_mouse NU_USBH_MOUSE;

typedef STATUS (*NU_USBH_MOUSE_Event_Handler)
        (NU_USBH_MOUSE*,  VOID *device, const NU_USBH_MOUSE_EVENT*);

/* Structure containing the callback routines for application. */
typedef struct _nu_usbh_mouse_callback
{
    STATUS (*Connection)          (VOID*      context,
                                   VOID*      device);

    STATUS (*Disconnection)       (VOID*      context,
                                   VOID*      device);
}NU_USBH_MOUSE_CALLBACK;

struct nu_usbh_mouse
{
    NU_USBH_HID_USER cb;
    /* Button, X,Y, Wheel pages. */
    NU_USBH_HID_USAGE usages[4];
    /* List of the devices currently served by the driver. */
    NU_USBH_MOUSE_DEVICE *mouse_list_head;
    /* The event handler of the client. */
    NU_USBH_MOUSE_Event_Handler ev_handler;
    /* Callback pointers and context for the application. */
    NU_USBH_MOUSE_CALLBACK*     call_back;
    VOID*                       context;
};

NU_USBH_MOUSE_DEVICE *USBH_Mouse_Find_Device(NU_USBH_MOUSE * cb,
                                VOID *handle);

/* ====================Device Manager Interface ==================== */
#define USBH_MSE_Get_USAGES                  (USB_MSE_IOCTL_BASE + 0)
#define USBH_MSE_REG_EVENT_HANDLER           (USB_MSE_IOCTL_BASE + 1)
#define USBH_MSE_REG_STATE_HANDLER           (USB_MSE_IOCTL_BASE + 2)
#define USBH_MSE_DELETE                      (USB_MSE_IOCTL_BASE + 3)
#define USBH_MSE_GET_HANDLE                  (USB_MSE_IOCTL_BASE + 4)
#define USBH_MSE_GET_INFO                    (USB_MSE_IOCTL_BASE + 5)

typedef struct hid_mse_ioctl_data
{
    void  *call_back;
    void  *context;
    void  *data_buffer;
    UINT8 usages;
    UINT8 num_buttons;
    BOOLEAN wheel_present;
    UINT8 pad[1];

} HID_MSE_IOCTL_DATA;
/* ===================================================================  */
#include "connectivity/nu_usbh_mouse_dat.h"

#endif /* _NU_USBH_MOUSE_IMP_H_ */

/* ====================== End of File ================================  */

