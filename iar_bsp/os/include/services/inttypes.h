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
*       inttypes.h
*
*   COMPONENT
*
*       Nucleus POSIX - Networking
*
*   DESCRIPTION
*
*       Contains definition for fixed size integer types.
*
*   DATA STRUCTURES
*
*       imaxdiv_t
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       stdint.h            POSIX stding.h definitions
*       stddef.h            POSIX stddef.h definitions
*
*************************************************************************/

#ifndef NU_PSX_INTTYPES_H
#define NU_PSX_INTTYPES_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/stdint.h"
#include "services/stddef.h"

/* For RVCT */
#ifndef __inttypes_h
#define __inttypes_h

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _INTTYPES_H
#define _INTTYPES_H

#if (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X))

/* Maximum Integer Division type */
typedef struct STDLIB_IMAXDIV_T_STRUCT
{
    intmax_t    quot;       /* Quotient */
    intmax_t    rem;        /* Remainder */

} imaxdiv_t;

#endif /* (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X)) */

#ifdef __cplusplus
extern "C" {
#endif

#if (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X))

imaxdiv_t imaxdiv(intmax_t, intmax_t);
intmax_t strtoimax(const char *nptr, char **endptr, int base);
intmax_t  strtoimax(const char *, char **, int);
uintmax_t strtoumax(const char *, char **, int);

#endif /* (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X)) */

#ifdef __cplusplus
}
#endif

#endif /* _INTTYPES_H */
#endif /* __inttypes_h */

#endif /* NU_PSX_INTTYPES_H */
