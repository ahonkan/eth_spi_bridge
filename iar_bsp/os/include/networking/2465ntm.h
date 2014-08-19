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
*        2465ntm.h                                   
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that are
*        responsible for handling SNMP requests on IPv6 NET To Media
*        Table.
*
*   DATA STRUCTURES
*
*        None.
*
*   DEPENDENCIES
*
*        None
*
************************************************************************/

#ifndef IP6_MIB_NTM_S_H
#define IP6_MIB_NTM_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */
    
#if ( (INCLUDE_IPV6_MIB_NTM == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10 having
 * ipv6IfIndex  of length 1 and ipv6NetToMediaNetAddress of length
 * IP6_ADDR_LEN as indexes this results in total sub-length of
 * 11 + IP6_ADDR_LEN.
 */
#define IP6_NTM_SUB_LEN             (11 + IP6_ADDR_LEN)

/* Offset of attribute in ipv6NetToMediaEntry. */
#define IP6_NTM_ATTR_OFFSET         (IP6_NTM_SUB_LEN -  \
                                            (1 + IP6_ADDR_LEN) )

/* Offset of interface index in 'ipv6NetToMediaEntry'. */
#define IP6_NTM_IF_INDEX_OFFSET     (IP6_NTM_ATTR_OFFSET + 1)

/* Offset of ipv6NetToMediaNetAddress in 'ipv6NetToMediaEntry'. */
#define IP6_NTM_IF_NET_ADDR_OFFSET  (IP6_NTM_IF_INDEX_OFFSET + 1)

UINT16 ipv6NetToMediaEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_NTM_S_H */
