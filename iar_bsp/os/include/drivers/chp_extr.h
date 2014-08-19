/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       chp_extr.h
*
*   COMPONENT
*
*       MSCHAP - Microsoft Challenge Handshake Authentication Protocol
*
*   DESCRIPTION
*
*       This file contains the function prototypes to support chap.c
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_CHP_EXTR_H
#define PPP_INC_CHP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

STATUS CHAP_Check_Response (NET_BUFFER *);
VOID   CHAP_MD5_Encrypt (UINT8 *, UINT8 *, UINT32);
VOID   CHAP_Respond_To_Challenge (NET_BUFFER *);
VOID   CHAP_Interpret(NET_BUFFER *);
VOID   CHAP_Timer_Expire (UNSIGNED unused);
VOID   CHAP_Send_Success (NET_BUFFER *);
VOID   CHAP_Send_Failure (NET_BUFFER *);
VOID   CHAP_Send_Challenge (DV_DEVICE_ENTRY *);


STATUS CHAP_MD5_Verify(CHAP_LAYER*, UINT8*, UINT8 HUGE*);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_CHP_EXTR_H */
