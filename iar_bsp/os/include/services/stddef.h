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
*       stddef.h
*
*   COMPONENT
*
*       RTL - RunTime Library.
*
*   DESCRIPTION
*
*       This file contains the standard type definitions.
*
*   DATA STRUCTURES
*
*       ptrdiff_t           Signed integer type of the result of
*                           subtracting two pointers.
*       wchar_t             Wide character type.
*       size_t              Unsigned integer type of the result of the
*                           sizeof operator.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_STDDEF_H
#define NU_PSX_STDDEF_H

#include "services/config.h"
#include "services/compiler.h"

/* For Metaware Metrowerks and KMC GNU Tools */
#ifndef _STDDEF_H
#define _STDDEF_H

/* For ADS Tools */
#ifndef __stddef_h
#define __stddef_h

/* For Paradigm Tools and Microtec Tools */
#ifndef __STDDEF_H
#define __STDDEF_H

/* For Microsoft Visual C */
#ifndef _INC_STDDEF
#define _INC_STDDEF

#ifndef __STDDEF_H_
#define __STDDEF_H_

/* For Code Sourcery ARM GNU */
#ifndef _ANSI_STDDEF_H
#define _ANSI_STDDEF_H

#ifndef __STDDEF_H__
#define __STDDEF_H__

/* For Code Sourcery ARM GNU */
#ifndef _STDDEF_H_
#define _STDDEF_H_

/* For DIAB tools */
#ifndef __Istddef
#define __Istddef

/* For TI tools, __TMS470__ and others */
#ifdef PSX_TI
    #ifdef __cplusplus

#include <stddef.h>

    #else /* not cplusplus */
        #ifndef _STDDEF

#define _STDDEF

        #endif /* STDDEF */
    #endif /* __cplusplus */
#endif /* PSX_TI */

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

/*  Integer constant expression of type size_t, the value of which is the
    offset in bytes to the structure member (member-designator), from the
    beginning of its structure (type). */
#ifndef offsetof
#define offsetof(s, m) (int)(&((s*)0)->m)
#endif

/* ptrdiff_t */

/* Pointer subtraction in C++ results in a ptrdiff_t value, so ptrdiff_t
   will be supplied. */
#ifndef SUN3
#ifndef SUN4
#ifndef HP700
#ifndef _PTRDIFF_T_
#define _PTRDIFF_T_

/* Signed integer type of the result of subtracting two pointers. */
typedef int ptrdiff_t;

#endif /* _PTRDIFF_T_ */
#endif /* HP700 */
#endif /* SUN4 */
#endif /* SUN3 */

/* wchar_t */

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED

#ifndef _WCHAR_T
#define _WCHAR_T

#ifndef __wchar_t__
#define __wchar_t__

#ifndef __wchar_t
#define __wchar_t

#ifndef __wchar_t_
#define __wchar_t_

#ifndef WCHAR_CK
#define WCHAR_CK

/* Determine if this is C++ compilation.  In C++, wchar_t is a built-in
   type and should not be define here, though some targets/tools still
   need it defined explicitly.  If this is C compilation then always
   provide a wchar_t type. */
#ifdef __cplusplus
    #if defined(PSX_HIT) || defined(PSX_H8) || defined(PSX_XTENSA)

typedef unsigned short wchar_t;

    #endif /* defined(PSX_HIT) || defined(PSX_H8) || defined(PSX_XTENSA) */

#else /* C compilation */

#ifdef PSX_CSGNU

/* Define the wide character type using the CSGNU internal definition. */
typedef __WCHAR_TYPE__ wchar_t;

#else

#error ERROR: Wide Characters not supported

#endif /* PSX_CSGNU */

#endif /* __cplusplus */

#endif /* WCHAR_CK */
#endif /* _WCHAR_T_DEFINED */
#endif /* _WCHAR_T */
#endif /* __wchar_t__ */
#endif /* __wchar_t  */
#endif /* __wchar_t_ */

/* size_t */

/* For Metaware tools TI, Paradigm and Hitachi Tools */
#ifndef   _SIZE_T
#define   _SIZE_T
#endif /* __SIZE_T_ */

/* For KMC GNU Tools */
#ifndef _SIZE_T_DEF
#define _SIZE_T_DEF
#endif /* _SIZE_T_DEF */

/* Microsoft Visual C and Microtec Tools*/
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#endif /* _SIZE_T_DEFINED */

/* For Metrowerks Tools */
#ifndef __size_t__
#define __size_t__
#endif /* __size_t__ */

#ifndef __SIZE_T_
#define __SIZE_T_
#endif /* _SIZE_T */

#ifdef _SIZE_T
#ifdef _SIZE_T_DEF
#ifdef _SIZE_T_DEFINED
#ifdef __size_t__
#ifdef __SIZE_T_

#ifndef SIZE_CK
#define SIZE_CK

#ifdef __cplusplus
    #if defined(PSX_SDE_GNU)

typedef __SIZE_TYPE__ size_t;

    #elif defined(PSX_RHGNU) || defined(PSX_NIOS2) || defined(PSX_HIT) || defined(PSX_H8)

typedef unsigned long size_t;

    #else

typedef unsigned int size_t;

    #endif /* PSX_SDE_GNU */
#else /* not __cplusplus */
    #if defined(PSX_HIT) || defined(PSX_H8)

typedef unsigned long size_t;

    #else

typedef unsigned int size_t;

    #endif /* defined(PSX_HIT) || defined(PSX_H8) */
#endif /* __cplusplus */

#endif /* SIZE_CK */
#endif /* __SIZE_T_ */
#endif /* _SIZE_T_DEF */
#endif /* _SIZE_T_DEFINED */
#endif /* __size_t__ */
#endif /* _SIZE_T */

#endif /* __Istddef */
#endif /* _STDDEF_H_ */
#endif /* __STDDEF_H__ */
#endif /* _ANSI_STDDEF_H */
#endif /* __STDDEF_H_ */
#endif /* _INC_STDDEF */
#endif /* __STDDEF_H */
#endif /* __stddef_h */
#endif /* _STDDEF_H */

#endif /* NU_PSX_STDDEF_H */
