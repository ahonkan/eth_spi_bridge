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
*        2465addr_pre.h                              
*
*   DESCRIPTION
*
*        This file contains the declarations of IPv6 Prefix Address
*        MIBs.
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

#ifndef IP6_MIB_ADDR_PRE_S_H
#define IP6_MIB_ADDR_PRE_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* _cplusplus */

#if ( (INCLUDE_SNMP == NU_TRUE) && \
      (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10 having
 * ipv6IfIndex, ipv6AddrPrefix and ipv6AddrPrefixLength as indexes this
 * results in sub-length of 28.
 */
#define IPV6_PRE_ADDR_SUB_LEN           (12 + IP6_ADDR_LEN)

#define IPV6_PRE_ADDR_ATTR_OFFSET       (IPV6_PRE_ADDR_SUB_LEN -    \
                                            (2 + IP6_ADDR_LEN))
           
/* Interface index offset. */
#define IPV6_PRE_ADDR_IF_INDEX_OFFSET   (IPV6_PRE_ADDR_SUB_LEN  \
                                            - (1 + IP6_ADDR_LEN))

/* Prefix address offset. */
#define IPV6_PRE_ADDR_PRE_ADDR_OFFSET   (IPV6_PRE_ADDR_SUB_LEN -    \
                                            IP6_ADDR_LEN)

/* Prefix address length offset. */
#define IPV6_PRE_ADDR_LEN_OFFSET        IPV6_PRE_ADDR_SUB_LEN

UINT16 ipv6AddrPrefixEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && \
           (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE) ) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_ADDR_PRE_S_H */
