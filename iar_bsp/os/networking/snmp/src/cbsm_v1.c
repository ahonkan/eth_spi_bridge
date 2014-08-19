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
*       cbsm_v1.c                                                
*
*   DESCRIPTION
*
*       This file contains functions specific to the Community-Based
*       Security Model Version 1.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       CBSM_V1_Verify
*
*   DEPENDENCIES
*
*       snmp.h
*       cbsm_v1.h
*       snmp_cfg.h
*
*************************************************************************/

#include "networking/snmp.h"
#include "networking/cbsm_v1.h"

#if (INCLUDE_SNMPv1 == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       CBSM_V1_Verify
*
*   DESCRIPTION
*
*       This function verifies the incoming message.
*
*   INPUTS
*
*       snmp_mp
*       mms
*       *security_param
*       snmp_sm
*       *security_level
*       **whole_msg
*       *msg_len
*       *security_engine_id
*       *security_engine_id_len
*       *security_name
*       *max_size_response_pdu
*       **security_state_ref
*       *error_indication
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS CBSM_V1_Verify(UINT32 snmp_mp, UINT32 mms, UINT8 *security_param,
                      UINT32 snmp_sm, UINT8 *security_level,
                      UINT8 **whole_msg, UINT32 *msg_len,
                      UINT8 *security_engine_id,
                      UINT32 *security_engine_id_len,
                      UINT8 *security_name, UINT32 *max_size_response_pdu,
                      VOID **security_state_ref,
                      SNMP_ERROR_STRUCT *error_indication)
{
    STATUS  status;

    UNUSED_PARAMETER(snmp_mp);
    UNUSED_PARAMETER(snmp_sm);
    UNUSED_PARAMETER(security_level);
    UNUSED_PARAMETER(security_state_ref);

    /* Call the actual CBSM function. */
    status = CBSM_Verify(mms, security_param, whole_msg, msg_len,
                         security_engine_id, security_engine_id_len,
                         security_name, max_size_response_pdu,
                         error_indication);

    return (status);

} /* CBSM_V1_Verify */

#endif



