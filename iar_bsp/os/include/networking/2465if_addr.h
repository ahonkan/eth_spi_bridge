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
*        2465if_addr.h                               
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that are
*        responsible for handling SNMP requests on ipv6AddrTable.
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

#ifndef IP6_IF_ADDR_S_H
#define IP6_IF_ADDR_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10
 * having ipv6IfIndex and ipv6AddrAddress as indexes this results
 * in total sub-length of 27.
 */
#define IP6_IF_ADDR_SUB_LEN             (11 + IP6_ADDR_LEN)

/* Offset of attribute in 'ipv6AddrEntry'. */
#define IP6_IF_ADDR_ATTR_OFFSET         (IP6_IF_ADDR_SUB_LEN -  \
                                            (IP6_ADDR_LEN + 1))

/* Interface index's offset in 'ipv6AddrEntry'. */
#define IP6_IF_ADDR_IF_INDEX_OFFSET     (IP6_IF_ADDR_ATTR_OFFSET + 1)

/* IPv6 Address offset in 'ipv6AddrEntry'. */
#define IP6_IF_ADDR_ADDR_OFFSET         (IP6_IF_ADDR_IF_INDEX_OFFSET + 1)

UINT16 ipv6AddrEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_IF_ADDR_S_H */
