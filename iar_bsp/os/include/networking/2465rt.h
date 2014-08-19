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
*        2465rt.h                                    
*
*   DESCRIPTION
*
*        This file contain declaration of the functions that are
*        responsible for handling SNMP request for IPv6 route table.
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

#ifndef IP6_MIB_RT_S_H
#define IP6_MIB_RT_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_RT == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10
 * having ipv6RouteDest, ipv6RoutePfxLength and ipv6RouteIndex as
 * indexes this results in sub-length of 28. */
#define IP6_ROUTE_SUB_LEN           (12 + IP6_ADDR_LEN)

/* Offset of attribute in 'ipv6RouteEntry' */
#define IP6_ROUTE_ATTR_OFFSET       (IP6_ROUTE_SUB_LEN -    \
                                        (2 + IP6_ADDR_LEN))

/* Offset of 'ipv6RouteDest' in 'ipv6RouteEntry' */
#define IP6_ROUTE_DEST_OFFSET       (IP6_ROUTE_ATTR_OFFSET + 1)

/* Offset of 'ipv6RoutePfxLength' in 'ipv6RouteEntry' */
#define IP6_ROUTE_PFX_LEN_OFFSET    (IP6_ROUTE_DEST_OFFSET + IP6_ADDR_LEN)

/* Offset of 'ipv6RouteIndex' in 'ipv6RouteEntry' */
#define IP6_ROUTE_INDEX_OFFSET      (IP6_ROUTE_PFX_LEN_OFFSET + 1)

UINT16 ipv6RouteNumber(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipv6DiscardedRoutes(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipv6RouteEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_IPV6_RT == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE)) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_RT_S_H */
