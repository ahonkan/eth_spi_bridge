/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       stdint.h
*
*   COMPONENT
*
*       Nucleus POSIX - Networking
*
*   DESCRIPTION
*
*       Contains definition for integer types.
*
*   DATA STRUCTURES
*
*       none
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_STDINT_H
#define NU_PSX_STDINT_H

#include "services/config.h"
#include "services/compiler.h"

#ifndef __stdint_h
#define __stdint_h

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _STDINT_H
#define _STDINT_H

/* Signed typedef.  */
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed long         int32_t;
typedef signed long long    int64_t;

/* Unsigned typedef.  */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;

/* Small typedef.  */

/* Signed typedef.  */
typedef signed char         int_least8_t;
typedef signed short        int_least16_t;
typedef int                 int_least32_t;
typedef signed long long    int_least64_t;

/* Unsigned typedef.  */
typedef unsigned char       uint_least8_t;
typedef unsigned short      uint_least16_t;
typedef unsigned int        uint_least32_t;
typedef unsigned long long  uint_least64_t;

/* Fast typedef.  */

/* Signed typedef.  */
typedef signed char         int_fast8_t;
typedef int                 int_fast16_t;
typedef int                 int_fast32_t;
typedef signed long long    int_fast64_t;

/* Unsigned typedef.  */
typedef unsigned char       uint_fast8_t;
typedef unsigned int        uint_fast16_t;
typedef unsigned int        uint_fast32_t;
typedef unsigned long long  uint_fast64_t;

/* Port and addr */
typedef uint16_t            in_port_t;
typedef uint32_t            in_addr_t;

/* Void pointers.  */
typedef int                 intptr_t;
typedef unsigned int        uintptr_t;

#if (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X))

/* Largest int types.  */
typedef long long int           intmax_t;
typedef unsigned long long int  uintmax_t;

#endif /* (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X)) */

/* Minimum of int signed types.  */
#define INT8_MIN            (-128)
#define INT16_MIN           (-32767-1)
#define INT32_MIN           (-2147483647-1)
#define INT64_MIN           (-9223372036854775807i64-1)

/* Maximum of int signed types.  */
#define INT8_MAX            (127)
#define INT16_MAX           (32767)
#define INT32_MAX           (2147483647)
#define INT64_MAX           (9223372036854775807i64)

/* Maximum of int unsigned types.  */
#define UINT8_MAX           (255)
#define UINT16_MAX          (65535)
#define UINT32_MAX          (4294967295U)
#define UINT64_MAX          (18446744073709551615ui64)

/* Minimum of int signed types (least).  */
#define INT_LEAST8_MIN      (-128)
#define INT_LEAST16_MIN     (-32767-1)
#define INT_LEAST32_MIN     (-2147483647-1)
#define INT_LEAST64_MIN     (-9223372036854775807i64-1)

/* Maximum of int signed typedef types (least).  */
#define INT_LEAST8_MAX      (127)
#define INT_LEAST16_MAX     (32767)
#define INT_LEAST32_MAX     (2147483647)
#define INT_LEAST64_MAX     (9223372036854775807i64)

/* Maximum of int unsigned typedef types (least).  */
#define UINT_LEAST8_MAX     (255)
#define UINT_LEAST16_MAX    (65535)
#define UINT_LEAST32_MAX    (4294967295U)
#define UINT_LEAST64_MAX    (18446744073709551615ui64)

/* Minimum of int signed typedef types (fast).  */
#define INT_FAST8_MIN       (-128)
#define INT_FAST16_MIN      (-2147483647-1)
#define INT_FAST32_MIN      (-2147483647-1)
#define INT_FAST64_MIN      (-9223372036854775807i64-1)

/* Maximum of int signed typedef types (fast).  */
#define INT_FAST8_MAX       (127)
#define INT_FAST16_MAX      (2147483647)
#define INT_FAST32_MAX      (2147483647)
#define INT_FAST64_MAX      (9223372036854775807i64)

/* Maximum of int unsigned typedef types (fast).  */
#define UINT_FAST8_MAX      (255)
#define UINT_FAST16_MAX     (4294967295U)
#define UINT_FAST32_MAX     (4294967295U)
#define UINT_FAST64_MAX     (18446744073709551615ui64)

/* Values of int int typedef  holding the pointer.  */
#define INTPTR_MIN          (-2147483647-1)
#define INTPTR_MAX          (2147483647)
#define UINTPTR_MAX         (4294967295U)

/* Minimum for largest signed type.  */
#define INTMAX_MIN          (9223372036854775807LL * -1)

/* Maximum for largest signed type.  */
#define INTMAX_MAX          (9223372036854775807LL)

/* Maximum for largest unsigned type.  */
#define UINTMAX_MAX         (18446744073709551615ULL)

/* Limit for ptrdiff_t type.  */
#define PTRDIFF_MIN         (-2147483647-1)
#define PTRDIFF_MAX         (2147483647)

/* Limit for sig_atomic_t type  */
#define SIG_ATOMIC_MIN      (-2147483647-1)
#define SIG_ATOMIC_MAX      (2147483647)

/* Limit for size_t type */
#define SIZE_MAX            (4294967295U)

/* Limit for wchar_t type.  Note: wchar_t is a built-in type in C++. */
#ifdef __cplusplus
    #if defined(PSX_HIT) || defined(PSX_H8) || defined(PSX_XTENSA)
#define WCHAR_MIN           (0U)
#define WCHAR_MAX           (255U)
    #endif /* defined(PSX_HIT) || defined(PSX_H8) || defined(PSX_XTENSA) */
#else /* C compilation */
#define WCHAR_MIN           (0U)
#define WCHAR_MAX           (255U)
#endif /* __cplusplus */

/* Limit for wint_t type  */
#define WINT_MIN            (0U)
#define WINT_MAX            (65535U)

#endif /* _STDINT_H */
#endif /* __stdint_h */

#endif /* NU_PSX_STDINT_H */
