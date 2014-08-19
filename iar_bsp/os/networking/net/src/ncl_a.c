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

/*************************************************************************
* FILE NAME
*
*       ncl_a.c
*
* DESCRIPTION
*
*       This file contains ASCII-to-integer routines.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NCL_Atoi
*       NCL_Ahtoi
*       NCL_Ahtol
*       NCL_Atol
*
* DEPENDENCIES
*
*       ncl.h
*
************************************************************************/

#include "networking/ncl.h"

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Atoi
*
*   DESCRIPTION
*
*       Converts an ASCII number to the integer equivalent.
*
*   INPUTS
*
*       *nptr                   Pointer to the ASCII number to convert
*
*   OUTPUTS
*
*       The ASCII number in integer form.
*
*************************************************************************/
int NCL_Atoi(const char *nptr)
{
    register const unsigned char *ptr = (const unsigned char *)nptr;
    register       unsigned int   num = 0;
    register                int   c   = *ptr;
    register                int   neg = 0;

    while ( NCL_IS_SPACE(c) )  c = *++ptr;   /* skip over whitespace chars */

    if ( c == '-' )                     /* get an optional sign */
    {
      neg = 1;
      c = *++ptr;
    }
    else if ( c == '+' )  c = *++ptr;

    while ( NCL_IS_DIGIT(c) )
    {
      num = ( 10 * num ) + (unsigned int)( c - '0' );
      c = *++ptr;
    }

    if ( neg )  return ( (int)NCL_SINEGATE(num) );

    return ( (int) num );

} /* NCL_Atoi */

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Ahtoi
*
*   DESCRIPTION
*
*       Converts an ASCII hexadecimal number to the integer equivalent.
*
*   INPUTS
*
*       *nptr                   Pointer to the ASCII hexadecimal number
*                               to convert
*
*   OUTPUTS
*
*        The ASCII number in integer form.
*        -1 for an invalid hexadecimal input.
*
*************************************************************************/
int NCL_Ahtoi(const char *nptr)
{
    register const unsigned char  *ptr = (const unsigned char *)nptr;
    register       unsigned int   num = 0;
    register                int   c   = *ptr;

    while (NCL_IS_SPACE(c))  c = *++ptr;   /* skip over whitespace chars */

    while (c)
    {
        if (NCL_IS_HDIGIT(c))
        {
            if ( (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
                num = (16 * num) + (NCL_To_Upper(c) - '7');

            else
                num = (16 * num) + (c - '0');

            c = *++ptr;
        }
        else
        {
            return -1;
        }
    }

    return ( (int) num );

} /* NCL_Ahtoi */

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Ahtol
*
*   DESCRIPTION
*
*       Converts an ASCII hexadecimal number to the unsigned long equivalent.
*
*   INPUTS
*
*       *nptr                   Pointer to the ASCII hexadecimal number
*                               to convert
*
*   OUTPUTS
*
*        The ASCII number in long form.
*        0 for an invalid hexadecimal input.
*
*************************************************************************/
unsigned long NCL_Ahtol(const char *nptr)
{
    register const unsigned char  *ptr = (const unsigned char *)nptr;
    register       unsigned long   num = 0;
    register                int   c   = *ptr;

    while (NCL_IS_SPACE(c))  c = *++ptr;   /* skip over whitespace chars */

    while (c)
    {
        if (NCL_IS_HDIGIT(c))
        {
            if ( (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
                num = (16 * num) + (NCL_To_Upper(c) - '7');

            else
                num = (16 * num) + (c - '0');

            c = *++ptr;
        }
        else
        {
            num = 0;
            break;
        }
    }

    return ( (unsigned long) num );

} /* NCL_Ahtol */

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Atol
*
*   DESCRIPTION
*
*       Converts an ASCII number to the long integer equivalent.
*
*   INPUTS
*
*       *nptr                   Pointer to the ASCII number to convert
*
*   OUTPUTS
*
*       The ASCII number in long integer form.
*
*************************************************************************/
long NCL_Atol(const char *nptr)
{
    register const unsigned char *ptr = (const unsigned char *)nptr;
    register       unsigned long  num = 0;
    register                int   c   = *ptr;
    register                int   neg = 0;

    while ( NCL_IS_SPACE(c) )  c = *++ptr;   /* skip over whitespace chars */

    if ( c == '-' )                     /* get an optional sign */
    {
      neg = 1;
      c = *++ptr;
    }
    else if ( c == '+' )  c = *++ptr;

    while ( NCL_IS_DIGIT(c) )
    {
      num = ( 10 * num ) + (unsigned int)( c - '0' );
      c = *++ptr;
    }

    if ( neg )  return ( (long)NCL_SNEGATE(num) );

    return ( (long) num );

} /* NCL_Atol */
