/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
* (Copyright for the JSON_Decode_UTF8() function in this file)
*
* Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
*************************************************************************/

/*************************************************************************
* (Notice for the JSON_Widechar_To_Utf8() function in this file)
*
*       This code is taken from "Basic UTF-8 manipulation routines"
*       by Jeff Bezanson, placed in the public domain Fall 2005.
*       It can be obtained from the following location:
*
*           http://www.cprogramming.com/tutorial/utf8.c
*************************************************************************/

/*************************************************************************
*
* FILE NAME
*
*       json_utils.c
*
* COMPONENT
*
*       JSON
*
* DESCRIPTION
*
*       JSON related utility functions.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       JSON_Decode_Utf8
*       JSON_Widechar_To_Utf8
*       JSON_String_To_UInt64
*       JSON_String_To_Int64
*       JSON_String_To_Double
*       JSON_Double_To_String
*
* DEPENDENCIES
*
*       nu_json.h
*       json_defs.h
*       nu_kernel.h
*       sockdefs.h
*       ncl.h
*       ctype.h
*       stdlib.h
*       string.h
*       math.h
*
*************************************************************************/
#include "networking/nu_json.h"
#include "os/networking/json/json_defs.h"
#include "kernel/nu_kernel.h"
#include "os/include/networking/sockdefs.h"
#include "os/include/networking/ncl.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if CFG_NU_OS_NET_JSON_INCLUDE_PARSER

static const UINT8 JSON_Utf8d[] = {
  /* The first part of the table maps bytes to character classes that
   * to reduce the size of the transition table and create bitmasks.
   */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  /* The second part is a transition table that maps a combination
   * of a state of the automaton and a character class to a state.
   */
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};

/************************************************************************
*
*   FUNCTION
*
*       JSON_Decode_Utf8
*
*   DESCRIPTION
*
*       This routine checks that a byte is proper UTF8 format.
*
*   INPUTS
*
*       *state                  The state of the comparison.
*       *codep                  The current code point for the stream
*                               of data to which the byte being
*                               validated belongs.
*       byte                    The byte being checked.
*
*   OUTPUTS
*
*       The state of the comparison, which is the value at the offset
*       into the UTF8 array.
*
*************************************************************************/
UINT32 JSON_Decode_Utf8(UINT32 *state, UINT32 *codep, UINT32 byte)
{
  UINT32 type = JSON_Utf8d[byte];

  *codep = (*state != JSON_UTF8_ACCEPT_STATE) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = JSON_Utf8d[256 + *state + type];
  return (*state);

} /* JSON_Decode_Utf8 */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Widechar_To_Utf8
*
*   DESCRIPTION
*
*       This function converts a wide-character to a UTF-8 encoded
*       sequence of bytes.
*
*       This code is taken from "Basic UTF-8 manipulation routines"
*       by Jeff Bezanson, placed in the public domain Fall 2005.
*       It can be obtained from the following location:
*
*           http://www.cprogramming.com/tutorial/utf8.c
*
*   INPUTS
*
*       wide_char               The wide-character value.
*       *dest                   On return, this contains a sequence of
*                               1 to 4 bytes of data representing the
*                               UTF-8 encoding of the wide-character.
*                               The value is not null-terminated and
*                               this buffer must be 4 bytes in length.
*
*   OUTPUTS
*
*       The number of bytes written to the "dest" buffer.
*
****************************************************************************/
INT JSON_Widechar_To_Utf8(UINT32 wide_char, CHAR *dest)
{
    if (wide_char < 0x80) {
        dest[0] = (char)wide_char;
        return 1;
    }
    if (wide_char < 0x800) {
        dest[0] = (wide_char>>6) | 0xC0;
        dest[1] = (wide_char & 0x3F) | 0x80;
        return 2;
    }
    if (wide_char < 0x10000) {
        dest[0] = (wide_char>>12) | 0xE0;
        dest[1] = ((wide_char>>6) & 0x3F) | 0x80;
        dest[2] = (wide_char & 0x3F) | 0x80;
        return 3;
    }
    if (wide_char < 0x110000) {
        dest[0] = (wide_char>>18) | 0xF0;
        dest[1] = ((wide_char>>12) & 0x3F) | 0x80;
        dest[2] = ((wide_char>>6) & 0x3F) | 0x80;
        dest[3] = (wide_char & 0x3F) | 0x80;
        return 4;
    }
    return 0;
} /* JSON_Widechar_To_Utf8 */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_String_To_UInt64
*
*   DESCRIPTION
*
*       Converts a string to a 64-bit unsigned integer. This function
*       does not perform much sanity checks on the format of the string
*       and assumes that it is provided valid input.
*
*   INPUTS
*
*       *string                 Pointer to the string to convert.
*       *value(out)             On successful return, this contains
*                               the 64-bit string value.
*
*   OUTPUTS
*
*       A 64-bit unsigned integer value.
*
****************************************************************************/
UINT64 JSON_String_To_UInt64(CHAR *string)
{
    CHAR        *ptr = string;
    INT64       num = 0;
    
    /* Continue until the next character is a digit. */
    while (isdigit((int)(*ptr)))
    {
        num = (num * 10) + ((*ptr) - '0');
        ptr++;
    }

    return (num);
} /* JSON_String_To_UInt64 */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_String_To_Int64
*
*   DESCRIPTION
*
*       Converts a string to a 64-bit signed integer. This function
*       does not perform much sanity checks on the format of the string
*       and assumes that it is provided valid input.
*
*   INPUTS
*
*       *string                 Pointer to the string to convert.
*
*   OUTPUTS
*
*       A 64-bit signed integer value.
*
****************************************************************************/
INT64 JSON_String_To_Int64(CHAR *string)
{
    CHAR        *ptr = string;
    INT64       num = 0;
    BOOLEAN     neg = NU_FALSE;

    /* If this is a negative number. */
    if (*ptr == '-')
    {
        neg = NU_TRUE;
        ptr++;
    }

    /* Continue until the next character is a digit. */
    while (isdigit((int)(*ptr)))
    {
        num = (num * 10) + ((*ptr) - '0');
        ptr++;
    }

    if (neg)
        return (-(num));

    return (num);
} /* JSON_String_To_Int64 */

