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
*        2452tcp.h                                   
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that are
*        responsible for handling SNMP requests on ipv6TcpConnTable.
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

#ifndef IP6_MIB_TCP_S_H
#define IP6_MIB_TCP_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IPV6_TCP_MIB == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 9
 * having ipv6TcpConnLocalAddress, ipv6TcpConnLocalPort,
 * ipv6TcpConnRemAddress, ipv6TcpConnRemPort and ipv6TcpConnIfIndex as
 * indexes this results in sub-length of 44.
 */
#define IP6_TCP_CONN_SUB_LEN            (12 + (2 * IP6_ADDR_LEN))

/* Offset of attribute in 'ipv6TcpConnEntry'. */
#define IP6_TCP_CONN_ATTR_OFFSET        (IP6_TCP_CONN_SUB_LEN - \
                                            (3 + (2 * IP6_ADDR_LEN)))

/* Offset of 'ipv6TcpConnLocalAddress' in 'ipv6TcpConnEntry'. */
#define IP6_TCP_CONN_LOCAL_ADDR_OFFSET  (IP6_TCP_CONN_ATTR_OFFSET + 1)

/* Offset of 'ipv6TcpConnLocalPort' in 'ipv6TcpConnEntry'. */
#define IP6_TCP_CONN_LOCAL_PORT_OFFSET  (IP6_TCP_CONN_LOCAL_ADDR_OFFSET \
                                            + IP6_ADDR_LEN)

/* Offset of 'ipv6TcpConnRemAddress' in 'ipv6TcpConnEntry'. */
#define IP6_TCP_CONN_REMOT_ADDR_OFFSET  (IP6_TCP_CONN_LOCAL_PORT_OFFSET \
                                            + 1)

/* Offset of 'ipv6TcpConnRemPort' in 'ipv6TcpConnEntry'. */
#define IP6_TCP_CONN_REMOT_PORT_OFFSET  (IP6_TCP_CONN_REMOT_ADDR_OFFSET \
                                            + IP6_ADDR_LEN)

/* Offset of 'ipv6TcpConnIfIndex' in 'ipv6TcpConnEntry'. */
#define IP6_TCP_CONN_IF_INDEX_OFFSET    (IP6_TCP_CONN_REMOT_PORT_OFFSET \
                                            + 1)
    
UINT16 ipv6TcpConnEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_TCP_S_H */
