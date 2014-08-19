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
*        2454udp.h                                   
*
*   DESCRIPTION
*
*        This file contains the declarations of the function(s) that are
*        responsible for handling SNMP requests on ipv6UdpTable.
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

#ifndef IP6_MIB_UDP_S_H
#define IP6_MIB_UDP_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_UDP_MIB == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 9
 * having ipv6UdpLocalAddress, ipv6UdpLocalPort and ipv6UdpIfIndex as
 * indexes this results in sub-length of 27.
 */
#define IP6_UDP_SUB_LEN             (11 + IP6_ADDR_LEN)

/* Offset of attribute in 'ipv6UdpEntry'. */
#define IP6_UDP_ATTR_OFFSET         (IP6_UDP_SUB_LEN - (2 + IP6_ADDR_LEN))

/* Offset of 'ipv6UdpLocalAddress' in 'ipv6UdpEntry'. */
#define IP6_UDP_LOCAL_ADDR_OFFSET   (IP6_UDP_ATTR_OFFSET + 1)

/* Offset of 'ipv6UdpLocalPort' in 'ipv6UdpEntry'. */
#define IP6_UDP_LOCAL_PORT_OFFSET   (IP6_UDP_LOCAL_ADDR_OFFSET +    \
                                        IP6_ADDR_LEN)

/* Offset of 'ipv6UdpIfIndex' in 'ipv6UdpEntry'. */
#define IP6_UDP_IF_INDEX_OFFSET     (IP6_UDP_LOCAL_PORT_OFFSET + 1)

UINT16 ipv6UdpEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_UDP_S_H */
