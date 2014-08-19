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
*       nu_types.h
*
*   DESCRIPTION
*
*       This file defines mapping of common data-types for Nucleus OS.
*
*   DATA STRUCTURES
*
*       .
*
*   DEPENDENCIES
*
*       nucleus.h
*
************************************************************************/
#ifndef NUSSH_TYPES_H
#define NUSSH_TYPES_H

#include "nucleus.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Redefine "inline" if using the ARMCC compiler. */
#if defined(__ARMCC_VERSION)
#define inline __inline
#endif

#if defined(ESAL_TS_64BIT_SUPPORT) && (ESAL_TS_64BIT_SUPPORT == NU_TRUE)
typedef UINT64 u64;
typedef INT64 s64;
#else
typedef unsigned long long u64;
typedef long long s64;
#endif /* (ESAL_TS_64BIT_SUPPORT) */



typedef INT8        int8_t;
typedef INT16       int16_t;
typedef INT32       int32_t;
typedef INT64       int64_t;

typedef UINT8       u_int8_t;
typedef UINT16      u_int16_t;
typedef UINT32      u_int32_t;
typedef UINT64      u_int64_t;

typedef UINT8       uint8_t;
typedef UINT16      uint16_t;
typedef UINT32      uint32_t;
typedef UINT64      uint64_t;

typedef INT         pid_t;

typedef UINT32      u_long;
typedef UINT        u_int;

typedef INT32 s32;
typedef INT16 s16;
typedef INT8 s8;

typedef UINT32 u32;
typedef UINT16 u16;
typedef UINT8 u8;

#define ssize_t long
#define time_t long

#define uid_t UINT16
#define gid_t UINT16

#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
typedef struct timeval {
    s32 tv_sec;
    s32 tv_usec;
} timeval;
#else
#include "sys/time.h"
#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */

struct itimerval {
    struct timeval it_interval; /* Next value */
    struct timeval it_value;    /* Current value */
};

/*
 * Network data-types mappings to Nucleus equivalents.
 */

/*Address families                                                          */
#define AF_UNSPEC       0                   /* Unspecified                  */
#define AF_INET         NU_FAMILY_IP        /* Internet domain sockets
                                               for use with IPv4 addresses  */
#define AF_INET6        NU_FAMILY_IP6       /* Internet domain sockets
                                               for use with IPv6 addresses  */
#define PF_INET6        AF_INET6            /* IPv6 Family type             */

#define PF_UNSPEC        0


#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
struct in_addr {
    u32 s_addr;
};

struct sockaddr_in {
    u16 sin_family;
    u16 sin_port;
    struct in_addr sin_addr;
    u8 sin_zero[8];
};

struct sockaddr {
    u16 sa_family;
#if (SSH_ENABLE_IPV6 == NU_FALSE)
    u8 sa_data[14];
#else
    u8 sa_data[28];
#endif
};

struct hostent {
    const char *h_name;         /* Name of host. */
    char    **h_aliases;        /* List of aliases. */
    short   h_addrtype;         /* Host address type. */
    short   h_length;           /* Length of address. */
    char    **h_addr_list;      /* List of host addresses. */
#define h_addr  h_addr_list[0]  /* Shortcut for first item in list. */
};

struct servent {
    char    *s_name;            /* Service name. */
    char    **s_aliases;        /* List of aliases. */
    short   s_port;             /* Port number. */
    char    *s_proto;           /* Protocol name. */
};

#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */

#define sa_family_t u16
#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
typedef int socklen_t;
#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */

#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
/* Structure used for manipulating linger option. */
struct  linger
{
    int l_onoff;                            /* Option on/off                */
    int l_linger;                           /* Linger time                  */
};
#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */

/* Definitions in signal.h. */
typedef UINT32 sigset_t;
typedef VOID __signalfn_t(INT);
typedef __signalfn_t *__sighandler_t;
typedef VOID __restorefn_t(VOID);
typedef __restorefn_t *__sigrestore_t;

#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
struct sigaction {
    __sighandler_t  sa_handler;
    unsigned long   sa_flags;
    __sigrestore_t  sa_restorer;
    sigset_t        sa_mask;  /* mask last for extensibility */
};
#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */


/** NEW ENTRIES **/


#define    LOG_EMERG    0    /* system is unusable */
#define    LOG_ALERT    1    /* action must be taken immediately */
#define    LOG_CRIT     2    /* critical conditions */
#define    LOG_ERR      3    /* error conditions */
#define    LOG_WARNING  4    /* warning conditions */
#define    LOG_NOTICE   5    /* normal but significant condition */
#define    LOG_INFO     6    /* informational */
#define    LOG_DEBUG    7    /* debug-level messages */


#define STDIN_FILENO    0x00
#define STDOUT_FILENO   0x01
#define STDERR_FILENO   0x02




#ifdef __cplusplus
}
#endif

#endif /* NUSSH_TYPES_H */
