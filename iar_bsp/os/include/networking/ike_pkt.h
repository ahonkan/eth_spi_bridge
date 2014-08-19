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
*       ike_pkt.h
*
* COMPONENT
*
*       IKE - Packet
*
* DESCRIPTION
*
*       This file contains constants, data structure and function
*       prototypes needed to implement the IKE Packet component.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_PKT_H
#define IKE_PKT_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Function prototypes. ****/

STATUS IKE_Send_Packet(IKE_PHASE1_HANDLE *phase1);
STATUS IKE_Send_Phase2_Packet(IKE_PHASE2_HANDLE *phase2);
STATUS IKE_Resend_Packet(IKE_PHASE1_HANDLE *phase1);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_PKT_H */
