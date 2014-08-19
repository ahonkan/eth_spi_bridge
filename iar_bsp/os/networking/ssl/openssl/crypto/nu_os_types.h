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
*       nu_os_types.h
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
#ifndef __NU_OS_TYPES_H__
#define __NU_OS_TYPES_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
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

typedef UINT32 u32;
typedef UINT16 u16;
typedef UINT8 u8;
typedef INT32 s32;
typedef INT16 s16;
typedef INT8 s8;
#define _NU_OS_TYPES_DEFINED_

#define ssize_t long
#define time_t long

#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
typedef struct timeval {
	s32 tv_sec;
	s32 tv_usec;
} timeval;

struct itimerval {
    struct timeval it_interval; /* Next value */
    struct timeval it_value;    /* Current value */
};
#else
#include "sys/time.h"
#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */

/*
 * Network data-types mappings to Nucleus equivalents.
 */

struct in_addr {
	u32 s_addr;
};

struct sockaddr_in {
	u16 sin_family;
	u16 sin_port;
	struct in_addr sin_addr;
    u8 sin_zero[8];
};

struct sockaddr_storage {
	u16 ss_family;
    u8 ss_pad[126];
};

struct sockaddr {
	u16 sa_family;
    u8 sa_data[14];
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

#define sa_family_t u16
typedef int socklen_t;

#ifdef __cplusplus
}
#endif

#endif /* __NU_OS_TYPES_H__ */
