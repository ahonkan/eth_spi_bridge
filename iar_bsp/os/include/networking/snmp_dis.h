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
*       snmp_dis.h                                               
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations for the Dispatcher.
*
*   DATA STRUCTURES
*
*       SNMP_MPD_MIB_STRUCT
*       SNMP_MESSAGE_STRUCT
*       SNMP_ERROR_STRUCT
*       SNMP_SESSION_STRUCT
*
*   DEPENDENCIES
*
*       snmp.h
*       agent.h
*
************************************************************************/

#ifndef SNMP_DIS_H
#define SNMP_DIS_H

#include "networking/snmp.h"
#include "networking/agent.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* DATA STRUCTURES */
typedef struct snmp_mpd_mib_struct
{
    UINT32             snmp_unknown_sm;
    UINT32             snmp_invalid_msg;
    UINT32             snmp_unknown_pduhandlers;
}SNMP_MPD_MIB_STRUCT;

typedef struct snmp_message_struct
{
    struct addr_struct snmp_transport_address;
    UINT8              *snmp_buffer;
    UINT32             snmp_buffer_len;
    UINT8              snmp_transport_domain;
    UINT8              snmp_pad[3];
}SNMP_MESSAGE_STRUCT;

typedef struct snmp_error_struct
{
    UINT32             snmp_oid[SNMP_SIZE_OBJECTID];
    UINT32             snmp_oid_len;
    UINT32             snmp_value;
}SNMP_ERROR_STRUCT;

typedef struct snmp_session_struct
{
    UINT32                snmp_mp;
    UINT32                snmp_sm;
    UINT32                snmp_context_engine_id_len;
    UINT32                snmp_pdu_version;
    UINT32                snmp_object_list_len;
    UINT32                snmp_maxsize_response_pdu;
    snmp_object_t         snmp_object_list[AGENT_LIST_SIZE];
    asn1_sck_t            snmp_err_asn1;
    SNMP_ERROR_STRUCT     snmp_status_info;
    snmp_pdu_t            snmp_pdu;
    SNMP_MESSAGE_STRUCT   *snmp_state_ref;
    VOID                  *snmp_security_state_ref;
    UINT8                 snmp_security_name[SNMP_SIZE_BUFCHR];
    UINT8                 snmp_context_engine_id[SNMP_SIZE_SMALLOBJECTID];
    UINT8                 snmp_context_name[SNMP_SIZE_BUFCHR];
    UINT8                 snmp_in_use;
    UINT8                 snmp_security_level;

    /* Make the structure word-aligned. */
    UINT8                 snmp_pad[2];
}SNMP_SESSION_STRUCT;

/* Functions */
STATUS SNMP_Send_Response(SNMP_MESSAGE_STRUCT *snmp_response,
                          SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_Determine_Ver(const SNMP_MESSAGE_STRUCT *snmp_request,
                          UINT32 *mp_model);
STATUS SNMP_Execute_Request(SNMP_MESSAGE_STRUCT *snmp_request);
STATUS SNMP_Send_Notification(SNMP_MESSAGE_STRUCT *snmp_notification,
                              SNMP_SESSION_STRUCT *snmp_session,
                              INT socket);
#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_DIS_H */

