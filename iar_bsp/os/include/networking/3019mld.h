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
*        3019mld.h                                   
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that are
*        responsible for handling SNMP requests on the MLD Group MIBs.
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

#ifndef IP6_MIB_MLD_S_H
#define IP6_MIB_MLD_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_MLD_MIB == NU_TRUE) && \
      (INCLUDE_IP_MULTICASTING == NU_TRUE) && \
      (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10
 * having mldInterfaceIfIndex index this results in sub-length of 11.
 */
#define IP6_MLD_IF_SUB_LEN          11

/* Offset of the attribute in 'mldInterfaceEntry'. */
#define IP6_MLD_IF_ATTR_OFFSET      (IP6_MLD_IF_SUB_LEN - 1)

/* Offset of the 'mldInterfaceIfIndex' in 'mldInterfaceEntry'. */
#define IP6_MLD_IF_IF_INDEX_OFFSET  (IP6_MLD_IF_ATTR_OFFSET + 1)

/* Object Identifier sub-length. Table entry OID is of length 10
 * having mldCacheAddress and mldCacheIfIndex as indexes this results
 * in sub-length of 27.
 */
#define IP6_MLD_CACHE_SUB_LEN           (11 + IP6_ADDR_LEN)

/* Offset of attribute in 'mldCacheEntry'. */
#define IP6_MLD_CACHE_ATTR_OFFSET       (IP6_MLD_CACHE_SUB_LEN -    \
                                            (1 + IP6_ADDR_LEN) )

/* Offset of IPv6 address of cache entry. */
#define IP6_MLD_CACHE_ADDR_OFFSET       (IP6_MLD_CACHE_ATTR_OFFSET + 1)

/* Offset of interface index of cache entry. */
#define IP6_MLD_CACHE_IF_INDEX_OFFSET   (IP6_MLD_CACHE_ADDR_OFFSET +    \
                                            IP6_ADDR_LEN)

UINT16 mldInterfaceEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 mldCacheEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_IPV6_MLD_MIB == NU_TRUE) && \
           (INCLUDE_IP_MULTICASTING == NU_TRUE) && \
           (INCLUDE_SNMP == NU_TRUE)) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_MLD_S_H */
