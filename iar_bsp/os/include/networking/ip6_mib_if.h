/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*        ip6_mib_if.h                                
*
*   DESCRIPTION
*
*        This file contains the implementation of interface group for IPv6
*        MIB.
*
*   DATA STRUCTURES
*
*        IP6_MIB_INTERFACE
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_IF_H
#define IP6_MIB_IF_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

extern UINT8        IP6_Hop_Limit;

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)
extern UINT32       IP6_MIB_If_Table_Last_Change;

/* Sub OID Length of lower layer. */
#define IP6_IF_MIB_LOW_LYR_SUB_OID_LEN      10

/* size of ipv6IfDescr. */
#define IP6_IF_DESC_SIZE                    256

typedef struct ip6_mib_interface
{
    CHAR            ip6_if_desc[IP6_IF_DESC_SIZE];
    UINT32          ip6_stats[20];
}IP6_MIB_INTERFACE;

#endif

/* Following macros will be used as index for ip6_stats. */
#define IP6_IF_STATS_IN_RCV            0
#define IP6_IF_STATS_IN_HDR_ERR        1
#define IP6_IF_STATS_IN_TOO_BIG_ERR    2
#define IP6_IF_STATS_IN_NO_ROUTES      3
#define IP6_IF_STATS_IN_ADDR_ERRORS    4
#define IP6_IF_STATS_IN_UNKNOWNPROTOS  5
#define IP6_IF_STATS_IN_TRUNCATED_PKTS 6
#define IP6_IF_STATS_IN_DISCARDS       7
#define IP6_IF_STATS_IN_DELIVERS       8
#define IP6_IF_STATS_OUT_FORW_DATAGRAM 9
#define IP6_IF_STATS_OUT_REQUESTS      10
#define IP6_IF_STATS_OUT_DISCARDS      11
#define IP6_IF_STATS_OUT_FRAGOKS       12
#define IP6_IF_STATS_OUT_FRAGFAILS     13
#define IP6_IF_STATS_OUT_FRAG_CREATES  14
#define IP6_IF_STATS_REASM_REQDS       15
#define IP6_IF_STATS_REASM_OKS         16
#define IP6_IF_STATS_REASM_FAILS       17
#define IP6_IF_STATS_IN_MCAST_PKTS     18
#define IP6_IF_STATS_OUT_MCAST_PKTS    19

#if ((INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) || \
     (INCLUDE_IPV6_ICMP_MIB == NU_TRUE))

UINT16 IP6_IF_MIB_Get_Next_Index(UINT32 *if_index);

#endif

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)
UINT16 IP6_IF_MIB_Get_Number(UINT32 *number);
UINT16 IP6_IF_MIB_Get_Index(UINT32 if_index);
UINT16 IP6_IF_MIB_Get_Descr(UINT32 if_index, CHAR *if_descr);
UINT16 IP6_IF_MIB_Set_Descr(UINT32 if_index, CHAR *if_descr);

#if (INCLUDE_IF_STACK == NU_TRUE)

UINT16 IP6_IF_MIB_Get_Lower_Layer(UINT32 if_index, UINT32 *object_oid,
                                  UINT32 *oid_len);

#endif /* (INCLUDE_IF_STACK == NU_TRUE) */

UINT16 IP6_IF_MIB_Get_Effective_MTU(UINT32 if_index, UINT32 *effective_mtu);
UINT16 IP6_IF_MIB_Get_Reasm_Max_Size(UINT32 if_index, UINT32 *reasm_max_size);
UINT16 IP6_IF_MIB_Get_Identifier(UINT32 if_index, UINT8 *if_identifier);
UINT16 IP6_IF_MIB_Set_Identifier(UINT32 if_index, UINT8 *if_identifier);
UINT16 IP6_IF_MIB_Get_Identifier_Len(UINT32 if_index, UINT32 *id_len);
UINT16 IP6_IF_MIB_Set_Identifier_Len(UINT32 if_index, UINT32 id_len);
UINT16 IP6_IF_MIB_Get_Status(UINT32 if_index, INT opt, UINT32 *counter);

#define IP6_IF_MIB_GET_NUMBER(num, status)  \
    (status) = IP6_IF_MIB_Get_Number(&(num))

