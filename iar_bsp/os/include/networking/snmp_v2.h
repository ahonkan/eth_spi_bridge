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
*       snmp_v2.h                                                
*
*   DESCRIPTION
*
*       This file contains definitions required by the SNMP Version 2
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
#ifndef SNMP_V2_H
#define SNMP_V2_H

#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

STATUS SNMP_V2_Init(VOID);
STATUS SNMP_V2_Dec_Request(SNMP_MESSAGE_STRUCT *snmp_request,
                           SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V2_Enc_Respond(SNMP_MESSAGE_STRUCT *snmp_response,
                           SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V2_Enc_Error(SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V2_Enc(SNMP_MESSAGE_STRUCT *snmp_response,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V2_Dec(SNMP_MESSAGE_STRUCT *snmp_request,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_V2_Notification_Enc(SNMP_MESSAGE_STRUCT *snmp_notification,
                                SNMP_SESSION_STRUCT* snmp_session);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_V2_H */

