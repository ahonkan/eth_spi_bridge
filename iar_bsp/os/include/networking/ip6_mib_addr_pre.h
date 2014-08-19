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
*        ip6_mib_addr_pre.h                          
*
*   DESCRIPTION
*
*        This file contains the implementation of IPv6 Prefix Address MIB.
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

#ifndef IP6_MIB_ADDR_PRE_H
#define IP6_MIB_ADDR_PRE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE)

/* Macro defining maximum prefix length. */
#define IP6_MIB_ADDR_MAX_PRE_LEN    128

/* Macro for finding the minimum value. */
#define IP6_MIB_ADDR_MIN(a,b) (((a) < (b)) ? (a) : (b))

UINT16 IP6_MIB_ADDR_PRE_Get_Next(UINT32 *if_index, UINT8 *addr_prefix,
                                    UINT32 *prefix_len);
UINT16 IP6_MIB_ADDR_PRE_Get_OnLinkFlag(UINT32 if_index,
             UINT8 *addr_prefix, UINT32 prefix_len, UINT32 *on_link_flag);
UINT16 IP6_MIB_ADDR_PRE_Get_AutmosFlag(UINT32 if_index,
          UINT8 *addr_prefix, UINT32 prefix_len, UINT32 *autonomous_flag);
UINT16 IP6_MIB_ADDR_PRE_Get_PreferLife(UINT32 if_index,
       UINT8 *addr_prefix, UINT32 prefix_len, UINT32 *preferred_life_tim);
UINT16 IP6_MIB_ADDR_PRE_Get_ValidLife(UINT32 if_index,
      UINT8 *addr_prefix, UINT32 prefix_len, UINT32 *valid_adv_life_time);

#define IP6_MIB_ADDR_PRE_GET_NEXT(if_index, prefix_addr, prefix_len,    \
                                  status)                               \
    (status) = IP6_MIB_ADDR_PRE_Get_Next(&(if_index), (prefix_addr),    \
                                         &(prefix_len))

#define IP6_MIB_ADDR_PRE_GET_ONLINKFLAG(if_index, prefix_addr,          \
                                    prefix_len, onlink_flag, status)    \
    (status) = IP6_MIB_ADDR_PRE_Get_OnLinkFlag((if_index),              \
                           (prefix_addr), (prefix_len), (&(onlink_flag)))

#define IP6_MIB_ADDR_PRE_GET_AUTMOSFLAG(if_index, prefix_addr,          \
                                prefix_len, autonomous_flag, status)    \
    (status) = IP6_MIB_ADDR_PRE_Get_AutmosFlag((if_index),              \
                      (prefix_addr), (prefix_len), (&(autonomous_flag)))

#define IP6_MIB_ADDR_PRE_GET_PREFERLIFE(if_index, prefix_addr,          \
                                prefix_len, prefer_life_time, status)   \
    (status) = IP6_MIB_ADDR_PRE_Get_PreferLife((if_index),              \
                    (prefix_addr), (prefix_len), (&(prefer_life_time)))

#define IP6_MIB_ADDR_PRE_GET_VALIDLIFE(if_index, prefix_addr,           \
                                   prefix_len, valid_life_time, status) \
    (status) = IP6_MIB_ADDR_PRE_Get_ValidLife((if_index),               \
                    (prefix_addr), (prefix_len), (&(valid_life_time)))

#endif /* (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_ADDR_PRE_H */
