/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       nussh_net.h
*
*   DESCRIPTION
*
*       This file defines an Net compatibility layer for Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       networking.h
*
************************************************************************/

#ifndef NUSSH_NET_H_
#define NUSSH_NET_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Define the Ethernet MAC address length. */
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

/*
 * Network API definitions and mappings to Nucleus equivalents.
 */

#ifndef IFF_UP
#define IFF_UP DV_UP
#endif
#ifndef IFF_RUNNING
#define IFF_RUNNING DV_RUNNING
#endif
#ifndef SO_ERROR
#define SO_ERROR 0x1234
#endif
#define PF_INET							NU_FAMILY_IP
#define AF_INET							NU_FAMILY_IP
#define SOCK_DGRAM						NU_TYPE_DGRAM
#define SOCK_STREAM						NU_TYPE_STREAM
#define ARPHRD_ETHER					ETHER

/* select() related macros. */
#undef fd_set
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#define fd_set							FD_SET
#define FD_ZERO(set)					NU_FD_Init(set)
#define FD_SET(fd, set)					NU_FD_Set(fd, set)
#define FD_ISSET(fd, set)				NU_FD_Check(fd, set)

#undef TCP_CONGESTION
#define INVALID_SOCKET                  -1
#define HOST_NOT_FOUND                  -1
#define NO_ADDRESS                      -2
#define NO_RECOVERY                     -3
#define TRY_AGAIN                       -4
#ifdef  EADDRINUSE
#undef  EADDRINUSE
#endif /* EADDRINUSE */
#define EADDRINUSE                      NU_ADDRINUSE
#ifdef  EAGAIN
#undef  EAGAIN
#endif /* EAGAIN */
#define EAGAIN                          NU_WOULD_BLOCK
#ifdef  ENOENT
#undef  ENOENT
#endif /* ENOENT */
#define ENOENT                          NUF_NOFILE
#define INADDR_ANY                      IP_ADDR_ANY

extern  INT    NUPTY_Parent_Socket;

extern  UINT8   IP6_Loopback_Address[16];

/* Network byte-order conversion macros. */
#define htons(hostshort)				(TLS_Intswap(hostshort))
#define ntohl(hostlong)					(TLS_Longswap(hostlong))
#define htonl(hostlong)					(TLS_Longswap(hostlong))
#define ntohs(hostshort)				(TLS_Intswap(hostshort))

/* Map network functions to the OS layer. */
#define accept nussh_accept
#define bind nussh_bind
#define connect nussh_connect
#define listen nussh_listen
#define getpeername nussh_getpeername
#define getsockname nussh_getsockname
#define getsockopt nussh_getsockopt
#define socket nussh_socket
#define setsockopt nussh_setsockopt
#define read nussh_recv
#define write nussh_send

#define select nussh_select
#define close nussh_close
#define shutdown(a,b) nussh_shutdown(a,b)

#define inet_ntoa nussh_inet_ntoa
#define inet_aton nussh_inet_aton
#define inet6_ntoa nussh_inet6_ntoa
#define inet6_aton nussh_inet6_aton
#define getservbyname nussh_getservbyname
#define gethostbyname nussh_gethostbyname
#define gethostbyaddr nussh_gethostbyaddr

int nussh_socket(int socket_family, int socket_type, int protocol);
int nussh_bind(int socket, struct sockaddr *addr, socklen_t addr_len);
int nussh_connect(int socket, struct sockaddr *addr, socklen_t addr_len);
int nussh_listen(int sockfd, int backlog);
int nussh_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t nussh_sendto(int socket, const void *message, size_t length,
                   int flags, const struct sockaddr *dest, socklen_t destlen);
ssize_t nussh_recv(int socket, void *buf, size_t len);
ssize_t nussh_send(int socket, const void *message, size_t length);
ssize_t nussh_recvfrom(int socket, void *buf, size_t len, int flags,
                     struct sockaddr *from, socklen_t *fromlen);
char *nussh_inet_ntoa(struct in_addr in);
int nussh_inet_aton(const char *cp, struct in_addr *inp);
char *nussh_inet6_ntoa(struct in6_addr in);
int nussh_inet6_aton(const char *cp, struct in6_addr *inp);
int nussh_close(int socket);
struct servent *nussh_getservbyname(const char *name, const char *proto);
struct hostent *nussh_gethostbyname(const char *name);
struct hostent *nussh_gethostbyaddr(const void *addr, socklen_t len, int type);
int nussh_getsockname(int socket, struct sockaddr *address,
                    socklen_t *address_len);
int nussh_getpeername(int socket, struct sockaddr *address,
                    socklen_t *address_len);
int nussh_setsockopt(int s, int level, int optname, const void *optval,
                   socklen_t optlen);
int nussh_getsockopt(int s, int level, int optname, void *optval,
                   socklen_t *optlen);
int nussh_shutdown(int s, int how);
int nussh_select(int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif /* NUSSH_NET_H_ */