#define IP6_IF_MIB_GET_NEXT_INDEX(if_index, status) \
    (status) = IP6_IF_MIB_Get_Next_Index(&(if_index))

#define IP6_IF_MIB_GET_INDEX(if_index, status)  \
    (status) = IP6_IF_MIB_Get_Index((if_index))

#define IP6_IF_MIB_GET_DESCR(if_index, if_descr, status)  \
    (status) = IP6_IF_MIB_Get_Descr((if_index), (CHAR *)(&(if_descr)[0]))

#define IP6_IF_MIB_SET_DESCR(if_index, if_descr, status)    \
    (status) = IP6_IF_MIB_Set_Descr((if_index), (CHAR *)(&(if_descr)[0]))

#define MIB_ipv6IfDescr_Set(device, if_descr)                           \
    if (strlen((if_descr)) < IP6_IF_DESC_SIZE)                          \
    {                                                                   \
        strcpy((CHAR *)((device) -> ip6_interface_mib->ip6_if_desc),    \
                (if_descr));                                            \
    }

#if (INCLUDE_IF_STACK == NU_TRUE)

#define IP6_IF_MIB_GET_LOWER_LAYER(if_index, lower_layer_oid,            \
                                   low_oid_len, status)                  \
    (status) = IP6_IF_MIB_Get_Lower_Layer((if_index), (lower_layer_oid), \
                                          &(low_oid_len))

#else

#define IP6_IF_MIB_GET_LOWER_LAYER(if_index, lower_layer_oid,           \
                                   low_oid_len, status)                 \
    status = IP6_MIB_NOSUCHNAME

#endif /* (INCLUDE_IF_STACK == NU_TRUE) */

#define IP6_IF_MIB_GET_EFFECTIVE_MTU(if_index, effective_mtu, status)   \
    (status) = IP6_IF_MIB_Get_Effective_MTU((if_index), &(effective_mtu))

#define IP6_IF_MIB_GET_REASM_MAX_SIZE(if_index, reasm_max_size, status) \
    (status) = IP6_IF_MIB_Get_Reasm_Max_Size((if_index),                \
                                             &(reasm_max_size))

#define IP6_IF_MIB_GET_IDENTIFIER(if_index, if_identifier, status)      \
    (status) = IP6_IF_MIB_Get_Identifier((if_index), (if_identifier))

#define IP6_IF_MIB_SET_IDENTIFIER(if_index, if_identifier, status)      \
    (status) = IP6_IF_MIB_Set_Identifier((if_index), (if_identifier))

#define IP6_IF_MIB_GET_IDENTIFIER_LEN(if_index, id_len, status)         \
    (status) = IP6_IF_MIB_Get_Identifier_Len((if_index), &(id_len))

#define IP6_IF_MIB_SET_IDENTIFIER_LEN(if_index, id_len, status)         \
    (status) = IP6_IF_MIB_Set_Identifier_Len((if_index), (id_len))

#define IP6_MIB_IF_TABLE_LAST_CHNG_GET(value, status)                   \
    (value) = IP6_MIB_If_Table_Last_Change;                             \
    (status) = IP6_MIB_SUCCESS

#define IP6_MIB_IF_TABLE_LAST_CHNG_SET(value)                           \
    IP6_MIB_If_Table_Last_Change = (value)

#define IP6_MIB_IF_PHY_ADDR_GET(if_index, phy_addr, length, status)      \
    if (MIB2_ifAddr_Get(((if_index) - 1), (phy_addr), (length)) !=       \
                                                            NU_SUCCESS)  \
        (status) = IP6_MIB_NOSUCHOBJECT

#define IP6_MIB_IF_ADMIN_STATUS_GET(if_index, state, status)             \
    if (MIB2_ifStatusAdmin_Get(((if_index) - 1), (state)) != NU_SUCCESS) \
        (status) = IP6_MIB_NOSUCHOBJECT

#define IP6_MIB_IF_ADMIN_STATUS_SET(if_index, value, status)             \
    if (MIB2_ifStatusAdmin_Set(((if_index) - 1), (value)) != NU_SUCCESS) \
        (status) = IP6_MIB_NOSUCHOBJECT

