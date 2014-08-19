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
*       snmp_v1.h                                                
*
*   DESCRIPTION
*
*       This file contains definitions required by the SNMP Version 1
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

#ifndef SNMP_V1_H
#define SNMP_V1_H

#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

STATUS SNMP_V1_Init(VOID);
STATUS SNMP_V1_Enc_Respond(SNMP_MESSAGE_STRUCT *snmp_reponse,
                           SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V1_Enc_Error(SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V1_Dec_Request(SNMP_MESSAGE_STRUCT *snmp_request,
                           SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V1_ObjEnc(asn1_sck_t *Asn1, const snmp_object_t *Obj);
STATUS SNMP_V1_ObjDec(asn1_sck_t *Asn1, snmp_object_t *Obj);
STATUS SNMP_V1_LstEnc(asn1_sck_t *Asn1, snmp_object_t *Lst,
                       UINT32 LstLen);
STATUS SNMP_V1_LstDec(asn1_sck_t *Asn1, snmp_object_t *Lst,
                       UINT32 LstSze, UINT32 *LstLen);
STATUS SNMP_V1_RqsEnc(asn1_sck_t *Asn1, const snmp_request_t *Rqs);
STATUS SNMP_V1_RqsDec(asn1_sck_t *Asn1, snmp_request_t *Rqs);
STATUS SNMP_V1_TrpEnc(asn1_sck_t *Asn1, const snmp_trap_t *Trp);
STATUS SNMP_V1_PduEnc(asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstLen);
STATUS SNMP_V1_PduDec(asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstSze, UINT32 *LstLen);
STATUS SNMP_V1_Enc(SNMP_MESSAGE_STRUCT *snmp_response,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V1_Dec(SNMP_MESSAGE_STRUCT *snmp_request,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V1_Notification_Enc(SNMP_MESSAGE_STRUCT *snmp_notification,
                                SNMP_SESSION_STRUCT* snmp_session);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_V1_H */

