/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME               
*
*       inv6.h                  
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contains definitions for Internet address family.
*
* DATA STRUCTURES
*
*       ipv6_mreq
*
* DEPENDENCIES
*
*       stdint.h
*
************************************************************************/
#ifndef _PNET_INV6_H
#define _PNET_INV6_H

#include "services/stdint.h"

extern struct id_struct     IP6_ADDR_ANY;
extern struct id_struct     IP6_LOOPBACK_ADDR;

/* IPv6 Wild card Address  */
/* IP6_ADDR_ANY is of type id_struct, so needs to be type casted  */
#define in6addr_any                 *((struct in6_addr *)&IP6_ADDR_ANY) 
#define IN6ADDR_ANY_INIT            IP6ADDR_ANY_INIT

/* IPv6 Loop back Address */
/* IP6_LOOPBACK_ADDR is of type id_struct, so needs to be type casted */
#define in6addr_loopback            *((struct in6_addr *)&IP6_LOOPBACK_ADDR)
#define IN6ADDR_LOOPBACK_INIT       IP6ADDR_LOOPBACK_INIT

/*IPv6 Address Testing Macros*/
#define IN6_IS_ADDR_LINKLOCAL       IPV6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LOOPBACK        IPV6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_MC_GLOBAL       IPV6_IS_ADDR_MC_GLOBAL
#define IN6_IS_ADDR_MC_LINKLOCAL    IPV6_IS_ADDR_MC_LINKLOCAL
#define IN6_IS_ADDR_MC_NODELOCAL    IPV6_IS_ADDR_MC_NODELOCAL
#define IN6_IS_ADDR_MC_ORGLOCAL     IPV6_IS_ADDR_MC_ORGLOCAL
#define IN6_IS_ADDR_MC_SITELOCAL    IPV6_IS_ADDR_MC_SITELOCAL
#define IN6_IS_ADDR_MULTICAST       IPV6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_UNSPECIFIED     IPV6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_V4COMPAT        IPV6_IS_ADDR_V4COMPAT
#define IN6_IS_ADDR_V4MAPPED        IPV6_IS_ADDR_V4MAPPED

/* Size(bytes) of IPv6 address in string form */
#define INET6_ADDRSTRLEN            46

/* in6_addr and sockaddr_in6 are removed from IPv6 in 
   version 4 or above */
#ifdef IPV6_1_4
/* This array is used to contain a 128-bit IPv6 address, 
   stored in network byte order. */
struct in6_addr
{
	uint8_t s6_addr[16];
};

/* The sin6_port and sin6_addr members shall be in network 
byte order. The sockaddr_in6 structure shall be set to zero
by an application prior to using it, since implementations 
are free to have additional,implementation-defined fields 
in sockaddr_in6.The sin6_scope_id field is a 32-bit integer
that identifies a set of interfaces as appropriate for the
scope of the address carried in the sin6_addr field. For a
link scope sin6_addr, the application shall ensure that 
sin6_scope_id is a link index. For a site scope sin6_addr,
the application shall ensure that sin6_scope_id is a site
index. The mapping of sin6_scope_id to an interface or set
of interfaces is implementation-defined.
*/
struct sockaddr_in6
{
    sa_family_t      sa_len;         /* Length of the structure      */
	sa_family_t      sin6_family;    /* AF_INET6. */
	in_port_t        sin6_port;      /* Port number. */
	uint32_t         sin6_flowinfo;  /* IPv6 traffic class 
	                                   and flow information. */
	struct in6_addr  sin6_addr;      /* IPv6 address. */
	uint32_t         sin6_scope_id;  /* Set of interfaces for a scope. */
};
#else

/* IPV6 IP address structure in6_addr and sockaddr_in6 are defined 
inside netv6/inc/socketd6.h */
#ifdef  s6_addr
#undef  s6_addr
#endif /* s6_addr */
/* Address field of in6_addr */
#define s6_addr     u6_addr8

#endif /* IPV6_1_4 */

struct ipv6_mreq
{
    struct in6_addr ipv6mr_multiaddr;       /* IPv6 multicast address       */
    unsigned        ipv6mr_interface;       /* Interface index              */
};

#endif /* _PNET_INV6_H */

