/*************************************************************************
*
*            Copyright 2009 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       dbg_rsp_extr.h
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP)
*
*   DESCRIPTION
*
*       This file contains function prototypes for the RSP Support
*       Component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RSP_Initialize
*       RSP_Register_Debug_Server_Callback
*       RSP_Process_Packet
*       RSP_Com_Error_Notify
*       RSP_Com_Disconnect_Notify
*       RSP_Send_Notification
*       RSP_Check_Packet
*       RSP_Application_State_Begin
*       RSP_Application_State_End
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef RSP_EXTR_H
#define RSP_EXTR_H

/* RSP Support Component function prototypes. */
RSP_STATUS      RSP_Initialize(VOID *    pMemory);
RSP_STATUS      RSP_Register_Debug_Server_Callback  (RSP_CALLBACK_IDX_TYPE index, VOID * pCallback, VOID * p_context);
RSP_STATUS      RSP_Process_Packet                  (CHAR *    p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size);
RSP_STATUS      RSP_Com_Error_Notify(VOID);
RSP_STATUS      RSP_Com_Disconnect_Notify(VOID);
RSP_STATUS      RSP_Send_Notification               (UINT  notifId, UINT notifSignal, \
                                                     CHAR * p_rsp_resp_buff, UINT * rsp_resp_size, \
                                                     DBG_THREAD_ID thread_id);
UINT            RSP_Check_Packet(CHAR *             p_data,
                                 UINT               data_size,
                                 VOID *             p_rsp_pkt,
                                 UINT *             p_rsp_pkt_size,
                                 UINT *             p_data_processed_size);
RSP_STATUS      RSP_Application_State_Begin(VOID);
RSP_STATUS      RSP_Application_State_End(VOID);

#endif /* ifndef RSP_EXTR_H */
