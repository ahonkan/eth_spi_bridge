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
*       stdlib.h
*
*   COMPONENT
*
*       RTL - RunTime Library.
*
*   DESCRIPTION
*
*       This file contains the standard library definitions.
*
*       NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*       div_t               Structure type returned by the div() function.
*       ldiv_t              Structure type returned by the
*                           ldiv() function.
*       size_t              An unsigned integer data type.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       unistd.h            POSIX unistd.h definitions
*       stddef.h            POSIX stddef.h definitions
*
*************************************************************************/

#ifndef NU_PSX_STDLIB_H
#define NU_PSX_STDLIB_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/unistd.h"
#include "services/stddef.h"

/* For Metaware Metrowerks and KMC GNU Tools */
#ifndef _STDLIB_H
#define _STDLIB_H

/* For ADS Tools */
#ifndef __stdlib_h
#define __stdlib_h

/* For Hitachi Tools and TI Tools  */
#ifndef _STDLIB
#define _STDLIB

/* For Paradigm Tools and Microtec Tools */
#ifndef __STDLIB_H
#define __STDLIB_H

/* For Microsoft Visual C */
#ifndef _INC_STDLIB
#define _INC_STDLIB

#ifndef __STDLIB_H_
#define __STDLIB_H_

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _STDLIB_H_
#define _STDLIB_H_

/* For DIAB tools */
#ifndef __Istdlib
#define __Istdlib

#ifdef __cplusplus
extern "C" {
#endif

#define EXIT_SUCCESS    0                   /*  Successful termination
                                                exit status  */
#define EXIT_FAILURE    1                   /*  Unsuccessful termination
                                                exit status */
#define RAND_MAX        32767               /*  Maximum value returned by
                                                rand function */
#define MB_CUR_MAX      2                   /*  Maximum bytes in multibyte
                                                character    */

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

/* Division type */

typedef struct STDLIB_DIV_T_STRUCT
{
    /* Quotient */

    int         quot;

    /* Remainder */

    int         rem;

} div_t;

/* Long Division type */

typedef struct STDLIB_LDIV_T_STRUCT
{
    /* Quotient */

    long        quot;

    /* Remainder */

    long        rem;

} ldiv_t;

#if (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X))

/* Long Long Division type */
#ifdef lldiv_t
#undef lldiv_t
#endif

typedef struct STDLIB_LLDIV_T_STRUCT
{
    /* Quotient */

    long long   quot;

    /* Remainder */

    long long   rem;

} PSX_lldiv_t;

#define lldiv_t PSX_lldiv_t

#endif /* (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X)) */

/*  Function Prototypes */

int     abs(int);
long    labs(long);

#if (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X))

long long           llabs(long long);
double              atof(const char *);
int                 atoi(const char *);
long                atol(const char *);
long long           atoll(const char *);
div_t               div(int, int);
ldiv_t              ldiv(long, long);
lldiv_t             lldiv(long long, long long);
double              strtod(const char *, char **);
float               strtof(const char *, char **);
long                strtol(const char *, char **, int);
long long           strtoll(const char *, char **, int);
long double         strtold(const char *, char **);
unsigned long       strtoul(const char *, char **, int);
unsigned long long  strtoull(const char *, char **, int);

#endif /* (!defined(PSX_MTEC68K) && !defined(PSX_MTECCF) && !defined(C55X)) */

void    abort(void);
int     atexit(void (*)(void));
void *  bsearch(const void *, const void *, size_t, size_t,
                int (*)(const void *, const void *));
void *  calloc(size_t, size_t);
void    exit(int);
void    free(void *);
char *  getenv(const char *);
void *  malloc(size_t);
void    qsort(void *, size_t, size_t, int (*)(const void *,const void *));
int     rand(void);

#if (_POSIX_THREAD_SAFE_FUNCTIONS != -1)

int     rand_r(unsigned *);

#endif  /* _POSIX_THREAD_SAFE_FUNCTIONS */

void *  realloc(void *, size_t);
int     setenv(const char *, const char *, int);
void    srand(unsigned);
int     system(const char *);
int     unsetenv(const char *);

/* Wide Character Support - These MUST match tools C library definitions
   to support the tools C++ support library. */
int     mblen(const char *, size_t);
size_t  mbstowcs(wchar_t *, const char *, size_t);
int     mbtowc(wchar_t *, const char *, size_t);
size_t  wcstombs(char *, const wchar_t *, size_t);
int     wctomb(char *, wchar_t);

#ifdef __cplusplus
}
#endif

#endif  /* __Istdlib */
#endif  /* _STDLIB_H_ */
#endif  /* __STDLIB_H_ */
#endif  /* _INC_STDLIB */
#endif  /* __STDLIB_H */
#endif  /* _STDLIB */
#endif  /* __stdlib_h */
#endif  /* _STDLIB_H */

#endif /* NU_PSX_STDLIB_H */
