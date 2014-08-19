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
*       cbsm_v1.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used in the Community-based Security Model Version 1.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       snmp.h
*       snmp_dis.h
*       cbsm.h
*
*************************************************************************/

#ifndef CBSM_V1_H
#define CBSM_V1_H

#include "networking/snmp.h"
#include "networking/snmp_dis.h"
#include "networking/cbsm.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

STATUS CBSM_V1_Verify(UINT32 snmp_mp, UINT32 mms, UINT8 *security_param,
                      UINT32 snmp_sm, UINT8 *security_level,
                      UINT8 **whole_msg, UINT32 *msg_len,
                      UINT8 *security_engine_id,
                      UINT32 *security_engine_id_len,
                      UINT8 *security_name, UINT32 *max_size_response_pdu,
                      VOID **security_state_ref,
                      SNMP_ERROR_STRUCT *error_indication);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* CBSM_V1_H */

