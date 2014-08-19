/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike2_info.h
*
* COMPONENT
*
*       IKEv2 - Informational Exchange Messages
*
* DESCRIPTION
*
*       This file contains prototypes for Informational exchange handling
*       functions.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef IKE2_INFO_H
#define IKE2_INFO_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

STATUS IKE2_Dispatch_INFORMATIONAL(IKE2_PACKET *pkt, IKE2_HDR *hdr,
                                   IKE2_SA *sa, IKE2_POLICY *policy);

STATUS IKE2_Process_Notify(IKE2_EXCHANGE_HANDLE *handle);

STATUS IKE2_Generate_Notify(IKE2_EXCHANGE_HANDLE *handle,
                            UINT8 protocol_id, UINT16 notify_type,
                            VOID* notify_data, UINT16 notify_data_len,
                            UINT8 spi_size, UINT8 *spi);

STATUS IKE2_Process_Delete(IKE2_EXCHANGE_HANDLE *handle);

STATUS IKE2_Generate_Delete(IKE2_EXCHANGE_HANDLE *handle, UINT8 *spi,
                            UINT8 spi_len, UINT16 spi_count, UINT8 proto_id);

STATUS IKE2_Send_Info_Notification(IKE2_EXCHANGE_HANDLE *handle,
                                   UINT8 proto_id, UINT8 spi_size,
                                   UINT16 type, UINT8 *spi, UINT8 *data,
                                   UINT16 data_len, UINT8 xchg);

STATUS IKE2_Send_Info_Delete(IKE2_EXCHANGE_HANDLE *handle, UINT8 proto_id,
                             UINT8 spi_size, UINT16 spi_count, UINT8 *spi);

STATUS IKE2_Process_Info_Xchg(IKE2_EXCHANGE_HANDLE *handle);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif
