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
*       ncp_extr.h
*
*   COMPONENT
*
*       NCP - Network Control Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes used by the NCP of
*       PPP and which are also accessible to other modules.
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
#ifndef PPP_INC_NCP_EXTR_H
#define PPP_INC_NCP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

STATUS          NCP_Init(DV_DEVICE_ENTRY *dev_ptr);
VOID            NCP_Reset(LINK_LAYER*);
STATUS          NCP_Change_IP_Mode(UINT8 new_mode,
                        DV_DEVICE_ENTRY *dev_ptr);
STATUS          NCP_Set_Client_IP_Address(PPP_OPTIONS *ppp_options);
VOID            NCP_Interpret(NCP_LAYER*, NET_BUFFER *buf_ptr);
VOID            NCP_Check_Config_Req(NCP_LAYER*, NET_BUFFER *buf_ptr);
VOID            NCP_Check_Config_Ack(NCP_LAYER*, NET_BUFFER *buf_ptr);
VOID            NCP_Check_Config_Nak(NCP_LAYER*, UINT8 nak_type,
                        NET_BUFFER *buf_ptr);
VOID            NCP_Check_Terminate_Req(NCP_LAYER*, NET_BUFFER *buf_ptr);
VOID            NCP_Check_Terminate_Ack(NCP_LAYER*, NET_BUFFER *buf_ptr);
VOID            NCP_Check_Code_Reject(NCP_LAYER*, NET_BUFFER *buf_ptr);

VOID            NCP_Send_Config_Req(NCP_LAYER*, DV_DEVICE_ENTRY *dev_ptr);
STATUS          NCP_Process_Request(NCP_LAYER*, NET_BUFFER *buf_ptr,
                        NET_BUFFER **ack_buf);
STATUS          NCP_Process_Nak(NCP_LAYER*, UINT8 ack_type,
                        NET_BUFFER *buf_ptr);
VOID            NCP_Event_Handler(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID            NCP_Close_IP_Layer(DV_DEVICE_ENTRY *dev_ptr);
VOID            NCP_Clean_Up_Link(DV_DEVICE_ENTRY *dev_ptr);


extern TQ_EVENT IPCP_Event;
extern TQ_EVENT IPV6CP_Event;

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_NCP_EXTR_H */
