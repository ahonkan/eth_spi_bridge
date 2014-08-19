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
*       nu_usbf_mse_ext.h
*
*
* COMPONENT
*
*       Nucleus USB Function Software : HID User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       HID mouse user driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS

*       None.
*
* DEPENDENCIES

*       nu_usbf_mse_imp.h                   Internal Definitions.
*
**************************************************************************/

/* =====================================================================*/
#ifndef _NU_USBF_MSE_EXT_H_
#define _NU_USBF_MSE_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#include "connectivity/nu_usbf_mse_imp.h"

/* USB Function Mouse User IOCTLS. */
#define NU_USBF_MSE_IOCTL_BASE                  USB_HID_MSE_IOCTL_BASE
#define NU_USBF_MSE_IOCTL_WAIT                  0
#define NU_USBF_MSE_IOCTL_SEND_LFT_BTN_CLICK    1
#define NU_USBF_MSE_IOCTL_SEND_RHT_BTN_CLICK    2
#define NU_USBF_MSE_IOCTL_SEND_MDL_BTN_CLICK    3
#define NU_USBF_MSE_IOCTL_MOVE_POINTER          4
#define NU_USBF_MSE_IOCTL_GET_HID_CB            5
#define NU_USBF_MSE_IOCTL_IS_HID_DEV_CONNECTED  6
#define TOTAL_USBF_MSE_IOCTLS                   7

typedef struct _usbf_mse_wait_data_
{
    UNSIGNED suspend;
    VOID **handle_out;
} USBF_MSE_WAIT_DATA;

/* ====================  Function Prototypes ========================== */

/* NU_USBF_MSE Constructor. */
/* The prototype may take more parameters depending on implementation */


STATUS NU_USBF_MSE_Create(NU_USBF_MSE *cb,
                          char *name);

STATUS NU_USBF_MSE_Wait(NU_USBF_MSE *cb,
                        UNSIGNED suspend,
                        void **handle_out);

STATUS  NU_USBF_MSE_Send_Data(NU_USBF_MSE *cb,
                              UINT8   *buffer,
                              UINT32  length);

STATUS  NU_USBF_MSE_Send_Mdl_Btn_Click(NU_USBF_MSE    *cb);

STATUS  NU_USBF_MSE_Send_Lft_Btn_Click(NU_USBF_MSE  *cb);

STATUS  NU_USBF_MSE_Send_Rht_Btn_Click(NU_USBF_MSE *cb);

STATUS  NU_USBF_MSE_Move_Pointer(NU_USBF_MSE  *cb,
                                 UINT8 X,
                                 UINT8 Y);

STATUS NU_USBF_MSE_Init_GetHandle (VOID  **handle);

/* ====================  MOUSE_USER Services ========================= */

STATUS _NU_USBF_MSE_Connect (NU_USB_USER * cb,
                             NU_USB_DRVR * class_driver,
                             void *handle);

STATUS _NU_USBF_MSE_Disconnect (NU_USB_USER * cb,
                                NU_USB_DRVR * class_driver,
                                VOID *handle);

STATUS _NU_USBF_MSE_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data,
                                 UINT32 *data_len);

STATUS _NU_USBF_MSE_New_Transfer(NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 void *handle,
                                 UINT8 **data,
                                 UINT32 *data_len);

STATUS _NU_USBF_MSE_Tx_Done(NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT8 *completed_data,
                            UINT32 completed_data_len,
                            UINT8 **data,
                            UINT32 *data_len);

STATUS _NU_USBF_MSE_Notify(NU_USBF_USER * cb,
                           NU_USBF_DRVR * drvr,
                           void *handle,
                           UINT32 event);

STATUS _NU_USBF_MSE_Delete(VOID *cb);

STATUS NU_USBF_MSE_USER_Bind_Interface(NU_USBF_MSE *cb);

STATUS NU_USBF_MSE_USER_Unbind_Interface(NU_USBF_MSE *cb);

/* Function prototypes for USB Function Mouse DM interface. */
STATUS  NU_USBF_MSE_DM_Open(VOID* dev_handle);

STATUS  NU_USBF_MSE_DM_Close(VOID* dev_handle);

STATUS  NU_USBF_MSE_DM_Read(VOID*       dev_handle,
                            VOID*       buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_read_ptr);

STATUS  NU_USBF_MSE_DM_Write(VOID*       dev_handle,
                             const VOID* buffer,
                             UINT32      numbyte,
                             OFFSET_T    byte_offset,
                             UINT32*     bytes_written_ptr);

STATUS  NU_USBF_MSE_DM_IOCTL(VOID*     dev_handle,
                             INT       ioctl_num,
                             VOID*     ioctl_data,
                             INT       ioctl_data_len);

/* Add the prototypes for extra services provided by MOUSE_USER here */

/* ==================================================================== */
#endif /* _NU_USBF_MSE_EXT_H_ */

/* ======================  End Of File  =============================== */
