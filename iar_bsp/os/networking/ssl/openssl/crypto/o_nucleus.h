/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
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
*       o_nucleus.h
*
*   DESCRIPTION
*
*       This file defines an OS compatibility layer for Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       pcdisk.h
*
************************************************************************/
#ifndef OPENSSL_NUCLEUS_H
#define OPENSSL_NUCLEUS_H

#include "nu_os_types.h"
#include "networking/nu_net.h"
#include "storage/pcdisk.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Define the Ethernet MAC address length. */
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif


/* Various macros, structures and function definitions for porting of
 * ANSI C/POSIX code to Nucleus equivalents.
 */

/*
 * General libc definitions and mappings to Nucleus equivalents.
 */

/* Replacement for "errno". */
extern u32 OPENSSL_errno;

/* select() related macros. */
#undef fd_set
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#define fd_set							FD_SET
#define FD_ZERO(set)					NU_FD_Init(set)
#define FD_SET(fd, set)					NU_FD_Set(fd, set)
#define FD_ISSET(fd, set)				NU_FD_Check(fd, set)

#ifndef CFG_NU_OS_SVCS_POSIX_CORE_ENABLE
#ifndef _SIGNAL_H_
/* Alarms/signals related macros. */
#define SIGHUP                          1
#define SIGINT                          2
#define SIGALRM                         14
#define SIGTERM                         15
#define SIGUSR1                         30
#define SIGUSR2                         31
#define SIGPIPE                         32
#define SIG_IGN                         33
#endif /* _SIGNAL_H_ */
#define ITIMER_REAL                     100

#endif  /* CFG_NU_OS_SVCS_POSIX_CORE_ENABLE */

/* Map memory and other general functions to the OS layer. */
#define zalloc cos_zalloc
#define malloc cos_malloc
#define realloc cos_realloc
#define free cos_free
#define strdup cos_strdup
#define snprintf cos_snprintf
#define alarm cos_alarm
#define signal cos_signal
#define exit cos_exit
#define atexit cos_atexit
#define printf cos_printf
#define vprintf cos_vprintf
#define perror cos_perror
#define gmtime cos_gmtime
#define time cos_time
#define clock cos_clock
#ifdef isdigit
#undef isdigit
#endif /* isdigit */
#define isdigit cos_isdigit
#ifdef isxdigit
#undef isxdigit
#endif /* isxdigit */
#define isxdigit cos_isxdigit
#ifdef isascii
#undef isascii
#endif /* isascii */
#define isascii cos_isascii

int cos_program_init(void);
void cos_program_deinit(void);

void *cos_zalloc(size_t size);
void *cos_malloc(size_t size);
void *cos_realloc(void *ptr, size_t size);
void cos_free(void *ptr);
char *cos_strdup(const char *s);
int cos_snprintf(char *str, size_t size, const char *format, ...);
unsigned cos_alarm(unsigned seconds);
void *cos_signal(int signum, void *handler);
void cos_exit(int status);
int cos_atexit(void (*function)(void));
int cos_printf(const char *fmt, ...);
int cos_vprintf(const char *format, va_list ap);
void cos_perror(const char *msg);
struct tm *cos_gmtime(const time_t *timer);
time_t cos_time(time_t* t);
clock_t cos_clock(void);
int cos_isdigit(int c);
int cos_isxdigit(int c);
int cos_isascii(int c);

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

/* Network byte-order conversion macros. */
#define htons(hostshort)				(TLS_Intswap(hostshort))
#define ntohl(hostlong)					(TLS_Longswap(hostlong))
#define htonl(hostlong)					(TLS_Longswap(hostlong))
#define ntohs(hostshort)				(TLS_Intswap(hostshort))

/* Map network functions to the OS layer. */
#define socket cos_socket
#define bind cos_bind
#define connect cos_connect
#define listen cos_listen
#define accept cos_accept
#define send cos_send
#define sendto cos_sendto
#define recv cos_recv
#define recvfrom cos_recvfrom
#define read( s, b, l )  cos_recv( s, (char*) b, l, 0 )
#define write( s, b, l ) cos_send( s, (char*) b, l, 0 )
#define inet_ntoa cos_inet_ntoa
#define close cos_close
#define getservbyname cos_getservbyname
#define gethostbyname cos_gethostbyname
#define getsockname cos_getsockname
#define getpeername cos_getpeername
#define setsockopt cos_setsockopt
#define getsockopt cos_getsockopt
#define shutdown(a,b) cos_shutdown(a,b)
#define select cos_select

int cos_socket(int socket_family, int socket_type, int protocol);
int cos_bind(int socket, struct sockaddr *addr, socklen_t addr_len);
int cos_connect(int socket, struct sockaddr *addr, socklen_t addr_len);
int cos_listen(int sockfd, int backlog);
int cos_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t cos_send(int socket, const void *message, size_t length, int flags);
ssize_t cos_sendto(int socket, const void *message, size_t length,
		           int flags, const struct sockaddr *dest, socklen_t destlen);
ssize_t cos_recv(int socket, void *buf, size_t len, int flags);
ssize_t cos_recvfrom(int socket, void *buf, size_t len, int flags,
		             struct sockaddr *from, socklen_t *fromlen);
char *cos_inet_ntoa(struct in_addr in);
int cos_close(int socket);
struct servent *cos_getservbyname(const char *name, const char *proto);
struct hostent *cos_gethostbyname(const char *name); 
int cos_getsockname(int socket, struct sockaddr *address,
                    socklen_t *address_len);
int cos_getpeername(int socket, struct sockaddr *address,
                    socklen_t *address_len);
int cos_setsockopt(int s, int level, int optname, const void *optval,
                   socklen_t optlen);
int cos_getsockopt(int s, int level, int optname, void *optval,
                   socklen_t *optlen);
int cos_shutdown(int s, int how);
int cos_select(int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout);

/*
 * File API definitions and mappings to Nucleus equivalents.
 */

/* Check for file system */
#ifndef CFG_NU_OS_STOR_FILE_FS_FAT_ENABLE
 #ifndef CFG_NU_OS_STOR_FILE_FS_SAFE_ENABLE
  #warning "No file system is defined. OpenSSL Crypto requires a file system"
 #endif
#endif
 
/* Define an internal type for the FILE data structure. */
typedef struct {
    int fd;
    int flags;
    int error;
} OPENSSL_FILE;

/* Map FILE to the void type. */
#define FILE void

/* Map file functions to the OS layer. */
#define fopen cos_fopen
#define fclose cos_fclose
#ifdef  feof
#undef  feof
#endif /* feof */
#define feof cos_feof
#define fseek cos_fseek
#define ftell cos_ftell
#define fflush cos_fflush
#define fgets cos_fgets
#ifdef  ferror
#undef  ferror
#endif /* ferror */
#define ferror cos_ferror
#define fread cos_fread
#define fwrite cos_fwrite

FILE *cos_fopen(const char *path, const char *mode);
int cos_fclose(FILE *fp);
int cos_feof(FILE *stream);
int cos_fseek(FILE *stream, long offset, int whence);
long cos_ftell(FILE *stream);
int cos_fflush(FILE *stream);
char *cos_fgets(char *s, int size, FILE *stream);
int cos_ferror(FILE *stream);
size_t cos_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t cos_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream); 

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_NUCLEUS_H */
