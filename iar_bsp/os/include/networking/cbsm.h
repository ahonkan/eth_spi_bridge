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
*       cbsm.h                                                   
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used with CBSM v1 and CBSM v2.
*
*   DATA STRUCTURES
*
*       CBSM_COMMUNITY_STRUCT
*       CBSM_COMMUNITY_TABLE
*       SNMP_CBSM_COMMUNITY_STRUCT
*
*   DEPENDENCIES
*
*       snmp.h
*       snmp_dis.h
*
************************************************************************/

#ifndef CBSM_H
#define CBSM_H

#include "networking/snmp.h"
#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

typedef struct cbsm_community_struct
{
    struct   cbsm_community_struct   *next;
    struct   cbsm_community_struct   *previous;

    UINT32   cbsm_transport_tag_len;
    UINT8    cbsm_community_index[SNMP_SIZE_SMALLOBJECTID];
    UINT8    cbsm_community_name[SNMP_SIZE_SMALLOBJECTID];
    UINT8    cbsm_security_name[SNMP_SIZE_SMALLOBJECTID];
    UINT8    cbsm_engine_id[SNMP_SIZE_SMALLOBJECTID];
    UINT8    cbsm_context_name[SNMP_SIZE_SMALLOBJECTID];
    UINT8    cbsm_transport_tag[SNMP_SIZE_SMALLOBJECTID];
    UINT8    cbsm_community_index_len;
    UINT8    cbsm_engine_id_len;
#if (INCLUDE_MIB_CBSM == NU_TRUE)
    UINT8    cbsm_storage_type;
    UINT8    cbsm_status;
    UINT8    cbsm_row_flag;
    UINT8    cbsm_pad[3];
#else
    UINT8    cbsm_pad[2];
#endif

}CBSM_COMMUNITY_STRUCT;

typedef struct cbsm_community_table
{
    struct   cbsm_community_struct   *next;
    struct   cbsm_community_struct   *previous;

}CBSM_COMMUNITY_TABLE;

typedef struct snmp_cbsm_community_struct
{
    CHAR    community_name[SNMP_SIZE_SMALLOBJECTID];
    CHAR    security_name[SNMP_SIZE_SMALLOBJECTID];
    CHAR    transport_tag[SNMP_SIZE_SMALLOBJECTID];
    CHAR    context_name[SNMP_SIZE_SMALLOBJECTID];

}SNMP_CBSM_COMMUNITY_STRUCT;

STATUS CBSM_Verify(UINT32 mms, UINT8 *security_param, UINT8 **whole_msg,
                   UINT32 *msg_len, UINT8 *security_engine_id,
                   UINT32 *security_engine_id_len, UINT8 *security_name,
                   UINT32 *max_size_response_pdu,
                   SNMP_ERROR_STRUCT *error_indication);

STATUS CBSM_Get_Community_Name(UINT8 *security_name, UINT8 *community_name);
CBSM_COMMUNITY_STRUCT *CBSM_Find_Community_Index(
             const UINT8 *community_index, UINT8 getflag, UINT16 *status);
STATUS CBSM_Add_Community(CBSM_COMMUNITY_STRUCT *node);
STATUS CBSM_Remove_Community(CBSM_COMMUNITY_STRUCT *community);
INT32  CBSM_Compare_Index(VOID *left_side, VOID *right_side);
STATUS CBSM_Save_Community (CBSM_COMMUNITY_STRUCT *community);
STATUS CBSM_Config(VOID);
STATUS CBSM_Init(VOID);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* CBSM_H */


