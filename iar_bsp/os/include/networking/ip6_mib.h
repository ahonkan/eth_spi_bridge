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
*        ip6_mib.h                                   
*
*   DESCRIPTION
*
*        This file contains the data structures and macros that maintains
*        the IPv6 statistics for SNMP.
*
*   DATA STRUCTURES
*
*        None.
*
*   DEPENDENCIES
*
*        ip6_mib_err.h
*        ip6_mib_if.h
*        ip6_mib_addr_pre.h
*        ip6_mib_if_addr.h
*        ip6_mib_ntm.h
*        ip6_mib_tcp.h
*        ip6_mib_udp.h
*        ip6_mib_icmp.h
*        ip6_mib_rt.h
*        ip6_mib_mld.h
*
*************************************************************************/

#ifndef IP6_MIB_H
#define IP6_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/ip6_mib_err.h"
#include "networking/ip6_mib_if.h"
#include "networking/ip6_mib_addr_pre.h"
#include "networking/ip6_mib_if_addr.h"
#include "networking/ip6_mib_ntm.h"
#include "networking/ip6_mib_tcp.h"
#include "networking/ip6_mib_udp.h"
#include "networking/ip6_mib_icmp.h"
#include "networking/ip6_mib_rt.h"
#include "networking/ip6_mib_mld.h"

#if (INCLUDE_SNMP == NU_TRUE)

STATUS IP6_MIB_Init(DV_DEVICE_ENTRY *dev);
STATUS IP6_MIB_Initialize(VOID);

#endif

#define IP6_MIB_FORWARDING_SET(value, status)   \
    if (MIB2_ipForwarding_Set((value)) != NU_SUCCESS) \
        (status) = SNMP_GENERROR

#define IP6_MIB_FORWARDING_GET(value, status)   \
    (value) = MIB2_ipForwarding_Get

#define IP6_MIB_DEFAULTHOPLIMIT_GET(value, status)  \
    (value) = IP6_Hop_Limit;

#define IP6_MIB_DEFAULTHOPLIMIT_SET(value, status)                      \
    if (NU_Set_Default_Hop_Limit(((UINT8)(value))) != NU_SUCCESS)   \
        status = SNMP_GENERROR;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_H */

