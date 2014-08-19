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
*       nu_usbf_kb_ext.h
*
*
* COMPONENT
*
*       Nucleus USB Function Software : HID User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       keyboard user driver.
*
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
*       nu_usbf_kb_imp.h                    Internal Definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef NU_USBF_KB_EXT_H_
#define NU_USBF_KB_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#include "connectivity/nu_usbf_kb_imp.h"

/* USB Function Keyboard User IOCTLS. */
#define NU_USBF_KBD_IOCTL_BASE                  USB_HID_KBD_IOCTL_BASE
#define NU_USBF_KBD_IOCTL_REG_CALLBACK          0
#define NU_USBF_KBD_IOCTL_WAIT                  1
#define NU_USBF_KBD_IOCTL_SEND_KEY_EVENT        2
#define NU_USBF_KBD_IOCTL_GET_HID_CB            3
#define NU_USBF_KBD_IOCTL_IS_HID_DEV_CONNECTED  4
#define TOTAL_USBF_KBD_IOCTLS                   5

typedef struct _usbf_kbd_wait_data_
{
    UNSIGNED suspend;
    VOID **handle_out;
} USBF_KBD_WAIT_DATA;

typedef struct _usbf_kbd_send_key_data_
{
    CHAR **key;
    UINT8 buffersize;
    UINT8 *modifierkeybuffer;
    UINT8 modifierkeybuffersize;
} USBF_KBD_SEND_KEY_DATA;

/* ====================  Function Prototypes ========================== */

/* NU_USBF_KB Constructor. */
/* The prototype may take more parameters depending on implementation */


STATUS NU_USBF_KB_Create(NU_USBF_KB * cb,
                         CHAR *name,
                         KEYBOARD_RX_CALLBACK   rx_callback);

STATUS NU_USBF_KB_Wait(NU_USBF_KB * cb,
                       UNSIGNED suspend,
                       VOID **handle_out);


STATUS  NU_USBF_KB_Send_Key_Event(NU_USBF_KB *cb,
                                  CHAR **Key,
                                  UINT8 buffersize,
                                  UINT8 *modifierkeybuffer,
                                  UINT8 modifierkeybuffersize);


/* ====================  HID_USER_KEYBOARD Services ==================== */

STATUS _NU_USBF_KB_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS _NU_USBF_KB_Disconnect (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle);

STATUS _NU_USBF_KB_New_Command (NU_USBF_USER * cb,
                                NU_USBF_DRVR * drvr,
                                VOID *handle,
                                UINT8 *command,
                                UINT16 cmd_len,
                                UINT8 **data, UINT32 *data_len);

STATUS _NU_USBF_KB_New_Transfer(NU_USBF_USER * cb,
                                NU_USBF_DRVR * drvr,
                                VOID *handle,
                                UINT8 **data,
                                UINT32 *data_len);

STATUS _NU_USBF_KB_Tx_Done (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT8 *completed_data,
                            UINT32 completed_data_len,
                            UINT8 **data,
                            UINT32 *data_len);

STATUS _NU_USBF_KB_Notify (NU_USBF_USER * cb,
                           NU_USBF_DRVR * drvr,
                           VOID *handle,
                           UINT32 event);


STATUS _NU_USBF_KB_Delete(VOID *cb);

STATUS NU_USBF_KB_Init_GetHandle(VOID  **handle);

STATUS NU_USBF_KB_Reg_Callback( NU_USBF_KB  *cb,
                                KEYBOARD_RX_CALLBACK rx_callback);

STATUS NU_USBF_KB_USER_Bind_Interface(NU_USBF_KB *cb);

STATUS NU_USBF_KB_USER_Unbind_Interface(NU_USBF_KB *cb);

/* Function prototypes for USB Function Keyboard DM interface. */
STATUS  NU_USBF_KB_DM_Open(VOID* dev_handle);

STATUS  NU_USBF_KB_DM_Close(VOID* dev_handle);

STATUS  NU_USBF_KB_DM_Read(VOID*       dev_handle,
                           VOID*       buffer,
                           UINT32      numbyte,
                           OFFSET_T    byte_offset,
                           UINT32*     bytes_read_ptr);

STATUS  NU_USBF_KB_DM_Write(VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr);

STATUS  NU_USBF_KB_DM_IOCTL(VOID*     dev_handle,
                            INT       ioctl_num,
                            VOID*     ioctl_data,
                            INT       ioctl_data_len);

/* Add the prototypes for extra services provided by NU_USBF_KB here */

/* ==================================================================== */
#endif /* _NU_USBF_KB_EXT_H_ */

/* ======================  End Of File  =============================== */
