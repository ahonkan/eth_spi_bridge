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
*        ip6_mib_rt.h                                
*
*   DESCRIPTION
*
*        This file is responsible of managing route table MIBs.
*
*   DATA STRUCTURES
*
*        None.
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_RT_H
#define IP6_MIB_RT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* _cplusplus */

#if (INCLUDE_IPV6_RT == NU_TRUE)

/* Macro(s) defining value for 'ipv6RouteType'. */
#define IP6_MIB_RT_TYPE_OTHER       1
#define IP6_MIB_RT_TYPE_DISCARD     2
#define IP6_MIB_RT_TYPE_LOCAL       3
#define IP6_MIB_RT_TYPE_REMOTE      4

/* Macro(s) defining value for 'ipv6RouteProtocol'. */
#define IP6_MIB_RT_PROT_OTHER       1
#define IP6_MIB_RT_PROT_LOCAL       2
#define IP6_MIB_RT_PROT_NETMGMT     3
#define IP6_MIB_RT_PROT_NDISC       4
#define IP6_MIB_RT_PROT_RIP         5
#define IP6_MIB_RT_PROT_OSPF        6
#define IP6_MIB_RT_PROT_BGP         7
#define IP6_MIB_RT_PROT_IDRP        8
#define IP6_MIB_RT_PROT_IGRP        9

UINT16 IP6_MIB_RT_Get_Number(UINT32 *rt_entry_count);

UINT16 IP6_MIB_RT_Get_Next(UINT8 *addr, UINT32 *pfx_len,
                           UINT32 *route_index);

UINT16 IP6_MIB_RT_Get_If_Index(UINT8 *addr, UINT32 pfx_len,
                                  UINT32 rt_index, UINT32 *if_index);

UINT16 IP6_MIB_RT_Get_Next_Hop(UINT8 *addr, UINT32 pfx_len,
                               UINT32 rt_index, UINT8 *next_hop);

UINT16 IP6_MIB_RT_Get_Type(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                           UINT32 *type);

UINT16 IP6_MIB_RT_Get_Protocol(UINT8 *addr, UINT32 pfx_len,
                                  UINT32 rt_index, UINT32 *protocol);

UINT16 IP6_MIB_RT_Get_Policy(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                             UINT32 *policy);

UINT16 IP6_MIB_RT_Get_Age(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                          UINT32 *rt_age);

UINT16 IP6_MIB_RT_Get_Next_Hop_RDI(UINT8 *addr, UINT32 pfx_len, 
                                   UINT32 rt_index, UINT32 *next_hop_rdi);

UINT16 IP6_MIB_RT_Get_Metric(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                             UINT32 *metric);

UINT16 IP6_MIB_RT_Get_Weight(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                             UINT32 *weight);

UINT16 IP6_MIB_RT_Get_Info(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                           UINT32 *oid, UINT32 *oid_len);

UINT16 IP6_MIB_RT_Get_Valid(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                            UINT32 *rt_valid);

UINT16 IP6_MIB_RT_Set_Valid(UINT8 *addr, UINT32 pfx_len,
                            UINT32 rt_index, UINT32 rt_valid);

#define IP6_MIB_RT_GET_NUMBER(rt_entry_count, status)                   \
    (status) = IP6_MIB_RT_Get_Number(&(rt_entry_count))

#define IP6_MIB_RT_GET_NEXT(addr, pfx_len, route_index, status)         \
    (status) = IP6_MIB_RT_Get_Next((addr), &(pfx_len), &(route_index))

#define IP6_MIB_RT_GET_IF_INDEX(addr, pfx_len, rt_index, if_index,      \
                                status)                                 \
    (status) = IP6_MIB_RT_Get_If_Index((addr), (pfx_len), (rt_index),   \
                                       &(if_index))

#define IP6_MIB_RT_GET_NEXT_HOP(addr, pfx_len, rt_index, next_hop,      \
                                status)                                 \
    (status) = IP6_MIB_RT_Get_Next_Hop((addr), (pfx_len), (rt_index),   \
                                     (next_hop))

#define IP6_MIB_RT_GET_TYPE(addr, pfx_len, rt_index, type, status)      \
    (status) = IP6_MIB_RT_Get_Type((addr), (pfx_len), (rt_index), &(type))

#define IP6_MIB_RT_GET_PROTOCOL(addr, pfx_len, rt_index, protocol,      \
                                status)                                 \
    (status) = IP6_MIB_RT_Get_Protocol((addr), (pfx_len), (rt_index),   \
                                       &(protocol))

#define IP6_MIB_RT_GET_POLICY(addr, pfx_len, rt_index, policy, status)  \
    (status) = IP6_MIB_RT_Get_Policy((addr), (pfx_len), (rt_index),     \
                                     &(policy))

#define IP6_MIB_RT_GET_AGE(addr, pfx_len, rt_index, rt_age, status)     \
    (status) = IP6_MIB_RT_Get_Age((addr), (pfx_len), (rt_index),        \
                                  &(rt_age))

#define IP6_MIB_RT_GET_NEXT_HOP_RDI(addr, pfx_len, rt_index,            \
                                    next_hop_rdi, status)               \
    (status) = IP6_MIB_RT_Get_Next_Hop_RDI((addr), (pfx_len),           \
                                        (rt_index), &(next_hop_rdi))

#define IP6_MIB_RT_GET_METRIC(addr, pfx_len, rt_index, metric, status)  \
    (status) = IP6_MIB_RT_Get_Metric((addr), (pfx_len), (rt_index),     \
                                     &(metric))

#define IP6_MIB_RT_GET_WEIGHT(addr, pfx_len, rt_index, weight, status)  \
    (status) = IP6_MIB_RT_Get_Weight((addr), (pfx_len), (rt_index),     \
                                     &(weight))

#define IP6_MIB_RT_GET_INFO(addr, pfx_len, rt_index, oid, oid_len,      \
                            status)                                     \
    (status) = IP6_MIB_RT_Get_Info((addr), (pfx_len), (rt_index),       \
                                   (oid), &(oid_len))

#define IP6_MIB_RT_GET_VALID(addr, pfx_len, rt_index, rt_valid, status) \
    (status) = IP6_MIB_RT_Get_Valid((addr), (pfx_len), (rt_index),      \
                                    &(rt_valid))

#define IP6_MIB_RT_SET_VALID(addr, pfx_len, rt_index, rt_valid, status) \
    (status) = IP6_MIB_RT_Set_Valid((addr), (pfx_len), (rt_index),      \
                                    (rt_valid))

#endif /* (INCLUDE_IPV6_RT == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_RT_H */
