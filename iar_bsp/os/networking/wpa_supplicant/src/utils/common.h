/*
 * wpa_supplicant/hostapd / common helper functions, etc.
 * Copyright (c) 2002-2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef COMMON_H
#define COMMON_H

/* Common header file for data-types for Nucleus platform. */ 
#include "openssl/nu_os_types.h"

#include "networking/nu_net.h"
#include "storage/pcdisk.h"
#include "os.h"

/* Define endian related macros. */

#ifdef CONFIG_NUCLEUS_OS
#define __BYTE_ORDER    ESAL_PR_ENDIANESS
#define __LITTLE_ENDIAN ESAL_LITTLE_ENDIAN
#define __BIG_ENDIAN    ESAL_BIG_ENDIAN
#endif /* CONFIG_NUCLEUS_OS */

#define getopt os_getopt
#define optind os_optind
/* Define platform specific integer types */

/* Define platform specific byte swapping macros */

#ifdef CONFIG_NUCLEUS_OS
static inline unsigned short bswap_16(unsigned short v)
{
    return ((v & 0xff) << 8) | (v >> 8);
}

static inline unsigned int bswap_32(unsigned int v)
{
    return ((v & 0xff) << 24) | ((v & 0xff00) << 8) |
        ((v & 0xff0000) >> 8) | (v >> 24);
}
#endif /* CONFIG_NUCLEUS_OS */

#ifndef WPA_BYTE_SWAP_DEFINED

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le_to_host16(n) ((__force u16) (le16) (n))
#define host_to_le16(n) ((__force le16) (u16) (n))
#define be_to_host16(n) bswap_16((__force u16) (be16) (n))
#define host_to_be16(n) ((__force be16) bswap_16((n)))
#define le_to_host32(n) ((__force u32) (le32) (n))
#define host_to_le32(n) ((__force le32) (u32) (n))
#define be_to_host32(n) bswap_32((__force u32) (be32) (n))
#define host_to_be32(n) ((__force be32) bswap_32((n)))
#define le_to_host64(n) ((__force u64) (le64) (n))
#define host_to_le64(n) ((__force le64) (u64) (n))
#define be_to_host64(n) bswap_64((__force u64) (be64) (n))
#define host_to_be64(n) ((__force be64) bswap_64((n)))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define le_to_host16(n) bswap_16(n)
#define host_to_le16(n) bswap_16(n)
#define be_to_host16(n) (n)
#define host_to_be16(n) (n)
#define le_to_host32(n) bswap_32(n)
#define be_to_host32(n) (n)
#define host_to_be32(n) (n)
#define le_to_host64(n) bswap_64(n)
#define host_to_le64(n) bswap_64(n)
#define be_to_host64(n) (n)
#define host_to_be64(n) (n)
#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN
#endif
#else
#error Could not determine CPU byte order
#endif

#define WPA_BYTE_SWAP_DEFINED
#endif /* !WPA_BYTE_SWAP_DEFINED */


/* Macros for handling unaligned memory accesses */

#define WPA_GET_BE16(a) ((u16) (((a)[0] << 8) | (a)[1]))
#define WPA_PUT_BE16(a, val)			\
	do {					\
		(a)[0] = ((u16) (val)) >> 8;	\
		(a)[1] = ((u16) (val)) & 0xff;	\
	} while (0)

#define WPA_GET_LE16(a) ((u16) (((a)[1] << 8) | (a)[0]))
#define WPA_PUT_LE16(a, val)			\
	do {					\
		(a)[1] = ((u16) (val)) >> 8;	\
		(a)[0] = ((u16) (val)) & 0xff;	\
	} while (0)

#define WPA_GET_BE24(a) ((((u32) (a)[0]) << 16) | (((u32) (a)[1]) << 8) | \
			 ((u32) (a)[2]))
#define WPA_PUT_BE24(a, val)					\
	do {							\
		(a)[0] = (u8) ((((u32) (val)) >> 16) & 0xff);	\
		(a)[1] = (u8) ((((u32) (val)) >> 8) & 0xff);	\
		(a)[2] = (u8) (((u32) (val)) & 0xff);		\
	} while (0)

#define WPA_GET_BE32(a) ((((u32) (a)[0]) << 24) | (((u32) (a)[1]) << 16) | \
			 (((u32) (a)[2]) << 8) | ((u32) (a)[3]))
#define WPA_PUT_BE32(a, val)					\
	do {							\
		(a)[0] = (u8) ((((u32) (val)) >> 24) & 0xff);	\
		(a)[1] = (u8) ((((u32) (val)) >> 16) & 0xff);	\
		(a)[2] = (u8) ((((u32) (val)) >> 8) & 0xff);	\
		(a)[3] = (u8) (((u32) (val)) & 0xff);		\
	} while (0)

