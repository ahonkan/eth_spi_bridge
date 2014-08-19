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
*       socket.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contains definitions for main sockets header.
*
* DATA STRUCTURES
*
*       sockaddr
*       sockaddr_storage
*       msghdr
*       cmsghdr
*       linger
*
* DEPENDENCIES
*
*       sim_posix.h
*       uio.h
*       stdint.h
*       nu_net.h
*
************************************************************************/
/* For GNU Toolset */
#ifndef _socket_h
#define _socket_h

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include "networking/net_cfg.h"
#include "services/compiler.h"
#include "services/sys/uio.h" 
#include "services/stdint.h"
#include "services/pnet.h"

#include "networking/nu_net.h"
#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/net6_cfg.h"
#endif /* INCLUDE_IPV6 */

typedef unsigned long       socklen_t;
typedef uint8_t             sa_family_t;

/* Large enough to accommodate all supported protocol-specific
   address structures  */
struct sockaddr_storage
{
    sa_family_t     sa_len;                 /* Length of the structure      */
    sa_family_t     ss_family;              /* Address family               */
    char            padding[126];           /* Padding and alignment
                                               Might be necessary for
                                               Protocol specific address    */
};


/* Structure used for manipulating linger option. */
struct  linger
{
    int l_onoff;                            /* Option on/off                */
    int l_linger;                           /* Linger time                  */
};

#if (INCLUDE_IPV6 == NU_TRUE)
#define PSX_ADDRESS_SIZE    26
#else
#define PSX_ADDRESS_SIZE    14
#endif

/* Structure used for socket address and length 16
   bytes-IPv4 and 28 bytes-IPv6 */
struct sockaddr
{
    sa_family_t     sa_len;                 /* Length of the structure      */
    sa_family_t     sa_family;              /* Address family               */
                                            /* Socket address
                                               (variable-length data)       */
    char            sa_data[PSX_ADDRESS_SIZE];
};

/* Indicates that the data array contains the access
   rights to be sent or received */
/* SCM_RIGHTS (NOT SUPPORTED) */


#define SOCK_STREAM     NU_TYPE_STREAM      /* Byte-stream socket           */
#define SOCK_DGRAM      NU_TYPE_DGRAM       /* Datagram socket              */
#define SOCK_RAW        NU_TYPE_RAW         /* Raw Protocol Interface       */
#define SOCK_SEQPACKET  3                   /* Sequenced-packet socket      */

/* Option Flags */
#define SO_ACCEPTCONN   5                   /* Socket is accepting
                                               connections                  */
#define SO_DEBUG        6                   /* Debugging information
                                               is being recorded            */
#define SO_DONTROUTE    7                   /* Bypass normal routing        */
#define SO_ERROR        8                   /* Socket error status          */
#define SO_OOBINLINE    9                   /* Out-of-band data is
                                               transmitted in line          */
#ifndef SO_RCVBUF
#define SO_RCVBUF       10                  /* Receive buffer size          */
#endif /* SO_RCVBUF */
#define SO_RCVLOWAT     11                  /* Receive "low water mark"     */
#define SO_RCVTIMEO     12                  /* Receive timeout              */
#define SO_SNDBUF       13                  /* Send buffer size             */
#define SO_SNDLOWAT     14                  /* Send"low water mark"         */
#define SO_SNDTIMEO     15                  /* Send timeout                 */
#define SO_TYPE         16                  /* Socket type                  */
#ifndef TCP_NODELAY
#define TCP_NODELAY     1
#endif /* TCP_NODELAY */
#ifndef SO_KEEPALIVE
#define SO_KEEPALIVE    2
#endif /* SO_KEEPALIVE */
/* SOL_SOCKET                                  (sockdefs.h)                 */
/* SO_BROADCAST                                (sockdefs.h)                 */
/* SO_LINGER                                   (sockdefs.h)                 */
/* SO_REUSEADDR                                (sockdefs.h)                 */
/* SHUT_RD                                     (sockdefs.h)                 */
/* SHUT_WR                                     (sockdefs.h)                 */
/* SHUT_RDWR                                   (sockdefs.h)                 */

/* The maximum backlog queue length for listen */
#define SOMAXCONN   128

#define MSG_CTRUNC      0x01                /* Control data truncated.      */
#define MSG_DONTROUTE   0x02                /* Send without using
                                               routing tables               */
#define MSG_EOR         0x04                /* Terminates a record
                                               (if supported by the
                                               protocol)                    */
#define MSG_OOB         0x08                /* Out-of-band data             */
#define MSG_PEEK        0x10                /* Leave received data 
                                               in queue                     */
#define MSG_TRUNC       0x20                /* Normal data truncated        */
#define MSG_WAITALL     0x40                /* Attempt to fill 
                                               the read buffer              */

/*Address families                                                          */
#define AF_UNSPEC       0                   /* Unspecified                  */
#define AF_UNIX         NU_FAMILY_UNIX      /* UNIX domain sockets          */
#define AF_INET         NU_FAMILY_IP        /* Internet domain sockets
                                               for use with IPv4 addresses  */
#define AF_INET6        NU_FAMILY_IP6       /* Internet domain sockets 
                                               for use with IPv6 addresses  */
#define PF_INET6        AF_INET6            /* IPv6 Family type             */


#ifdef __cplusplus
extern "C" {
#endif

int     accept(int, struct sockaddr *, socklen_t *);
int     bind(int, const struct sockaddr *, socklen_t);
int     connect(int, const struct sockaddr *, socklen_t);
int     getpeername(int, struct sockaddr *, socklen_t *);
int     getsockname(int, struct sockaddr *, socklen_t *);
int     getsockopt(int, int, int, void *, socklen_t *);
int     listen(int, int);
ssize_t recv(int, void *, size_t, int);
ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t recvmsg(int, msghdr *, int);
ssize_t sendmsg(int, msghdr *, int);
ssize_t send(int, const void *, size_t, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *,
        socklen_t);
int     setsockopt(int, int, int, const void *, socklen_t);
int     shutdown(int, int);
int     socket(int, int, int);

#ifdef __cplusplus
}
#endif

#endif /*_socket_h*/
#endif /*_SYS_SOCKET_H*/

