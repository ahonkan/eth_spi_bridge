/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       ncl.h
*
*   COMPONENT
*
*       NET - NET 'C' Library
*
*   DESCRIPTION
*
*       This file contains prototypes and macros for the NCL.C
*       module.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       limits.h
*
***********************************************************************/
#ifndef NCL_H
#define NCL_H


#if ( (defined(INCLUDE_NU_POSIX)) && (INCLUDE_NU_POSIX) )
#include "posix/inc/limits.h"
#else
#include <limits.h>
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++  */
#endif /* _cplusplus */

/***********************************************************************
*   Macros to negate an unsigned long, and convert to a signed long
*   and to negate an unsigned int, and convert to a signed int.
*   It is needed to prevent possible overflows for large negatives.
*   These macros should work on any form of integer representation.
************************************************************************/
#ifndef INT_MAX
#define INT_MAX ((sizeof(int) == 2) ? 32767 : 2147483647UL)
#endif

#ifndef UINT_MAX
#define UINT_MAX ((sizeof(int) == 2) ? 65535 : 4294967295UL)
#endif

#ifndef LONG_MAX
#define LONG_MAX    2147483647UL
#endif

#ifndef ULONG_MAX
#define ULONG_MAX   4294967295UL
#endif

#ifndef NCL_ITOA_LOOPS_BASE10
#define NCL_ITOA_LOOPS_BASE10 ((sizeof(int) == 2) ? 10000 : 1000000000UL)
#endif

#ifndef NCL_ITOA_LOOPS_BASE16
#define NCL_ITOA_LOOPS_BASE16 ((sizeof(int) == 2) ? 0x1000 : 0x10000000UL)
#endif

#define NCL_SNEGATE(uvalue)    ( ( uvalue <= LONG_MAX )         \
                ?  ( - (long) uvalue )                          \
                :  ( - (long)(uvalue - LONG_MAX) - LONG_MAX ) )

#define NCL_SINEGATE(uvalue)   ( ( uvalue <= INT_MAX )          \
                ?  ( - (int) uvalue )                           \
                :  ( - (int)(uvalue - INT_MAX ) - INT_MAX ) )

/**********************************************************************
* Macros to test if a character is a digit and to test if
*  a character is a space. These are used by functions within the
*  NCL.C module.
***********************************************************************/
#define NCL_IS_DIGIT(c) ( (int) (c >= '0' && c <= '9') )
#define NCL_IS_HDIGIT(c) ( (int) (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
#define NCL_IS_SPACE(c) ( (int) (c == 0x20 || (c >= 0x09 && c <= 0x0D) ))
#define NCL_IS_ASCII(c) ( (unsigned int) (c <= 0x7f) )


/**********************************************************************
* Function prototypes for the NCL.C module.
***********************************************************************/
char *NCL_Ultoa(unsigned long val, char buf[], int base);
char *NCL_Itoa(int value, char *string, int radix);
int   NCL_To_Upper(int ch);
int   NCL_Stricmp(register const char *s1, register const char *s2);
long  NCL_Atol (const char *nptr);
int   NCL_Atoi (const char *nptr);
int   NCL_Ahtoi (const char *nptr);
unsigned long NCL_Ahtol(const char *nptr);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NCL_H */
