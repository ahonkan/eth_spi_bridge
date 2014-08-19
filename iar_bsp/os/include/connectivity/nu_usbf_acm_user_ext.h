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
*       nu_usbf_acm_user_ext.h
*
*
* COMPONENT
*
*       Nucleus USB Function Software : Remote ACM User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       ACM User Driver.
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
*       nu_usbf_acm_user_imp.h              Internal Definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_ACM_USER_EXT_H_
#define _NU_USBF_ACM_USER_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#ifdef __cplusplus
extern "C" {                        /* C declarations in C++. */
#endif

/* ==================================================================== */
#include "connectivity/nu_usbf_acm_user_imp.h"

/* ACM User IOCTLS. */
#define NU_USBF_ACM_IOCTL_BASE              USB_MODEM_IOCTL_BASE
#define NU_USBF_ACM_IOCTL_GET_COMM_DATA     0
#define NU_USBF_ACM_IOCTL_REGISTER_CB       1
#define NU_USBF_ACM_IOCTL_REG_RX_BUF        2
#define NU_USBF_ACM_IOCTL_UPDATE_STATE      3
#define NU_USBF_ACM_IOCTL_SEND_RESP         4
#define NU_USBF_ACM_IOCTL_REM_MDM_SEND_DATA 5
#define NU_USBF_ACM_IOCTL_CONFIG_XFERS      6
#define NU_USBF_ACM_IOCTL_DIS_RECEPTION     7
#define TOTAL_USBF_MODEM_IOCTLS             8

typedef struct _usbf_acm_callbacks
{
    ACMF_ENCAP_COMM_RCV_CALLBACK    encap_comm_rcv_callback;
    ACMF_NEW_TRANSFER_CALLBACK      new_transfer_callback;
    ACMF_APP_NOTIFY                 app_notify;
} USBF_ACM_CALLBACKS;

typedef struct _usbf_acm_data
{
    VOID *dev_handle;
    UINT8 *buffer;
    UINT32 length;
    UINT8  state;
} USBF_ACM_DATA;

typedef struct _usbf_comm_data_param
{
    VOID *comm_data_drvr_ptr;
    VOID *dev_handle;
    VOID *param_ptr;
} USBF_COMM_DATA_PARAM;

/* ==================================================================== */
/* NU_USBF_ACM_USER Constructor. */
/* The prototype may take more parameters depending on implementation */

/* ====================  ACM_USER Services ========================= */
#if (ACMF_VERSION_COMP >= ACMF_2_0)
STATUS NU_USBF_ACM_USER_Create (
       NU_USBF_ACM_USER*                cb,
       CHAR*                            name,
       NU_MEMORY_POOL                   *pool,
       ACMF_ENCAP_COMM_RCV_CALLBACK     encap_comm_rcv_callback,
       ACMF_NEW_TRANSFER_CALLBACK       new_transfer_callback,
       ACMF_APP_NOTIFY                  app_notify);
#else
STATUS NU_USBF_ACM_USER_Create (
       NU_USBF_ACM_USER*                cb,
       CHAR*                            name,
       ACMF_ENCAP_COMM_RCV_CALLBACK     encap_comm_rcv_callback,
       ACMF_NEW_TRANSFER_CALLBACK       new_transfer_callback,
       ACMF_APP_NOTIFY                  app_notify);
#endif

STATUS NU_USBF_ACM_USER_Bind_Interface(NU_USBF_ACM_USER *cb);

STATUS NU_USBF_ACM_USER_Unbind_Interface(NU_USBF_ACM_USER *cb);

STATUS _NU_USBF_ACM_USER_Delete(VOID *cb);

STATUS _NU_USBF_ACM_USER_Connect(
       NU_USB_USER*      cb,
       NU_USB_DRVR*      class_driver,
       VOID*             handle);

STATUS _NU_USBF_ACM_USER_Disconnect(
       NU_USB_USER*      cb,
       NU_USB_DRVR*      class_driver,
       VOID*             handle);

/* Init Function called to initialize ACM Function driver during Initialization. */
STATUS nu_os_conn_usb_func_comm_mdm_init(CHAR *path, INT startstop);

/* Get handle to the Function ACM user driver */
STATUS NU_USBF_USER_ACM_GetHandle ( VOID** handle);

/* Register Callback functions with ACM user driver */
STATUS   NU_USBF_USER_ACM_Register_Cb (
                    NU_USBF_ACM_USER                *cb,
                    ACMF_ENCAP_COMM_RCV_CALLBACK    encap_comm_rcv_callback,
                    ACMF_NEW_TRANSFER_CALLBACK      new_transfer_callback,
                    ACMF_APP_NOTIFY                 app_notify
                    );

STATUS _NU_USBF_ACM_USER_New_Command(
       NU_USBF_USER*     cb,
       NU_USBF_DRVR*     drvr,
       VOID*             handle,
       UINT8*            command,
       UINT16            cmd_len,
       UINT8**           data,
       UINT32*           data_len);

STATUS _NU_USBF_ACM_USER_New_Transfer(
       NU_USBF_USER*  cb,
       NU_USBF_DRVR*  drvr,
       VOID*          handle,
       UINT8**        data,
       UINT32*        data_len);

STATUS _NU_USBF_ACM_USER_Tx_Done(
       NU_USBF_USER*   cb,
       NU_USBF_DRVR*   drvr,
       VOID*           handle,
       UINT8*          completed_data,
       UINT32          completed_data_len,
       UINT8**         data,
       UINT32*         data_len);

STATUS _NU_USBF_ACM_USER_Notify(
       NU_USBF_USER*  cb,
       NU_USBF_DRVR*  drvr,
       VOID*          handle,
       UINT32         data_send_event);

STATUS _NU_USBF_ACM_DATA_Disconnect(
       NU_USB_USER*     cb,
       NU_USB_DRVR*     class_driver,
       VOID*            handle);

STATUS _NU_USBF_ACM_DATA_Connect(
       NU_USB_USER*      cb,
       NU_USB_DRVR*      class_driver,
       VOID*             handle);

#if (ACMF_VERSION_COMP >= ACMF_2_0) /* For ACM version 2.1 and greater */

STATUS NU_USBF_ACM_Rem_Mdm_Send_Data(
       NU_USBF_ACM_USER*        acm,
       VOID*                    handle,
       CHAR*                    info_buffer,
       UINT32                   info_length);

STATUS NU_USBF_ACM_Send_Resp(
       NU_USBF_ACM_USER*   acm,
       VOID*               handle,
       UINT8*              buffer,
       UINT32              len);

STATUS NU_USBF_ACM_Update_State(
       NU_USBF_ACM_USER*  acm,
       VOID*               handle,
       UINT8              state);

STATUS NU_USBF_ACM_Ring_Notif(
       NU_USBF_ACM_USER*        acm,
       VOID*               handle,
       BOOLEAN                  bit);

STATUS NU_USBF_ACM_Par_Err_Notif(
       NU_USBF_ACM_USER*     acm,
       VOID*               handle,
       BOOLEAN               bit);

STATUS NU_USBF_ACM_Frm_Err_Notif(
       NU_USBF_ACM_USER*      acm,
       VOID*               handle,
       BOOLEAN                bit);

STATUS NU_USBF_ACM_Overrun_Notif(
       NU_USBF_ACM_USER*      acm,
       VOID*               handle,
       BOOLEAN                bit);

STATUS NU_USBF_ACM_Brk_Detect_Notif(
       NU_USBF_ACM_USER*          acm,
       VOID*               handle,
       BOOLEAN                    bit);

STATUS NU_USBF_ACM_DSR_Notif(
       NU_USBF_ACM_USER*      acm,
       VOID*               handle,
       BOOLEAN                bit);

STATUS NU_USBF_ACM_DCD_Notif(
       NU_USBF_ACM_USER*      acm,
       VOID*               handle,
       BOOLEAN                bit);

VOID NU_USBF_ACM_NEG_SEND_BRK (
     UNSIGNED           data);

#else         /* For ACMF Version less than 2.1 */

STATUS NU_USBF_ACM_Rem_Mdm_Send_Data(
       NU_USBF_ACM_USER*        acm,
       CHAR*                    info_buffer,
       UINT32                   info_length);
STATUS NU_USBF_ACM_Send_Resp(
       NU_USBF_ACM_USER*   acm,
       UINT8*              buffer,
       UINT32              len);

STATUS NU_USBF_ACM_Update_State(
       NU_USBF_ACM_USER*  acm,
       UINT8              state);

STATUS NU_USBF_ACM_Ring_Notif(
       NU_USBF_ACM_USER*        acm,
       BOOLEAN                  bit);

STATUS NU_USBF_ACM_Par_Err_Notif(
       NU_USBF_ACM_USER*     acm,
       BOOLEAN               bit);

STATUS NU_USBF_ACM_Frm_Err_Notif(
       NU_USBF_ACM_USER*      acm,
       BOOLEAN                bit);

STATUS NU_USBF_ACM_Overrun_Notif(
       NU_USBF_ACM_USER*      acm,
       BOOLEAN                bit);

STATUS NU_USBF_ACM_Brk_Detect_Notif(
       NU_USBF_ACM_USER*          acm,
       BOOLEAN                    bit);

STATUS NU_USBF_ACM_DSR_Notif(
       NU_USBF_ACM_USER*      acm,
       BOOLEAN                bit);

STATUS NU_USBF_ACM_DCD_Notif(
       NU_USBF_ACM_USER*      acm,
       BOOLEAN                bit);

VOID NU_USBF_ACM_NEG_SEND_BRK (
     UNSIGNED           data);


#endif                              /* ACMF_VERSION_COMP */

STATUS    NU_USBF_ACM_DM_Open (VOID* dev_handle);

STATUS    NU_USBF_ACM_DM_Close(VOID* dev_handle);

STATUS    NU_USBF_ACM_DM_Read(
                            VOID*       dev_handle,
                            VOID*       buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_read_ptr);

STATUS   NU_USBF_ACM_DM_Write(
                            VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr);

STATUS   NU_USBF_ACM_DM_IOCTL(
                            VOID*     dev_handle,
                            INT       ioctl_num,
                            VOID*     ioctl_data,
                            INT       ioctl_data_len);

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations   */
#endif

#endif                                      /* _NU_USBF_ACM_USER_EXT_H_ */

/* ======================  End Of File  =============================== */
