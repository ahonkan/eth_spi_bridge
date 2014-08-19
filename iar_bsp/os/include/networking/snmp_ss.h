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
*       snmp_ss.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used in the Security Subsystem.
*
*   DATA STRUCTURES
*
*       SNMP_SS_STRUCT
*
*   DEPENDENCIES
*
*       snmp.h
*       snmp_dis.h
*       snmp_cfg.h
*
************************************************************************/
#ifndef SNMP_SS_H
#define SNMP_SS_H

#include "networking/snmp.h"
#include "networking/snmp_dis.h"
#include "networking/snmp_cfg.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

typedef STATUS  (*SNMP_SECURE_OUTGOING_STRUCT)(UINT32, UINT8 **, UINT32 *,
                                               UINT32, UINT32, UINT8 *,
                                               UINT32, UINT8 *, UINT8,
                                               UINT8 *, VOID *);

typedef STATUS (*SNMP_VERIFY_INCOMING_STRUCT)(UINT32, UINT32, UINT8 *,
                                              UINT32, UINT8 *, UINT8 **,
                                              UINT32 *, UINT8 *, UINT32 *,
                                              UINT8 *, UINT32 *, VOID **,
                                              SNMP_ERROR_STRUCT *);


/* Structure used for calling functions for a particular
 * Security Model.
 */
typedef struct snmp_ss_struct
{
    UINT32                         model;
    SNMP_INIT                      snmp_init_cb;
    SNMP_CONFIG                    snmp_config_cb;
    SNMP_SECURE_OUTGOING_STRUCT    process_outgoing_cb;
    SNMP_VERIFY_INCOMING_STRUCT    process_incoming_cb;
} SNMP_SS_STRUCT;

STATUS SNMP_Ss_Init(VOID);
STATUS SNMP_Ss_Config(VOID);

STATUS SNMP_Secure(UINT32 snmp_mp, UINT8 **whole_message, UINT32 *msg_len,
                   UINT32 max_message_size, UINT32 snmp_sm,
                   UINT8 *security_engine_id,
                   UINT32 security_engine_id_len, UINT8 *security_name,
                   UINT8 security_level, UINT8 *scoped_pdu,
                   VOID *security_state_ref);

STATUS SNMP_Verify(UINT32 snmp_mp, UINT32 max_message_size,
                   UINT8 *security_param, UINT32 snmp_sm,
                   UINT8 *security_level, UINT8 **whole_message,
                   UINT32 *msg_len, UINT8 *security_engine_id,
                   UINT32 *security_engine_id_len, UINT8 *security_name,
                   UINT32 *max_response_pdu, VOID **security_state_ref,
                   SNMP_ERROR_STRUCT *error_indication);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_SS_H */