#define IP6_MIB_IF_OPER_STATUS_GET(if_index, state, status)             \
    if (MIB2_ifStatusOper_Get(((if_index) - 1), (state)) != NU_SUCCESS) \
        (status) = IP6_MIB_NOSUCHOBJECT

#define IP6_MIB_IF_LAST_CHANGE_GET(if_index, time, status)              \
    if (MIB2_ifLastChange_Get(((if_index) - 1), (time)) != NU_SUCCESS)  \
        (status) = IP6_MIB_NOSUCHOBJECT

#define IP6_IF_MIB_GET_STATUS(if_index, option, counter, status)        \
    (status) = IP6_IF_MIB_Get_Status((if_index), (option), &(counter))

/* This macro will be used internally in this file to maintain IP6
   statistics. */
#define IP6_MIB_ADD_IF_STAT(device, val, index)                         \
    if ((device) -> ip6_interface_mib)                                  \
    {                                                                   \
        (device) -> ip6_interface_mib -> ip6_stats[index] += (val);     \
    }
#else

#define IP6_MIB_ADD_IF_STAT(device, val, index)
#define MIB_ipv6IfDescr_Set(device, if_descr)

#endif /* (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) */

/* This macro will be used to maintain ipv6IfStatsInReceives. */
#define MIB_ipv6IfStatsInReceives_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_RCV)

/* This macro will be used to increment ipv6IfStatsInReceives. */
#define MIB_ipv6IfStatsInReceives_Inc(device)                           \
    MIB_ipv6IfStatsInReceives_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInHdrErrors. */
#define MIB_ipv6IfStatsInHdrErrors_Add(device, val)                     \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_HDR_ERR)

/* This macro will be used to increment ipv6IfStatsInHdrErrors. */
#define MIB_ipv6IfStatsInHdrErrors_Inc(device)                          \
    MIB_ipv6IfStatsInHdrErrors_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInTooBigErrors. */
#define MIB_ipv6IfStatsInTooBigErr_Add(device, val)                  \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_TOO_BIG_ERR)

/* This macro will be used to increment ipv6IfStatsInTooBigErrors. */
#define MIB_ipv6IfStatsInTooBigErr_Inc(device)                       \
    MIB_ipv6IfStatsInTooBigErr_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInNoRoutes. */
#define MIB_ipv6IfStatsInNoRoutes_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_NO_ROUTES)

/* This macro will be used to increment ipv6IfStatsInNoRoutes. */
#define MIB_ipv6IfStatsInNoRoutes_Inc(device)                           \
    MIB_ipv6IfStatsInNoRoutes_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInAddrErrors. */
#define MIB_ipv6IfStatsInAddrErr_Add(device, val)                    \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_ADDR_ERRORS)

/* This macro will be used to increment ipv6IfStatsInAddrErrors. */
#define MIB_ipv6IfStatsInAddrErr_Inc(device)                         \
    MIB_ipv6IfStatsInAddrErr_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInUnknownProtos. */
#define MIB_ipv6IfStatsInUndefProt_Add(device, val)                 \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_UNKNOWNPROTOS)

/* This macro will be used to increment ipv6IfStatsInUnknownProtos. */
#define MIB_ipv6IfStatsInUndefProt_Inc(device)                      \
    MIB_ipv6IfStatsInUndefProt_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInTruncatedPkts. */
#define MIB_ipv6IfStatsInTruncPkts_Add(device, val)                 \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_TRUNCATED_PKTS)

/* This macro will be used to increment ipv6IfStatsInTruncatedPkts. */
#define MIB_ipv6IfStatsInTruncPkts_Inc(device)                      \
    MIB_ipv6IfStatsInTruncPkts_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInDiscards. */
#define MIB_ipv6IfStatsInDiscards_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_DISCARDS)

/* This macro will be used to increment ipv6IfStatsInDiscards. */
#define MIB_ipv6IfStatsInDiscards_Inc(device)                           \
    MIB_ipv6IfStatsInDiscards_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInDelivers. */
#define MIB_ipv6IfStatsInDelivers_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_DELIVERS)

