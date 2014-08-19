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
*       in.h                  
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
*       in_addr
*       sockaddr_in
*
* DEPENDENCIES
*
*       inttypes.h
*       socket.h
*       stdint.h
*       net6_cfg.h
*       ip6.h
*       inv6.h
*
************************************************************************/
#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include    "services/inttypes.h"
#include    "services/sys/socket.h"
#include    "services/stdint.h"
#if (INCLUDE_IPV6 == NU_TRUE)
#include    "networking/net6_cfg.h"
#include    "networking/ip6.h"
#include    "services/inv6.h"
#endif /* INCLUDE_IPV6 */

/* IPv4 local host address  */
#define INADDR_ANY          0x0UL
/* IPv4 broadcast address  */
#define INADDR_BROADCAST    0xFFFFFFFFUL

/* Size(bytes) of IPv4 address in string form */
#define INET_ADDRSTRLEN             16      

/* IPV4 IP address structure (4 bytes) */
struct in_addr
{
    in_addr_t  s_addr;
};

/* IPV4 socket structure (16 bytes) */
struct sockaddr_in
{
  sa_family_t       sin_len;                /* Length of the structure      */
  sa_family_t       sin_family;             /* AF_INET                      */
  in_port_t         sin_port;               /* Port number                  */
  struct in_addr    sin_addr;               /* IP address                   */
  char              sin_zero[8];            /* Padding based on sockaddr 
                                               (unused)                     */
};

#endif /*_NETINET_IN_H */
