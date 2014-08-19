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
* FILE NAME
*
*       nu_usbf_eth_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Ethernet User Driver.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       Ethernet User Driver.
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
*       nu_usbf_eth_imp.h                   Internal Definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_ETH_EXT_H_
#define _NU_USBF_ETH_EXT_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#include "connectivity/nu_usbf_eth_imp.h"

/* ==================================================================== */
/* NU_USBF_ETH Constructor. */
/* The prototype may take more parameters depending on implementation. */
STATUS NU_USBF_ETH_Create (NU_USBF_ETH *cb,
                           CHAR *name,
#if (EF_VERSION_COMP >= EF_2_0)
                           NU_MEMORY_POOL       *pool,
#endif                      
                           COMMF_APP_NOTIFY     app_notify,
                           COMMF_APP_RX_CALLBACK   rcvd_cb,
                           COMMF_APP_TX_DONE       tx_done,
                           COMMF_APP_IOCTL         ioctl);
                           
STATUS NU_USBF_ETH_Bind_Interface(NU_USBF_ETH *cb);

STATUS NU_USBF_ETH_Unbind_Interface(NU_USBF_ETH *cb);

#if (EF_VERSION_COMP >= EF_2_0)
STATUS NU_USBF_ETH_Send_Connection(NU_USBF_ETH *eth,VOID *handle,
                                UINT32 us_bit_rate, UINT32 ds_bit_rate);

STATUS NU_USBF_ETH_Send_Disconnection(NU_USBF_ETH *eth, VOID *handle);

STATUS NU_USBF_ETH_Send_Encap_Resp(NU_USBF_ETH *eth,VOID *handle,
                                    UINT8 *data, UINT16 length);

STATUS NU_USBF_ETH_Send_Notif(NU_USBF_ETH *cb, VOID *handle,UINT8 first_notif,
                                                     UINT8 second_notif);
/* Data Transfer related functions. */
STATUS NU_USBF_ETH_Send_Data(NU_USBF_ETH *cb, VOID *handle,
                    UINT8 *buffer,
                    UINT16 length);

#else
STATUS NU_USBF_ETH_Send_Connection(NU_USBF_ETH *eth,
                                UINT32 us_bit_rate, UINT32 ds_bit_rate);

STATUS NU_USBF_ETH_Send_Disconnection(NU_USBF_ETH *eth);

STATUS NU_USBF_ETH_Send_Encap_Resp(NU_USBF_ETH *eth,
                                    UINT8 *data, UINT16 length);

STATUS NU_USBF_ETH_Send_Notif(NU_USBF_ETH *cb,UINT8 first_notif,
                                              UINT8 second_notif);
/* Data Transfer related functions. */
STATUS NU_USBF_ETH_Send_Data(NU_USBF_ETH *cb,
                    UINT8 *buffer,
                    UINT16 length);
#endif

STATUS nu_os_conn_usb_func_comm_eth_init(CHAR *path, INT startstop);

STATUS NU_USBF_ETH_GetHandle ( VOID** handle);

STATUS NU_USBF_ETH_Register_Cb (NU_USBF_ETH *cb,
                                COMMF_APP_NOTIFY     app_notify,
                                COMMF_APP_RX_CALLBACK   rcvd_cb,
                                COMMF_APP_TX_DONE       tx_done,
                                COMMF_APP_IOCTL         ioctl);

/* ====================  ETH Services ========================= */

STATUS _NU_USBF_ETH_Connect (NU_USB_USER * cb, NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS _NU_USBF_ETH_Disconnect (NU_USB_USER * cb,
                                NU_USB_DRVR * class_driver,
                                VOID *handle);

STATUS _NU_USBF_ETH_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data, UINT32 *data_len);

STATUS _NU_USBF_ETH_New_Transfer (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle, UINT8 **data,
                                  UINT32 *data_len);

STATUS _NU_USBF_ETH_Tx_Done (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle,
                                  UINT8 *completed_data,
                                  UINT32 completed_data_len,
                                  UINT8 **data, UINT32 *data_len);

STATUS _NU_USBF_ETH_Notify (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT32 event);

STATUS _NU_USBF_ETH_Delete (VOID *cb);

STATUS _NU_USBF_ETH_DATA_Disconnect (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle);

STATUS _NU_USBF_ETH_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS    NU_USBF_ETH_DM_Open (VOID* dev_handle);

STATUS    NU_USBF_ETH_DM_Close(VOID* dev_handle);

STATUS    NU_USBF_ETH_DM_Read( 
                            VOID*          dev_handle,
                            VOID*          buffer,
                            UINT32         numbyte,
                            OFFSET_T       byte_offset,
                            UINT32*        bytes_read_ptr);
                            
STATUS   NU_USBF_ETH_DM_Write(
                            VOID*        dev_handle,
                            const VOID*  buffer,
                            UINT32       numbyte,
                            OFFSET_T     byte_offset,
                            UINT32*      bytes_written_ptr);
                            
STATUS   NU_USBF_ETH_DM_IOCTL (
                            VOID*        dev_handle,
                            INT          ioctl_num,
                            VOID*        ioctl_data,
                            INT          ioctl_data_len);         
/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_ETH_EXT_H_ */

/* ======================  End Of File  =============================== */