#define WPA_GET_LE32(a) ((((u32) (a)[3]) << 24) | (((u32) (a)[2]) << 16) | \
			 (((u32) (a)[1]) << 8) | ((u32) (a)[0]))
#define WPA_PUT_LE32(a, val)					\
	do {							\
		(a)[3] = (u8) ((((u32) (val)) >> 24) & 0xff);	\
		(a)[2] = (u8) ((((u32) (val)) >> 16) & 0xff);	\
		(a)[1] = (u8) ((((u32) (val)) >> 8) & 0xff);	\
		(a)[0] = (u8) (((u32) (val)) & 0xff);		\
	} while (0)

#define WPA_GET_BE64(a) ((((u64) (a)[0]) << 56) | (((u64) (a)[1]) << 48) | \
			 (((u64) (a)[2]) << 40) | (((u64) (a)[3]) << 32) | \
			 (((u64) (a)[4]) << 24) | (((u64) (a)[5]) << 16) | \
			 (((u64) (a)[6]) << 8) | ((u64) (a)[7]))
#define WPA_PUT_BE64(a, val)				\
	do {						\
		(a)[0] = (u8) (((u64) (val)) >> 56);	\
		(a)[1] = (u8) (((u64) (val)) >> 48);	\
		(a)[2] = (u8) (((u64) (val)) >> 40);	\
		(a)[3] = (u8) (((u64) (val)) >> 32);	\
		(a)[4] = (u8) (((u64) (val)) >> 24);	\
		(a)[5] = (u8) (((u64) (val)) >> 16);	\
		(a)[6] = (u8) (((u64) (val)) >> 8);	\
		(a)[7] = (u8) (((u64) (val)) & 0xff);	\
	} while (0)

#define WPA_GET_LE64(a) ((((u64) (a)[7]) << 56) | (((u64) (a)[6]) << 48) | \
			 (((u64) (a)[5]) << 40) | (((u64) (a)[4]) << 32) | \
			 (((u64) (a)[3]) << 24) | (((u64) (a)[2]) << 16) | \
			 (((u64) (a)[1]) << 8) | ((u64) (a)[0]))


#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif


#ifdef __GNUC__
#define PRINTF_FORMAT(a,b) __attribute__((__format__(__printf__, (a), (b))))
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define PRINTF_FORMAT(a,b)
#define STRUCT_PACKED
#endif


#ifdef CONFIG_ANSI_C_EXTRA

#if (!defined(_MSC_VER) || _MSC_VER < 1400) && !defined(CONFIG_NUCLEUS_OS)
/* snprintf - used in number of places; sprintf() is _not_ a good replacement
 * due to possible buffer overflow; see, e.g.,
 * http://www.ijs.si/software/snprintf/ for portable implementation of
 * snprintf. */
int snprintf(char *str, size_t size, const char *format, ...);

/* vsnprintf - only used for wpa_msg() in wpa_supplicant.c */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif /* !defined(_MSC_VER) || _MSC_VER < 1400 */

/* getopt - only used in main.c */
int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int optind;

#ifndef __func__
#define __func__ "__func__ not defined"
#endif

#ifndef bswap_16
#define bswap_16(a) ((((u16) (a) << 8) & 0xff00) | (((u16) (a) >> 8) & 0xff))
#endif

#ifndef bswap_32
#define bswap_32(a) ((((u32) (a) << 24) & 0xff000000) | \
		     (((u32) (a) << 8) & 0xff0000) | \
     		     (((u32) (a) >> 8) & 0xff00) | \
     		     (((u32) (a) >> 24) & 0xff))
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

#ifdef _WIN32_WCE
void perror(const char *s);
#endif /* _WIN32_WCE */

#endif /* CONFIG_ANSI_C_EXTRA */

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

/*
 * Definitions for sparse validation
 * (http://kernel.org/pub/linux/kernel/people/josh/sparse/)
 */
#ifdef __CHECKER__
#define __force __attribute__((force))
#define __bitwise __attribute__((bitwise))
#else
#define __force
#define __bitwise
#endif

typedef u16 __bitwise be16;
typedef u16 __bitwise le16;
typedef u32 __bitwise be32;
typedef u32 __bitwise le32;
typedef u64 __bitwise be64;
typedef u64 __bitwise le64;

#ifndef __must_check
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __must_check __attribute__((__warn_unused_result__))
#else
#define __must_check
#endif /* __GNUC__ */
#endif /* __must_check */

