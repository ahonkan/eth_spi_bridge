/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_kb_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Software : HID Key board User Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for HID Keyboard User Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_KB_IN_REPORT                IN Report data structure.
*       NU_USBF_KB_OUT_REPORT               OUT Report data structure.
*       NU_USBF_KB                          HID Keyboard User Driver
*                                           Control Block.
*       NU_USBF_KB_DISPATCH                 HID Keyboard User Driver
*                                           Dispatch Table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_user_ext.h                  Function user external
*                                           definitions.
*       nu_usbf_kb_dat.h                    Dispatch Table Definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_KB_IMP_H_
#define _NU_USBF_KB_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_user_ext.h"

#define USBF_KB_SUBCLASS                1   /*Boot Device */
#define USBF_KB_PROTOCOL                1   /*Keyboard */
#define USBF_KB_REPORT_LEN              8   /*Report Length For Keyboard*/

/* Event Flags */
#define USBF_KB_DEVICE_CONNECT          4   /*Connection event */

/* User driver constants*/
#define USBF_KB_MAX_KEYS                6   /*Maximum Keys pressed at
                                            once*/
#define USBF_KB_BUFFER_SIZE             20  /*Buffer size for key data*/
#define USBF_KB_ASCII_TABLE_SIZE        100 /*Number of ASCII keys*/

#define USBF_KB_REPORT_DESCRIPTOR_LEN   65

#define USBF_KB_LEFT_CTRL               0x01
#define USBF_KB_LEFT_SHIFT              0x02
#define USBF_KB_LEFT_ALT                0x04
#define USBF_KB_LEFT_GUI                0x08
#define USBF_KB_RIGHT_CTRL              0x10
#define USBF_KB_RIGHT_SHIFT             0x20
#define USBF_KB_RIGHT_ALT               0x40
#define USBF_KB_RIGHT_GUI               0x80
#define USBF_KB_NUM_LOCK                0x80



/*  Prototype definition for the rx_callback which should be called in
    SET_REPORT request.*/
typedef STATUS (*KEYBOARD_RX_CALLBACK)(UINT32 size,UINT8 *data_buf);

/* Keyboard IN Report*/
typedef struct _nu_usbf_kb_in_report
{
    UINT8   modifierByte;                   /* Modifier byte containing
                                            keys such as CTRL,SHIFT etc.*/
    UINT8   reserved;                       /* Reserved byte */
    /* Usage ID table data filled by user driver*/
    UINT8   keyCode[USBF_KB_MAX_KEYS];
}
NU_USBF_KB_IN_REPORT;

/* Keyboard OUT Report */
typedef struct _nu_usbf_kb_out_report
{
    UINT8   ledstates;
    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[3];
}
NU_USBF_KB_OUT_REPORT;


typedef struct _nu_usbf_kb
{
    NU_USBF_USER        parent;             /* Parent's control block. */
    NU_TIMER            Idle_Timer;         /* Timer which determines time
                                            interval for report transfers
                                            on interrupt in pipe.*/
    KEYBOARD_RX_CALLBACK rx_callback;       /*RX_Callback function
                                            pointer*/
    NU_EVENT_GROUP  device_connect_event;   /* Event Group which will be
                                            used for connection event
                                            notification*/
    NU_SEMAPHORE    connection_lock;        /* Semaphore used for device
                                             connection synchronization */
    NU_USBF_DRVR        *drvr;              /* Pointer to driver for this
                                            user */
    void    *handle;
    UINT8   Protocol;                       /* User protocol*/

    /* Device configuration allocated USB function. */
    USBF_FUNCTION       *kbd_function;
    UINT8               *kb_report_descriptor;

    UINT8   pad[3];                         /* To properly align this
                                             structure on 32-bit
                                             boundary */
}
NU_USBF_KB;

/* ============= Dispatch Table ============= */
typedef struct _nu_usbf_kb_dispatch
{
    NU_USBF_USER_DISPATCH   dispatch;
}
NU_USBF_KB_DISPATCH;

/* Timer expiration routine */
void NU_USBF_KB_Timer_expiration_routine(UNSIGNED ID);

STATUS NU_USBF_KB_Process_Current_Request(NU_USBF_KB *hiduser,
                                          NU_USBF_HID_CMD *cmd,
                                          UINT8 **data_out,
                                          UINT32 *data_len_out);
void NU_USBF_KB_Fill_Key_Data(CHAR *KeyPressed,
                              UINT8 *KeyCode);

void NU_USBF_KB_Fill_Modifier_Byte(UINT8 *modifierByte,
                                   UINT8 *modifierkeybuffer,
                                   UINT8 buffersize);

#include "connectivity/nu_usbf_kb_dat.h"
/* ==================================================================== */
#endif /* _NU_USBF_KB_IMP_H_ */

/* ======================  End Of File  =============================== */
