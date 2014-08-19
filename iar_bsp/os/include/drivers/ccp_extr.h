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
*       ccp_extr.h
*
*   COMPONENT
*
*       CCP - Compression Control Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes used by CCP.
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
#ifndef PPP_INC_CCP_EXTR_H
#define PPP_INC_CCP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

extern TQ_EVENT PPP_CCP_Event;

STATUS      CCP_Init(DV_DEVICE_ENTRY*);
VOID        CCP_Reset(LINK_LAYER*);
VOID        CCP_Interpret(NET_BUFFER*);
VOID        CCP_Data_Interpret(NET_BUFFER*);
VOID        CCP_Send_Config_Req(DV_DEVICE_ENTRY*);
VOID        CCP_Check_Config_Req(NET_BUFFER*);
VOID        CCP_Check_Config_Ack(NET_BUFFER*);
VOID        CCP_Check_Config_Nak(UINT8, NET_BUFFER*);
VOID        CCP_Check_Terminate_Req(NET_BUFFER*);
VOID        CCP_Check_Terminate_Ack(NET_BUFFER*);
VOID        CCP_Check_Reset_Req(NET_BUFFER*);
STATUS      CCP_Process_Request(NET_BUFFER*, NET_BUFFER**);
STATUS      CCP_Process_Nak(UINT8, NET_BUFFER*);
VOID        CCP_Send_Protocol_Reject(NET_BUFFER*);
VOID        CCP_Send_Code_Reject(NET_BUFFER*, UINT16);
VOID        CCP_Process_Code_Reject(NET_BUFFER*);
VOID        CCP_Send_Reset_Req(DV_DEVICE_ENTRY*, UINT16);
VOID        CCP_Send_Reset_Ack(DV_DEVICE_ENTRY*, UINT16);
VOID        CCP_Send_Terminate_Req(DV_DEVICE_ENTRY*, UINT16);
VOID        CCP_Send_Terminate_Ack(DV_DEVICE_ENTRY*, UINT16);
VOID        CCP_Event_Handler(TQ_EVENT, UNSIGNED, UNSIGNED);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_CCP_EXTR_H */
