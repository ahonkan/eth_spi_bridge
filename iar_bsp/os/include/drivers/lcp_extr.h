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
*       lcp_extr.h
*
*   COMPONENT
*
*       LCP - Link Control Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes used by LCP and
*       also accessible to other modules.
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
#ifndef PPP_INC_LCP_EXTR_H
#define PPP_INC_LCP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

STATUS      LCP_Init(DV_DEVICE_ENTRY*);
VOID        LCP_Reset(LINK_LAYER*);
VOID        LCP_Interpret(NET_BUFFER*);
VOID        LCP_Send_Config_Req(DV_DEVICE_ENTRY*);
VOID        LCP_Check_Config_Req(NET_BUFFER*);
VOID        LCP_Check_Config_Ack(NET_BUFFER*);
VOID        LCP_Check_Config_Nak(UINT8, NET_BUFFER*);
VOID        LCP_Check_Terminate_Req(NET_BUFFER*);
VOID        LCP_Check_Terminate_Ack(NET_BUFFER*);
STATUS      LCP_Process_Request(NET_BUFFER*, NET_BUFFER**);
STATUS      LCP_Process_Nak(UINT8, NET_BUFFER*);
VOID        LCP_Send_Protocol_Reject(NET_BUFFER*);
VOID        LCP_Send_Code_Reject(NET_BUFFER*, UINT16);
VOID        LCP_Process_Code_Reject(NET_BUFFER*);
VOID        LCP_Process_Protocol_Reject(NET_BUFFER*);
UINT8       LCP_Random_Number(VOID);
UINT32      LCP_Random_Number32(VOID);
VOID        LCP_Send_Echo_Req(DV_DEVICE_ENTRY*);
VOID        LCP_Process_Echo_Req(NET_BUFFER*);
VOID        LCP_Send_Terminate_Req(DV_DEVICE_ENTRY*, UINT16);
VOID        LCP_Send_Terminate_Ack(DV_DEVICE_ENTRY*, UINT16);
STATUS      LCP_Nak_Option(LCP_FRAME*, UINT8, UINT8, UINT8*);
STATUS      LCP_Reject_Option(LCP_FRAME*, UINT8, UINT8, UINT8*);
NET_BUFFER *LCP_New_Buffer(DV_DEVICE_ENTRY*, UINT8, INT16);
STATUS      LCP_Send(DV_DEVICE_ENTRY*, NET_BUFFER*, UINT16);
UINT8       LCP_Append_Option(LCP_FRAME*, UINT8, UINT8, UINT8*);

extern CHAR StateStrings[10][10];

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_LCP_EXTR_H */
