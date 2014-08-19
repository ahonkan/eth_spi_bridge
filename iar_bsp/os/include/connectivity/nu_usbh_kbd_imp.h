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
*       nu_usbh_kbd_imp.h               
*                                       
*
* COMPONENT
*
*       Nucleus USB Host Keyboard Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for USB Host Keyboard driver.
*
*
* DATA STRUCTURES
*
*       nu_usbh_kbd                         USB Host HID User Control
*                                           Block.
*       NU_USBH_KBD_EVENT                   A keyboard event structure.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
**************************************************************************/

#ifndef _NU_USBH_KBD_IMP_H_
#define _NU_USBH_KBD_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif


/* =================== Data Structures ===============================  */

/* The Control Block. */

#define USBH_KEY_UP   0
#define USBH_KEY_DOWN 1
#define USBH_KEY_MODIFIER 2

/* Maximum number of keys that can be down simultaneously. */
#define USBH_KBD_MAX_KEY_DOWNS 8

/* Keys that control LED state on the keyboard. */
#define USBH_KEY_NUM_LOCK     0x1
#define USBH_KEY_CAPS_LOCK    0x2
#define USBH_KEY_SCROLL_LOCK  0x4
#define USBH_KEY_COMPOSE      0x8

#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define USBH_KBD_NUM_ITEMS    0x05
#else
#define USBH_KBD_NUM_ITEMS    0x0A
#endif /* NU_USB_OPTIMIZE_FOR_SIZE */

typedef struct _nu_usbh_kbd_event
{
    /* as per bCountryCode values defined in HID spec. */
    UINT8 country_code;
    /* USB_KEY_UP or USB_KEY_DOWN. */
    UINT8 type;
    UINT8 modifier_keys;                    /*
                                             * bit 0 - L ctrl
                                             * bit 1 - L Alt
                                             * bit 2 - L shift
                                             * bit 3 - L GUI
                                             * bit 4 - R ctrl
                                             * bit 5 - R Alt
                                             * bit 6 - R shift
                                             * bit 7 - R GUI
                                             */
    /* as per table 12 of USB HID usage tables spec. */
    UINT8 scan_code;
}
NU_USBH_KBD_EVENT;

/* Per keyboard structure. */
typedef struct keyboard_device
{
    CS_NODE   node;
    NU_USBH_HID_ITEM kbd_items[USBH_KBD_NUM_ITEMS];
    VOID * handle;
    /* Bitmap of USBH_KEY_CAPS_LOCK, USBH_KEY_NUM_LOCK,
     * USBH_KEY_SCROLL_LOCK, USBH_KEY_COMPOSE. */
    UINT8 led_keys;
    UINT8 modifier_keys;
    UINT8 keys_down[USBH_KBD_MAX_KEY_DOWNS];
    
    /* To align this structure on 32-bit boundary */
    UINT8 pad[2];
}NU_USBH_KBD_DEVICE;

typedef struct nu_usbh_kbd NU_USBH_KBD;

typedef STATUS (*NU_USBH_KBD_Event_Handler)
        (NU_USBH_KBD*,  VOID *device, const NU_USBH_KBD_EVENT*);

/* Structure containing the callback routines for application. */
typedef struct _nu_usbh_kbd_callback
{
    STATUS (*Connection)          (VOID*      context,
                                   VOID*      device);

    STATUS (*Disconnection)       (VOID*      context,
                                   VOID*      device);
}NU_USBH_KBD_CALLBACK;

struct nu_usbh_kbd
{
    NU_USBH_HID_USER cb;
    NU_USBH_HID_USAGE usages[2];

    /* List of the devices currently served by the driver. */
    NU_USBH_KBD_DEVICE *keyboard_list_head;

    /* The event handler of the client. */
    NU_USBH_KBD_Event_Handler ev_handler;
    VOID*                     context;
    /* Callback pointers and context for the application. */
    NU_USBH_KBD_CALLBACK*     call_back;

};

NU_USBH_KBD_DEVICE *USBH_KBD_Find_Device(NU_USBH_KBD * cb,VOID *handle);


/* ====================Device Manager Interface ==================== */
#define USBH_KBD_Get_USAGES                  (USB_KBD_IOCTL_BASE + 0)
#define USBH_KBD_REG_EVENT_HANDLER           (USB_KBD_IOCTL_BASE + 1)
#define USBH_KBD_REG_STATE_HANDLER           (USB_KBD_IOCTL_BASE + 2)
#define USBH_KBD_DELETE                      (USB_KBD_IOCTL_BASE + 3)
#define USBH_KBD_GET_HANLDE                  (USB_KBD_IOCTL_BASE + 4)


typedef struct hid_kbd_ioctl_data
{
	void  *call_back;
	void  *context;
	void  *data_buffer;
	UINT8 usages;
	UINT8 pad[3];
	
} HID_KBD_IOCTL_DATA;

/* ===================================================================  */
#include "connectivity/nu_usbh_kbd_dat.h"

#endif      /* _NU_USBH_KBD_IMP_H_ */

/* ====================== End of File ================================  */












