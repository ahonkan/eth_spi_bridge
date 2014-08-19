/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_v3.h                                                
*
*   DESCRIPTION
*
*       This file contains definitions required by the SNMP Version 3
*       Message Processing Model.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       snmp_dis.h
*
************************************************************************/
#ifndef SNMP_V3_H
#define SNMP_V3_H

#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define SNMP_SECURITY_LEVEL_MASK        3

#define SNMP_REPORTABLE_FLAG_1          0x04        /* 0000 0100 */
#define SNMP_REPORTABLE_FLAG_0          0x03        /* 0000 0011 */


STATUS SNMP_V3_Init(VOID);
STATUS SNMP_V3_Dec_Request(SNMP_MESSAGE_STRUCT *snmp_request,
                           SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V3_Dec(SNMP_MESSAGE_STRUCT* snmp_request,
                   SNMP_SESSION_STRUCT* snmp_session);
STATUS SNMP_Msg_V3_Dec(asn1_sck_t* asn1, UINT32* snmp_sm,
                       UINT8* snmp_security_name,
                       UINT8* snmp_security_level, UINT8* snmp_engine_id,
                       UINT8* snmp_context_name,
                       snmp_object_t* snmp_object_list,
                       UINT32* snmp_object_list_len, snmp_pdu_t* snmp_pdu,
                       UINT32* buff_len,
                       UINT32* snmp_context_engine_id_len,
                       UINT32* snmp_max_size_response_pdu,
                       VOID** snmp_security_state_ref,
                       SNMP_ERROR_STRUCT*  error_indication);

STATUS SNMP_Scoped_Pdu_Dec(asn1_sck_t* asn1, UINT8* snmp_engine_id,
                           UINT8* snmp_context_name,
                           snmp_object_t* snmp_object_list,
                           UINT32* snmp_object_list_len,
                           snmp_pdu_t* snmp_pdu,
                           UINT32* snmp_engine_id_len);

STATUS SNMP_V3_Enc_Respond(SNMP_MESSAGE_STRUCT *snmp_response,
                           SNMP_SESSION_STRUCT *snmp_session);

STATUS SNMP_V3_Encode(SNMP_MESSAGE_STRUCT * snmp_response,
                       SNMP_SESSION_STRUCT* snmp_session);

STATUS SNMP_V3_Message_Encode(asn1_sck_t* asn1, snmp_pdu_t* snmp_pdu,
                              snmp_object_t* snmp_object_list,
                              UINT32 snmp_object_list_len,
                              const UINT8* snmp_engine_id,
                              UINT32 snmp_engine_id_len,
                              const UINT8* snmp_context_name,
                              UINT32 *snmp_sm, UINT32 snmp_mp,
                              UINT8* snmp_security_name,
                              UINT8 snmp_security_level,
                              VOID* security_state_ref,
                              UINT8** scoped_pdu, UINT32 msgID);

BOOLEAN SNMP_V3_Extract_MsgId(UINT8* snmp_buffer, UINT32 snmp_buffer_size,
                              UINT32* msg_id);
STATUS  SNMP_V3_Notification_Enc(SNMP_MESSAGE_STRUCT *snmp_notification,
                                 SNMP_SESSION_STRUCT* snmp_session);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

