/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       util_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Util
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains definition for utility routines
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef UTIL_DEFS_H
#define UTIL_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#define NUF_ATOI        NUF_Atoi

/***********************************************************************
*   Macros to negate an unsigned long, and convert to a signed long 
*   and to negate an unsigned int, and convert to a signed int.     
*   It is needed to prevent possible overflows for large negatives. 
*   These macros should work on any form of integer representation. 
************************************************************************/
#ifndef NUF_INT_MAX
#define NUF_INT_MAX ((sizeof(int) == 2) ? 32767 : 2147483647UL)
#endif

#ifndef NUF_UINT_MAX
#define NUF_UINT_MAX ((sizeof(int) == 2) ? 65535 : 4294967295UL)    
#endif    
    
#ifndef NUF_LONG_MAX
#define NUF_LONG_MAX    2147483647UL
#endif
    
#ifndef NUF_ULONG_MAX
#define NUF_ULONG_MAX   4294967295UL
#endif

#ifndef NUF_ITOA_LOOPS_BASE10
#define NUF_ITOA_LOOPS_BASE10 ((sizeof(int) == 2) ? 10000 : 1000000000UL)
#endif

#ifndef NUF_ITOA_LOOPS_BASE16
#define NUF_ITOA_LOOPS_BASE16 ((sizeof(int) == 2) ? 0x1000 : 0x10000000UL)
#endif

#define NUF_SNEGATE(uvalue)    ( ( uvalue <= NUF_LONG_MAX )         \
                ?  ( - (long) uvalue )                          \
                :  ( - (long)(uvalue - NUF_LONG_MAX) - NUF_LONG_MAX ) )

#define NUF_SINEGATE(uvalue)   ( ( uvalue <= NUF_INT_MAX )          \
                ?  ( - (int) uvalue )                           \
                :  ( - (int)(uvalue - NUF_INT_MAX ) - NUF_INT_MAX ) )

/**********************************************************************
* Macros to test if a character is a digit and to test if
*  a character is a space. These are used by functions within the
*  util.c module.
***********************************************************************/
#define NUF_IS_DIGIT(c) ( (int) (c >= '0' && c <= '9') )
#define NUF_IS_HDIGIT(c) ( (int) (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define NUF_IS_SPACE(c) ( (int) (c == 0x20 || (c >= 0x09 && c <= 0x0D) ))
#define NUF_IS_ASCII(c) ( (unsigned int) (c <= 0x7f) )

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* UTIL_DEFS_H */
