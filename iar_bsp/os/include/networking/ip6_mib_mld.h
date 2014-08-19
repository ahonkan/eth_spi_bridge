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
*        ip6_mib_mld.h                               
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that handles
*        request on MLD Group MIBs.
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

#ifndef IP6_MIB_MLD_H
#define IP6_MIB_MLD_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_MLD_MIB == NU_TRUE) && \
      (INCLUDE_IP_MULTICASTING == NU_TRUE) )

/* Macro defining active row. */
#define IP6_MIB_MLD_ROW_ACTIVE      1

UINT16 IP6_MLD_MIB_Get_Next_Index(UINT32 *index);
UINT16 IP6_MLD_MIB_Get_If_Status(UINT32 if_index, UINT32 *if_status);
UINT16 IP6_MLD_MIB_Get_If_Qrier_Addr(UINT32 if_index, UINT8 *addr);
UINT16 IP6_MLD_MIB_Get_Next_Cache(UINT8 *addr, UINT32 *if_index);
UINT16 IP6_MLD_MIB_Get_Cache_Self(UINT8 *addr, UINT32 if_index,
                                  UINT32 *is_cache_self);
UINT16 IP6_MLD_MIB_Get_Cache_Status(UINT8 *addr, UINT32 if_index,
                                    UINT32 *cache_status);

#define IP6_MLD_MIB_GET_NEXT_INDEX(if_index, status)    \
    (status) = IP6_MLD_MIB_Get_Next_Index(&(if_index))

#define IP6_MLD_MIB_GET_IF_STATUS(if_index, if_status, status)  \
    (status) = IP6_MLD_MIB_Get_If_Status((if_index), &(if_status))

#define IP6_MLD_MIB_GET_IF_QRIER_ADDR(if_index, addr, status)   \
    (status) = IP6_MLD_MIB_Get_If_Qrier_Addr((if_index), (addr))

#define IP6_MLD_MIB_GET_NEXT_CACHE(addr, if_index, status)  \
    (status) = IP6_MLD_MIB_Get_Next_Cache((addr), &(if_index))

#define IP6_MLD_MIB_GET_CACHE_SELF(addr, if_index, is_cache_self,   \
                                   status)                          \
    (status) = IP6_MLD_MIB_Get_Cache_Self((addr), (if_index),   \
                                          &(is_cache_self))

#define IP6_MLD_MIB_GET_CACHE_STATUS(addr, if_index, cache_status,  \
                                     status)                        \
    (status) = IP6_MLD_MIB_Get_Cache_Status((addr), (if_index),     \
                                            &(cache_status))

#endif /* ((INCLUDE_IPV6_MLD_MIB == NU_TRUE) && \
           (INCLUDE_IP_MULTICASTING == NU_TRUE)) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_MLD_H */