int hwaddr_aton(const char *txt, u8 *addr);
int hexstr2bin(const char *hex, u8 *buf, size_t len);
void inc_byte_array(u8 *counter, size_t len);
void wpa_get_ntp_timestamp(u8 *buf);
int wpa_snprintf_hex(char *buf, size_t buf_size, const u8 *data, size_t len);
int wpa_snprintf_hex_uppercase(char *buf, size_t buf_size, const u8 *data,
			       size_t len);

#ifdef CONFIG_NATIVE_WINDOWS
void wpa_unicode2ascii_inplace(TCHAR *str);
TCHAR * wpa_strdup_tchar(const char *str);
#else /* CONFIG_NATIVE_WINDOWS */
#define wpa_unicode2ascii_inplace(s) do { } while (0)
#define wpa_strdup_tchar(s) strdup((s))
#endif /* CONFIG_NATIVE_WINDOWS */

const char * wpa_ssid_txt(const u8 *ssid, size_t ssid_len);

static inline int is_zero_ether_addr(const u8 *a)
{
	return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

#include "wpa_debug.h"


#ifdef CONFIG_NUCLEUS_OS

/* Various macros, structures and function definitions for porting of
 * ANSI C/POSIX code to Nucleus equivalents.
 */

/*
 * General libc definitions and mappings to Nucleus equivalents.
 */

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

/* Alarms/signals related macros. */
#define SIGHUP                          1
#define SIGINT                          2
#define SIGALRM                         14
#define SIGTERM                         15
#define SIGUSR1                         30
#define SIGUSR2                         31

#endif  /* CFG_NU_OS_SVCS_POSIX_CORE_ENABLE */

/* Map various functions to the OS layer. */

#define unlink os_unlink

int os_printf(const char *fmt, ...);
int os_vprintf(const char *format, va_list ap);
void os_perror(const char *msg);
int os_select(int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout);
void os_setdatetime(int year, int month, int day, int hour,
		int min, int sec);

/*
 * File related API definitions.
 */

int os_file_setup(void);
void os_file_deinit(void);
int os_file_open(const char *path, int flags, int mode);
size_t os_file_length(int file_desc);
int os_file_close(int file);
size_t os_file_read(int file, char *buf, size_t size);
size_t os_file_fprintf(int file, char *str);
char * os_fgets(char *s, int size, int stream);
int os_unlink(const char *pathname);

/*
 * Network API definitions and mappings to Nucleus equivalents.
 */

#ifndef IFF_UP
#define IFF_UP DV_UP
#endif
#ifndef IFF_RUNNING
#define IFF_RUNNING DV_RUNNING
#endif
#define PF_INET							NU_FAMILY_IP
#define AF_INET							NU_FAMILY_IP
#define SOCK_DGRAM						NU_TYPE_DGRAM
#define SOCK_STREAM						NU_TYPE_STREAM
#define ARPHRD_ETHER					ETHER

/* Network byte-order conversion macros. */
#define htons(hostshort)				(TLS_Intswap(hostshort))
#define ntohl(hostlong)					(TLS_Longswap(hostlong))
#define htonl(hostlong)					(TLS_Longswap(hostlong))
#define ntohs(hostshort)				(TLS_Intswap(hostshort))

/* Map network functions to the OS layer. */
#undef  socket
#define socket os_socket
#undef  bind
#define bind os_bind
#undef  connect
#define connect os_connect
#undef  send
#define send os_send
#undef  sendto
#define sendto os_sendto
#undef  recv
#define recv os_recv
#undef  recvfrom
#define recvfrom os_recvfrom
#undef  inet_ntoa
#define inet_ntoa os_inet_ntoa
#undef  close
#define close os_close
#undef  read 
#define read( s, b, l )  os_recv( s, (char*) b, l, 0 )
#undef  write
#define write( s, b, l ) os_send( s, (char*) b, l, 0 ) 
#undef  select
#define select os_select

int os_socket(int socket_family, int socket_type, int protocol);
int os_bind(int socket, struct sockaddr *addr, socklen_t addr_len);
int os_connect(int socket, struct sockaddr *addr, socklen_t addr_len);
ssize_t os_send(int socket, const void *message, size_t length, int flags);
ssize_t os_sendto(int socket, const void *message, size_t length,
		int flags, const struct sockaddr *dest, socklen_t destlen);
ssize_t os_recv(int socket, void *buf, size_t len, int flags);
ssize_t os_recvfrom(int socket, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen);
char * os_inet_ntoa(struct in_addr in);
int os_close(int socket);

#endif /* CONFIG_NUCLEUS_OS */

#endif /* COMMON_H */
