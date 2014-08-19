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
*       ike2_pkt.h
*
* COMPONENT
*
*       IKEv2 - Packet Processing
*
* DESCRIPTION
*
*       This file contains prototypes of functions related to sending of
*       IKEv2 packets.
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

#ifndef IKE2_PKT_H
#define IKE2_PKT_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Function prototypes. ****/

STATUS IKE2_Send_Packet(IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Resend_Packet(IKE2_EXCHANGE_HANDLE *handle);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_PKT_H */
