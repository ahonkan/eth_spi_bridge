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
*       nu_usbf_ndis_user_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Remote NDIS User Driver
*
* DESCRIPTION
*       This file contains definitions for external Interfaces exposed by
*       NDIS User Driver.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*       nu_usbf_ndis_user_imp.h         Internal Definitions.
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_NDIS_USER_EXT_H_
#define _NU_USBF_NDIS_USER_EXT_H_

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
#include "connectivity/nu_usbf_ndis_user_imp.h"

/* ==================================================================== */
/* NU_USBF_NDIS_USER Constructor. */
/* The prototype may take more parameters depending on implementation */
STATUS NU_USBF_NDIS_USER_Create (NU_USBF_NDIS_USER *cb, CHAR *name,
#if (NF_VERSION_COMP >= NF_2_0)
                                 NU_MEMORY_POOL *pool,
#endif
                                 COMMF_APP_NOTIFY     conn_handler,
                                 COMMF_APP_RX_CALLBACK   rcvd_cb,
                                 COMMF_APP_TX_DONE       tx_done,
                                 COMMF_APP_IOCTL         ioctl);
                                 
STATUS NU_USBF_NDIS_USER_Bind_Interface(NU_USBF_NDIS_USER *cb);

STATUS NU_USBF_NDIS_USER_Unbind_Interface(NU_USBF_NDIS_USER *cb);

#if (NF_VERSION_COMP >= NF_2_0)
STATUS NU_USBF_NDIS_Send_Eth_Frame(NU_USBF_NDIS_USER * ndis,VOID *handle,
                            UINT8 *data_buffer, UINT32 data_length,
                            UINT8 *info_buffer, UINT32 info_length );

STATUS NU_USBF_NDIS_Send_Status(NU_USBF_NDIS_USER * ndis,VOID *handle,
                    UINT32 status_val,
                    UINT8 * status_buffer,
                    UINT32 length);


STATUS NU_USBF_NDIS_Get_Conn_Status(NU_USBF_NDIS_USER * ndis,
                                    VOID *handle,
                                    UINT32* status_out);
#else
STATUS NU_USBF_NDIS_Send_Eth_Frame(NU_USBF_NDIS_USER * ndis,
                            UINT8 *data_buffer, UINT32 data_length,
                            UINT8 *info_buffer, UINT32 info_length);

STATUS NU_USBF_NDIS_Send_Status(NU_USBF_NDIS_USER * ndis,
                    UINT32 status_val,
                    UINT8 * status_buffer,
                    UINT32 length
                    );

STATUS NU_USBF_NDIS_Get_Conn_Status(NU_USBF_NDIS_USER * ndis,
                                    UINT32* status_out);
#endif
/* ====================  NDIS_USER Services ========================= */

STATUS _NU_USBF_NDIS_USER_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS _NU_USBF_NDIS_USER_Disconnect (NU_USB_USER * cb,
                                NU_USB_DRVR * class_driver,
                                VOID *handle);

STATUS _NU_USBF_NDIS_USER_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data, UINT32 *data_len);

STATUS _NU_USBF_NDIS_USER_New_Transfer (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle, UINT8 **data,
                                  UINT32 *data_len);

STATUS _NU_USBF_NDIS_USER_Tx_Done (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle,
                                  UINT8 *completed_data,
                                  UINT32 completed_data_len,
                                  UINT8 **data, UINT32 *data_len);

STATUS _NU_USBF_NDIS_USER_Notify (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT32 event);

STATUS _NU_USBF_NDIS_USER_Delete (VOID *cb);

STATUS _NU_USBF_NDIS_DATA_Disconnect (NU_USB_USER * cb,
                                    NU_USB_DRVR * class_driver,
                                    VOID *handle);

STATUS _NU_USBF_NDIS_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

STATUS nu_os_conn_usb_func_comm_ndis_init(CHAR *path, INT startstop);

STATUS NU_USBF_NDIS_USER_GetHandle ( VOID** handle);

STATUS   NU_USBF_NDIS_USER_Register_Cb (NU_USBF_NDIS_USER *cb,
                                COMMF_APP_NOTIFY     conn_handler,
                                COMMF_APP_RX_CALLBACK   rcvd_cb,
                                COMMF_APP_TX_DONE       tx_done,
                                COMMF_APP_IOCTL         ioctl);

STATUS NU_USBF_NDIS_Set_MAC_Address(NU_USBF_NDIS_USER   *ndis,
                                    VOID                *handle,
                                    UINT8               *buffer,
                                    UINT8               buffer_len);

STATUS NU_USBF_NDIS_Get_MAC_Address(NU_USBF_NDIS_USER   *ndis,
                                    VOID                *handle,
                                    UINT8               *mac_address);

STATUS    NU_USBF_RNDIS_DM_Open (VOID* dev_handle);

STATUS    NU_USBF_RNDIS_DM_Close(VOID* dev_handle);

STATUS    NU_USBF_RNDIS_DM_Read( 
                            VOID*       dev_handle,
                            VOID*       buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_read_ptr);
                            
STATUS   NU_USBF_RNDIS_DM_Write(
                            VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr);
                            
STATUS   NU_USBF_RNDIS_DM_IOCTL(
                            VOID*     dev_handle,
                            INT       ioctl_num,
                            VOID*     ioctl_data,
                            INT       ioctl_data_len);                                                        

/* ==================================================================== */

#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_NDIS_USER_EXT_H_*/

/* ======================  End Of File  =============================== */
