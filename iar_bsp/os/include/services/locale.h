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
*       locale.h               
*
*   COMPONENT
*
*       PX - POSIX
*
*   DESCRIPTION
*
*       Contains the definitions for localization.
*
*   DATA STRUCTURES
*
*       NONE
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*
*************************************************************************/

#ifndef NU_PSX_LOCALE_H
#define NU_PSX_LOCALE_H

#include "services/config.h"

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _LOCALE_H_
#define _LOCALE_H_

/* Locale Conversion structure */
struct lconv
{
    /* The local currency symbol applicable to the current locale. */

    char    *currency_symbol;
    
    /* The radix character used to format non-monetary quantities.  */
    
    char    *decimal_point;
    
    /* The number of fractional digits (those after the decimal-point) to 
       be displayed in a formatted monetary quantity. */
    
    char     frac_digits;
    
    /* A string whose elements taken as one-byte integer values indicate 
       the size of each group of digits in formatted non-monetary 
       quantities. */ 
    
    char    *grouping;
    
    /* The international currency symbol applicable to the current locale.
       The first three characters contain the alphabetic international 
       currency symbol in accordance with those specified in the ISO 
       4217:2001 standard. The fourth character (immediately preceding 
       the null byte) is the character used to separate the international
       currency symbol from the monetary quantity. */
    
    char    *int_curr_symbol;
    
    /* The number of fractional digits (those after the decimal-point) to 
       be displayed in an internationally formatted monetary quantity. */
    
    char     int_frac_digits;
    
    /* Set to 1 or 0 if the int_curr_symbol respectively precedes or 
       succeeds the value for a negative internationally formatted 
       monetary quantity. */
    
    char     int_n_cs_precedes;
    
    /* Set to a value indicating the separation of the int_curr_symbol, 
       the sign string, and the value for a negative internationally 
       formatted monetary quantity. */
       
    char     int_n_sep_by_space;
    
    /* Set to a value indicating the positioning of the negative_sign for
       a negative internationally formatted monetary quantity. */
       
    char     int_n_sign_posn;
    
    /* Set to 1 or 0 if the int_curr_symbol respectively precedes or 
       succeeds the value for a non-negative internationally formatted 
      monetary quantity. */
       
    char     int_p_cs_precedes;
    
    /* Set to a value indicating the separation of the int_curr_symbol, 
       the sign string, and the value for a non-negative internationally 
       formatted monetary quantity. */
       
    char     int_p_sep_by_space;
    
    /* Set to a value indicating the positioning of the positive_sign for
       a non-negative internationally formatted monetary quantity. */
       
    char     int_p_sign_posn;
    
    /* The radix character used to format monetary quantities. */
    
    char    *mon_decimal_point;
    
    /* A string whose elements taken as one-byte integer values indicate 
       the size of each group of digits in formatted monetary 
       quantities. */
       
    char    *mon_grouping;
    
    /* The separator for groups of digits before the decimal-point in 
       formatted monetary quantities. */
       
    char    *mon_thousands_sep;
    
    /* The string used to indicate a negative valued formatted monetary 
       quantity. */
    
    char    *negative_sign;
    
    /* Set to 1 if the currency_symbol precedes the value for a negative 
       formatted monetary quantity. Set to 0 if the symbol succeeds the 
       value. */
       
    char     n_cs_precedes;
    
    /* Set to a value indicating the separation of the currency_symbol, 
       the sign string, and the value for a negative formatted monetary 
       quantity. */
       
    char     n_sep_by_space;
    
    /* Set to a value indicating the positioning of the negative_sign for
       a negative formatted monetary quantity. */
       
    char     n_sign_posn;
    
    /* The string used to indicate a non-negative valued formatted 
       monetary quantity. */
    
    char    *positive_sign;
    
    /* Set to 1 or 0 if the int_curr_symbol respectively precedes or 
       succeeds the value for a non-negative internationally formatted 
       monetary quantity. */

    char     p_cs_precedes;
    
    /* Set to a value indicating the separation of the currency_symbol, 
       the sign string, and the value for a non-negative formatted 
       monetary quantity. */
        
    char     p_sep_by_space;
    
    /* Set to a value indicating the positioning of the positive_sign for 
       a non-negative formatted monetary quantity. */

    char     p_sign_posn;
    
    /* The character used to separate groups of digits before the decimal
       -point character in formatted non-monetary quantities. */    
    
    char    *thousands_sep;

}; /* lconv */

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

/* Locale Conversion (LC) constants */

/* The value LC_ALL for category names the program's entire locale. */

#define LC_ALL                  0

/* Affects the behavior of regular expressions and the collation 
   functions. */

#define LC_COLLATE              1

/* Affects the behavior of regular expressions, character classification, 
   character conversion functions, and wide-character functions. */
   
#define LC_CTYPE                2

/* Affects what strings are expected by commands and utilities as 
   affirmative or negative responses.  It also affects what strings are 
   given by commands and utilities as affirmative or negative responses, 
   and the content of messages. */

#define LC_MESSAGES             3

/* Affects the behavior of functions that handle monetary values. */

#define LC_MONETARY             4

/* Affects the behavior of functions that handle numeric values. */

#define LC_NUMERIC              5

/* Affects the behavior of the time conversion functions. */

#define LC_TIME                 6

/* API Functions */

#ifdef __cplusplus
extern "C" {
#endif

struct  lconv *localeconv(void);
char   *setlocale(int, const char *);

#ifdef __cplusplus
}
#endif

#endif /* _LOCALE_H_ */

#endif /* NU_PSX_LOCALE_H */
