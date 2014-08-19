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
*       netdb.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contains definitions for network database operations.
*
* DATA STRUCTURES
*
*       hostent
*       netent
*       protoent
*       servent
*       addrinfo
*
* DEPENDENCIES
*
*       socket.h
*       errno.h
*
************************************************************************/
#ifndef _NETDB_H
#define _NETDB_H

#include "services/sys/socket.h"
#include "services/errno.h"

/* The hostent structure includes at least the following members: */
struct hostent
{
    char   *h_name;                         /* Official name of the host.   */
    char  **h_aliases;                      /* Not USED                     */
    int     h_addrtype;                     /* Address type.                */
    int     h_length;                       /* The length, in bytes,
                                               of the address.              */
    char  **h_addr_list;                    /* A pointer to an array of
                                               pointers to network
                                               addresses (in network byte
                                               order) for the host, terminated
                                               by a null pointer.           */
};

/* The netent structure includes at least the following members: */
struct netent
{
    char     *n_name;                       /* Official, fully-qualified
                                               (including the domain)
                                               name of the host.            */
    char    **n_aliases;                    /* A pointer to an array of
                                               pointers to alternative
                                               network names, terminated by a
                                               null pointer.                */
    int       n_addrtype;                   /* The address type of the
                                               network.                     */
    uint32_t  n_net;                        /* The network number, in host
                                               byte order.                  */
};

/* The protoent structure includes at least the following members: */
struct protoent
{
    char   *p_name;                         /* Official name of the
                                               protocol.                    */
    char  **p_aliases;                      /* A pointer to an array of
                                               pointers to alternative
                                               protocol names, terminated
                                               by a null pointer.           */
    int     p_proto;                        /* The protocol number.         */
};

/* The servent structure includes at least the following members: */
struct servent
{
    char   *s_name;                         /* Official name of the service.*/
    char  **s_aliases;                      /* A pointer to an array of
                                               pointers to alternative service
                                               names, terminated by a null
                                               pointer.                     */
    int     s_port;                         /* The port number at which the
                                               service resides, in network
                                               byte order.                  */
    char   *s_proto;                        /* The name of the protocol to
                                               use when contacting the
                                               service.                     */
};

/* Address Information Structure */

/* The addrinfo structure includes at least the following members: */
struct addrinfo
{
    int               ai_flags;             /* Input flags.                 */
    int               ai_family;            /* Address family of socket.    */
    int               ai_socktype;          /* Socket type.                 */
    int               ai_protocol;          /* Protocol of socket.          */
    socklen_t         ai_addrlen;           /* Length of socket address.    */
    struct sockaddr  *ai_addr;              /* Socket address of socket.    */
    char             *ai_canonname;         /* Canonical name of
                                               service location.            */
    struct addrinfo  *ai_next;              /* Pointer to next in list.     */
};

/* Maps h_errno to errno */
#define h_errno   errno

/* The error values for gethostbyaddr() and gethostbyname(): */
#define HOST_NOT_FOUND  89
#define PNO_DATA        90
#define NO_RECOVERY     91
#define TRY_AGAIN       92

/* The following macros evaluate to bitwise-distinct
   integer constants for use in the flags field of the addrinfo structure */

#define AI_PASSIVE      0x0001              /* Socket address is intended
                                               for bind().                  */
#define AI_CANONNAME    0x0002              /* Request for canonical name.  */
#define AI_NUMERICHOST  0x0004              /* Return numeric host address
                                               as name.                     */
#define AI_NUMERICSERV  0x0008              /* Inhibit service name
                                               resolution.                  */
#define AI_V4MAPPED     0x0010              /* If no IPv6 addresses are found,
                                               query for IPv4 addresses and
                                               return them to the caller as
                                               IPv4-mapped IPv6 addresses   */
#define AI_ALL          0x0020              /* Query for both IPv4 and IPv6
                                               addresses.*/
#define AI_ADDRCONFIG   0x0040              /* Query for IPv4 addresses only
                                               when an IPv4 address is
                                               configured; query for IPv6
                                               addresses only when an IPv6
                                               address is configured.       */

#define NI_NOFQDN       1                   /* Only the node name portion
                                               of the FQDN is returned for
                                               local hosts.                 */
#define NI_NUMERICHOST  2                   /* The numeric form of the
                                               node's address is returned
                                               instead of its name.         */
#define NI_NAMEREQD     4                   /* Return an error if the node's
                                               name cannot be located in the
                                               database.                    */
#define NI_NUMERICSERV  8                   /* The numeric form of the
                                               service address is returned
                                               instead of its name.         */
#define NI_NUMERICSCOPE 16                  /* For IPv6 addresses, the
                                               numeric form of the scope
                                               identifier is returned
                                               instead of its name.         */
#define NI_DGRAM        32                  /* Indicates that the service
                                               is a datagram service
                                               (SOCK_DGRAM).                */

/* Address Information Errors */

#define EAI_AGAIN       -1                  /* The name could not be
                                               resolved at this time.Future
                                               attempts may succeed.        */
#define EAI_BADFLAGS    -2                  /* The flags had an invalid
                                               value.                       */
#define EAI_FAIL        -3                  /* A non-recoverable error
                                               occurred.                    */
#define EAI_FAMILY      -4                  /* The address family was not
                                               recognized or the address
                                               length was invalid for the
                                               specified family.            */
#define EAI_MEMORY      -5                  /* There was a memory allocation
                                               failure.                     */
#define EAI_NONAME      -6                  /* The name does not resolve for
                                               the supplied parameters.     */
#define EAI_SERVICE     -7                  /* The service passed was not
                                               recognized for the specified
                                               socket type.                 */
#define EAI_SOCKTYPE    -8                  /* The intended socket type was
                                               not recognized.              */
#define EAI_SYSTEM      -9                  /* A system error occurred.
                                               The error code can be found
                                               in errno.                    */
#define EAI_OVERFLOW    -10                 /* An argument buffer
                                               overflowed.                  */

#ifdef __cplusplus
extern "C" {
#endif

void              endhostent(void);
struct hostent   *gethostbyaddr(const void *, socklen_t, int);
struct hostent   *gethostbyname(const char *);
struct hostent   *gethostent(void);
void              sethostent(int);

#ifdef __cplusplus
}
#endif

#endif /* _NETDB_H */
