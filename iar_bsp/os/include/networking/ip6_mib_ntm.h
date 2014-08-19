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
*        ip6_mib_ntm.h                               
*
*   DESCRIPTION
*
*        This file contains the function that are responsible of handling
*        requests on IPv6 Net to Media Table.
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
#ifndef IP6_MIB_NTM_H
#define IP6_MIB_NTM_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* _cplusplus */

#if (INCLUDE_IPV6_MIB_NTM == NU_TRUE)

/* Macro(s) defining values for 'ipv6NetToMediaType'. */
#define IP6_MIB_NTM_TYPE_OTHER          1
#define IP6_MIB_NTM_TYPE_DYNAMIC        2
#define IP6_MIB_NTM_TYPE_STATIC         3
#define IP6_MIB_NTM_TYPE_LOCAL          4

/* Macro(s) defining values for 'ipv6IfNetToMediaState'. */
#define IP6_MIB_NTM_STATE_REACHABLE     1
#define IP6_MIB_NTM_STATE_STALE         2
#define IP6_MIB_NTM_STATE_DELAY         3
#define IP6_MIB_NTM_STATE_PROBE         4
#define IP6_MIB_NTM_STATE_INVALID       5
#define IP6_MIB_NTM_STATE_UNKNOWN       6


UINT16 IP6_MIB_NTM_Get_Next_Index(UINT32 *if_index, UINT8 *ipv6_addr);
UINT16 IP6_MIB_NTM_Get(UINT32 if_index, UINT8 *ipv6_addr, VOID *value,
                       INT opt);

#define IP6_MIB_NTM_GET_NEXT_INDEX(if_index, ipv6_addr, status)     \
    (status) = IP6_MIB_NTM_Get_Next_Index(&(if_index), (ipv6_addr))

#define IP6_MIB_NTM_GET(if_index, ipv6_addr, value, option, status)     \
    (status) = IP6_MIB_NTM_Get((if_index), (ipv6_addr), (value), (option))

#endif /* (INCLUDE_IPV6_MIB_NTM == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* IP6_MIB_NTM_H */
