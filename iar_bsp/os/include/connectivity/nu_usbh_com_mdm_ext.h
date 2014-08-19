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

***************************************************************************
*
* FILE NAME
*
*     nu_usbh_com_mdm_ext.h
*
* COMPONENT
*     Nucleus USB host software : Communication user driver
*
* DESCRIPTION
*     This file contains definitions for external Interfaces exposed by
*     Communication Modem user driver.
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*     None
*
* DEPENDENCIES
*     nu_usbh_mdm_imp.h         Internal Definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_MDM_EXT_H_
#define _NU_USBH_COM_MDM_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Making internals visible */
#include "connectivity/nu_usbh_com_mdm_imp.h"

/* ACM User IOCTLS. */
#define NU_USBH_MDM_IOCTL_BASE                  USB_MODEM_IOCTL_BASE
#define NU_USBH_MDM_IOCTL_REG_EVT_HDL           0
#define NU_USBH_MDM_IOCTL_CREATE_POLL           1
#define NU_USBH_MDM_IOCTL_START_POLL            2
#define NU_USBH_MDM_IOCTL_STOP_POLL             3
#define NU_USBH_MDM_IOCTL_DELETE_POLL           4
#define NU_USBH_MDM_IOCTL_GET_LINE_CODE         5
#define NU_USBH_MDM_IOCTL_SET_LINE_CODE         6
#define NU_USBH_MDM_IOCTL_GET_DEV_CB            7
#define NU_USBH_MDM_IOCTL_SET_RX_DATA_BUF       8
#define NU_USBH_MDM_IOCTL_SET_RX_DATA_LEN       9
#define NU_USBH_MDM_IOCTL_SET_RX_DATA_TRFR_LEN  10
#define NU_USBH_MDM_IOCTL_GET_RX_DATA_TRFR_LEN  11
#define TOTAL_USBH_MODEM_IOCTLS                 12

/* ========================= Functions Prototypes ====================== */

/* Add the prototypes for extra services provided by COM_MDM here */
STATUS NU_USBH_COM_MDM_Create (
           NU_USBH_COM_MDM*,
           CHAR*,
           NU_MEMORY_POOL*,
           NU_USBH_COM_MDM_HANDL*);

STATUS _NU_USBH_COM_MDM_Delete (
           VOID*);

STATUS _NU_USBH_COM_MDM_Connect_Handler(
           NU_USB_USER*,
           NU_USB_DRVR*,
           VOID*,
           VOID*);

STATUS _NU_USBH_COM_MDM_Disconnect_Handler(
           NU_USB_USER*,
           NU_USB_DRVR*,
           VOID*);

STATUS _NU_USBH_COM_MDM_Intr_Handler(
           VOID*,
           NU_USBH_COM_XBLOCK*);

STATUS NU_USBH_COM_MDM_Send_Char(
           NU_USBH_COM_MDM_DEVICE*,
           UINT8);

STATUS NU_USBH_COM_MDM_Send_Buffer (
           NU_USBH_COM_MDM_DEVICE*);

STATUS NU_USBH_MDM_Set_Rx_Data_Buffer(NU_USBH_COM_MDM_DEVICE*,
                                      VOID*);

STATUS NU_USBH_MDM_Set_Rx_Data_Length(NU_USBH_COM_MDM_DEVICE*,
                                      UINT32);

STATUS NU_USBH_MDM_Set_Rx_Data_Transfer_Length(NU_USBH_COM_MDM_DEVICE*,
                                               UINT32);

STATUS NU_USBH_MDM_Get_Rx_Data_Transfer_Length(NU_USBH_COM_MDM_DEVICE*,
                                               UINT32*);

STATUS nu_os_conn_usb_host_comm_mdm_init(CHAR *path, INT startstop);

STATUS NU_USBH_COM_MDM_GetHandle(VOID  **handle);

STATUS NU_USBH_MDM_Reg_Event_Handler ( NU_USBH_COM_MDM * cb,
                                       NU_USBH_COM_MDM_HANDL* p_handlers);

STATUS    NU_USBH_MDM_DM_Open (VOID* dev_handle);

STATUS    NU_USBH_MDM_DM_Close(VOID* dev_handle);

STATUS NU_USBH_MDM_DM_Read( VOID*       dev_handle,
                            VOID*       buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_read_ptr);

STATUS NU_USBH_MDM_DM_Write(VOID*          dev_handle,
                            const VOID*    buffer,
                            UINT32         numbyte,
                            OFFSET_T       byte_offset,
                            UINT32*        bytes_written_ptr);

STATUS NU_USBH_MDM_DM_IOCTL(VOID*          dev_handle,
                            INT            ioctl_num,
                            VOID*          ioctl_data,
                            INT            ioctl_data_len);

#endif /* _NU_USBH_COM_MDM_EXT_H_ */