/* This macro will be used to increment ipv6IfStatsInDelivers. */
#define MIB_ipv6IfStatsInDelivers_Inc(device)                           \
    MIB_ipv6IfStatsInDelivers_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutForwDatagrams. */
#define MIB_ipv6IfStatsOutForDgram_Add(device, val)                \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_FORW_DATAGRAM)

/* This macro will be used to increment ipv6IfStatsOutForwDatagrams. */
#define MIB_ipv6IfStatsOutForDgram_Inc(device)                     \
    MIB_ipv6IfStatsOutForDgram_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutRequests. */
#define MIB_ipv6IfStatsOutRequests_Add(device, val)                     \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_REQUESTS)

/* This macro will be used to increment ipv6IfStatsOutRequests. */
#define MIB_ipv6IfStatsOutRequests_Inc(device)                          \
    MIB_ipv6IfStatsOutRequests_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutDiscards. */
#define MIB_ipv6IfStatsOutDiscards_Add(device, val)                     \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_DISCARDS)

/* This macro will be used to increment ipv6IfStatsOutDiscards. */
#define MIB_ipv6IfStatsOutDiscards_Inc(device)                          \
    MIB_ipv6IfStatsOutDiscards_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutFragOKs. */
#define MIB_ipv6IfStatsOutFragOKs_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_FRAGOKS)

/* This macro will be used to increment ipv6IfStatsOutFragOKs. */
#define MIB_ipv6IfStatsOutFragOKs_Inc(device)                           \
    MIB_ipv6IfStatsOutFragOKs_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutFragFails. */
#define MIB_ipv6IfStatsOutFragFail_Add(device, val)                    \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_FRAGFAILS)

/* This macro will be used to increment ipv6IfStatsOutFragFails. */
#define MIB_ipv6IfStatsOutFragFail_Inc(device)                         \
    MIB_ipv6IfStatsOutFragFail_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutFragCreates. */
#define MIB_ipv6IfStatsOutFragCrt_Add(device, val)                  \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_FRAG_CREATES)

/* This macro will be used to increment ipv6IfStatsOutFragCreates. */
#define MIB_ipv6IfStatsOutFragCrt_Inc(device)                       \
    MIB_ipv6IfStatsOutFragCrt_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsReasmReqds. */
#define MIB_ipv6IfStatsReasmReqds_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_REASM_REQDS)

/* This macro will be used to increment ipv6IfStatsReasmReqds. */
#define MIB_ipv6IfStatsReasmReqds_Inc(device)                           \
    MIB_ipv6IfStatsReasmReqds_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsReasmOKs. */
#define MIB_ipv6IfStatsReasmOKs_Add(device, val)                        \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_REASM_OKS)

/* This macro will be used to increment ipv6IfStatsReasmOKs. */
#define MIB_ipv6IfStatsReasmOKs_Inc(device)                             \
    MIB_ipv6IfStatsReasmOKs_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsReasmFails. */
#define MIB_ipv6IfStatsReasmFails_Add(device, val)                      \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_REASM_FAILS)

/* This macro will be used to increment ipv6IfStatsReasmFails. */
#define MIB_ipv6IfStatsReasmFails_Inc(device)                           \
    MIB_ipv6IfStatsReasmFails_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsInMcastPkts. */
#define MIB_ipv6IfStatsInMcastPkts_Add(device, val)                     \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_IN_MCAST_PKTS)

/* This macro will be used to increment ipv6IfStatsInMcastPkts. */
#define MIB_ipv6IfStatsInMcastPkts_Inc(device)                          \
    MIB_ipv6IfStatsInMcastPkts_Add((device), 1)

/* This macro will be used to maintain ipv6IfStatsOutMcastPkts. */
#define MIB_ipv6IfStatsOutMcastPkt_Add(device, val)                    \
    IP6_MIB_ADD_IF_STAT((device), (val), IP6_IF_STATS_OUT_MCAST_PKTS)

/* This macro will be used to increment ipv6IfStatsOutMcastPkts. */
#define MIB_ipv6IfStatsOutMcastPkt_Inc(device)                         \
    MIB_ipv6IfStatsOutMcastPkt_Add((device), 1)

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_IF_H */
