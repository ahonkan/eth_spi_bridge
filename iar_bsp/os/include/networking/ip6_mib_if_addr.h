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
*        ip6_mib_if_addr.h                           
*
*   DESCRIPTION
*
*        This file contains the implementation of IPv6 Interface address
*        MIB.
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

#ifndef IP6_MIB_IF_ADDR_H
#define IP6_MIB_IF_ADDR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE)

/* Macro(s) for address type values. */
#define IP6_MIB_IF_ADDR_TYPE_UNKNOWN        3

/* Macro(s) for address status values. */
#define IP6_MIB_IF_ADDR_STATUS_PREFER       1
#define IP6_MIB_IF_ADDR_STATUS_DEPREC       2
#define IP6_MIB_IF_ADDR_STATUS_INVALID      3
#define IP6_MIB_IF_ADDR_STATUS_INACCES      4
    
UINT16 IP6_IF_ADDR_MIB_Get_Next(UINT32 *if_index, UINT8 *addr);
UINT16 IP6_IF_ADDR_MIB_Get_Pfx_Length(UINT32 if_index, UINT8 *addr,
                                     UINT32 *pfx_len);
UINT16 IP6_IF_ADDR_MIB_Get_Type(UINT32 if_index, UINT8 *addr, UINT32 *type);
UINT16 IP6_IF_ADDR_MIB_Get_AnycastFlag(UINT32 if_index, UINT8 *addr,
                                      UINT32 *anycast_flag);
UINT16 IP6_IF_ADDR_MIB_Get_Status(UINT32 if_index, UINT8 *addr,
                                 UINT32 *if_addr_status);

#define IP6_IF_ADDR_MIB_GET_NEXT(if_index, addr, status)    \
    (status) = IP6_IF_ADDR_MIB_Get_Next(&(if_index), (addr))

#define IP6_IF_ADDR_MIB_GET_PFX_LENGTH(if_index, addr, prefix_len,      \
                                       status)                          \
    (status) = IP6_IF_ADDR_MIB_Get_Pfx_Length((if_index), (addr),       \
                                              &(prefix_len))

#define IP6_IF_ADDR_MIB_GET_TYPE(if_index, addr, type, status)  \
    (status) = IP6_IF_ADDR_MIB_Get_Type((if_index), (addr), &(type))

#define IP6_IF_ADDR_MIB_GET_ANYCASTFLAG(if_index, addr, anycast_flag,   \
                                        status)                         \
    (status) = IP6_IF_ADDR_MIB_Get_AnycastFlag((if_index), (addr),      \
                                               &(anycast_flag))

#define IP6_IF_ADDR_MIB_GET_STATUS(if_index, addr, if_addr_status,      \
                                   ret_status)                          \
    (ret_status) = IP6_IF_ADDR_MIB_Get_Status((if_index), (addr),       \
                                              &(if_addr_status))

#endif /* (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_IF_ADDR_H */