#endif /* CFG_NU_OS_NET_JSON_INCLUDE_PARSER */

#if CFG_NU_OS_NET_JSON_INCLUDE_FLOAT_CONVERSION
/**************************************************************************
*
*   FUNCTION
*
*       JSON_String_To_Double
*
*   DESCRIPTION
*
*       Converts a string into a double data type.
*
*   INPUTS
*
*       *str                    Number to be converted in string format.
*       *num(out)               On successful return, this contains the
*                               converted number in double format.
*
*   OUTPUTS
*
*       NU_SUCCESS              Conversion was successful.
*       NU_INVALID_POINTER      One of the parameters was not valid.
*
****************************************************************************/
STATUS JSON_String_To_Double(CHAR *str, double *num)
{
    INT     sign = 1;
    double  result = 0.0;
    double  fraction = 0.0;
    CHAR    *end_of_frac;
    INT     exponent = 0;

    /* Make sure the parameters are valid. */
    if ((str == NU_NULL) || (num == NU_NULL))
    {
        return NU_INVALID_POINTER;
    }

    /* Check for a negative sign. */
    if (*str == '-')
    {
        sign = -1;
        str++;
    }

    /* Process all numbers before the decimal sign. */
    while (isdigit((int)*str))
    {
        result = (result * 10) + ((*str) - '0');
        str++;
    }

    /* If the decimal is encountered. */
    if (*str == '.')
    {
        str++;
        /* Locate the point where this fractional part ends. */
        while (isdigit((int)*str))
            str++;
        end_of_frac = str - 1;

        /* Process all numbers after the decimal sign. */
        while (isdigit((int)*end_of_frac))
        {
            fraction = (fraction / 10) + ((*end_of_frac) - '0');
            end_of_frac--;
        }
        fraction = fraction / 10;
    }

    /* If there is an exponent part. */
    if ((*str == 'e') || (*str == 'E'))
    {
        str++;
        exponent = atoi(str);
    }

    /* Store the result. */
    *num = (result + fraction) * pow(10, exponent) * sign;

    return NU_SUCCESS;
} /* JSON_String_To_Double */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Double_To_String
*
*   DESCRIPTION
*
*       Converts a double number into a string representation.
*
*   INPUTS
*
*       num                     Number to be converted to string format.
*       *result(out)            This specifies the buffer and its length
*                               allocated by the caller. On successful
*                               return, this contains the converted
*                               number in string format.
*
*   OUTPUTS
*
*       NU_SUCCESS              Conversion was successful.
*       NU_INVALID_POINTER      One of the parameters was not valid.
*       NUF_FLOAT_LENGTH_EXCEEDED Buffer for storing the number is not
*                               large enough.
*
****************************************************************************/
STATUS JSON_Double_To_String(double num, JSON_STRING *result, INT decimal_places)
{
    INT     exponent;
    double  int_part;
    double  frac_part;
    BOOLEAN is_negative = NU_FALSE;
    CHAR    *str;
    INT     i;

    /* Make sure the parameters are valid. */
    if ((result == NU_NULL) || (result->str == NU_NULL))
    {
        return NU_INVALID_POINTER;
    }
    else if (result->length <= 8 + decimal_places)
    {
        return NUF_FLOAT_LENGTH_EXCEEDED;
    }
    else if (decimal_places <= 0)
    {
        return NU_INVALID_PARM;
    }

    /* Check if the number is negative. */
    if (num < 0)
    {
        is_negative = NU_TRUE;
        num = (-num);
    }
    else if (num == 0.0)
    {
        strcpy(result->str, "0.0");
        result->length = strlen("0.0");
        return NU_SUCCESS;
    }

    /* Calculate the exponent and adjust number for this exponent. */
    exponent = (INT)log10(num);
    num = num / pow(10, exponent);

    /* Now extract the integer and fractional parts from the number. */
    frac_part = modf(num, &int_part);

    /* Write the initial part of the string. */
    str = result->str;
    if (is_negative)
        *str++ = '-';
    /* The "int_part" will be a single digit so directly convert it. */
    *str++ = ((CHAR)int_part) + '0';
    *str++ = '.';

    /* Now write the fractional part to the string. */
    for (i = 0; i < decimal_places; i++)
    {
        /* Extract the next digit from the fractional part. */
        frac_part = frac_part * 10;
        int_part = fmod(frac_part, 10);
        modf(int_part, &int_part);
        *str++ = ((CHAR)int_part) + '0';
    }

    /* Null-terminate the string. */
    *str = '\0';

    /* Now write the exponent if non-zero. */
    if (exponent != 0)
    {
        *str++ = 'e';
        NCL_Itoa(exponent, str, 10);
    }
    /* Set the string's length. */
    result->length = strlen(result->str);

    return NU_SUCCESS;
} /* JSON_Double_To_String */

#endif /* CFG_NU_OS_NET_JSON_INCLUDE_FLOAT_CONVERSION */
