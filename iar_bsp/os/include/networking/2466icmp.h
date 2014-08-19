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
*        2466icmp.h                                  
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that are
*        responsible for the handling SNMP request on ipv6IfIcmpTable.
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

#ifndef IP6_MIB_ICMP_S_H
#define IP6_MIB_ICMP_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_ICMP_MIB == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10
 * having ipv6IfIndex as index this results in sub-length of 11.
 */
#define IP6_IF_ICMP_SUB_LEN             11

/* Offset of attribute in 'ipv6IfIcmpEntry'. */
#define IP6_IF_ICMP_ATTR_OFFSET         (IP6_IF_ICMP_SUB_LEN - 1)

/* Offset of interface index in 'ipv6IfIcmpEntry'. */
#define IP6_IF_ICMP_IF_INDEX_OFFSET     (IP6_IF_ICMP_ATTR_OFFSET + 1)

UINT16 ipv6IfIcmpEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_ICMP_S_H */
