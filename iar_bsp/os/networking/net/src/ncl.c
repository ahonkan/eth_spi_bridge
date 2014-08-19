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
*
*   FILE NAME
*
*       ncl.c
*
*   COMPONENT
*
*       NCL -- Nucleus Net 'C' library supplement
*
*   DESCRIPTION
*
*       This file contains 'C' library functions that are required by
*       Nucleus NET and other Nucleus networking products. These
*       functions are not supplied by all tool vendors and thus are being
*       supplied here.
*
*   FUNCTIONS
*
*       NCL_Stricmp
*       NCL_To_Upper
*       NCL_Itoa
*
*   DEPENDENCIES
*
*       ncl.h
*
*************************************************************************/

#include "networking/ncl.h"

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Stricmp
*
*   DESCRIPTION
*
*       The function compares two strings, continuing until a difference
*       is found or the end of the strings is reached. Case is ignored.
*
*   INPUTS
*
*       *s1                     Pointer to one string
*       *s2                     Pointer to the string to compare with
*
*   OUTPUTS
*
*       < 0                     s1 less than s2
*       0                       s1 identical to s2
*       > 0                     s1 greater than s2
*
*************************************************************************/
int NCL_Stricmp(register const char *s1, register const char *s2)
{
    while( (NCL_To_Upper(*s1) == NCL_To_Upper(*s2)) && (*s1) ) ++s1, ++s2;

    return ((int)(unsigned char)*s1) - ((int)(unsigned char)*s2);

} /* NCL_Stricmp */

/*************************************************************************
*
*   FUNCTION
*
*       NCL_To_Upper
*
*   DESCRIPTION
*
*       The function converts a character to upper case.
*
*   INPUTS
*
*       ch                      Character to convert
*
*   OUTPUTS
*
*       The character in uppercase.
*
*************************************************************************/
int NCL_To_Upper(int ch)
{
    if ( (ch < 'a') || (ch > 'z') )
        return (ch);

    ch -= 32;

    return (ch);

} /* NCL_To_Upper */

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Itoa
*
*   DESCRIPTION
*
*       This function converts value to a null terminated ascii string.
*
*   INPUTS
*
*       value                   The integer to convert to ASCII
*       *string                 Pointer to where the string should be built
*       radix                   Base to be used for conversion
*
*   OUTPUTS
*
*       A pointer to the converted integer
*
*************************************************************************/
char *NCL_Itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return (string);
    }

    /* If this is a decimal integer */
    if (radix == 10)
    {
        /* if this is a negative value insert the minus sign. */
        if (value < 0)
        {
            *ptr++ = '-';

            /* Make the value positive. */
            value *= -1;
        }

        for (i = (int)NCL_ITOA_LOOPS_BASE10; i > 0; i /= 10)
        {
            d = value / i;

            if (d || flag)
            {
                *ptr++ = (char)(d + 0x30);
                value -= (d * i);
                flag = 1;
            }
        }
    }

    /* If this is a hexadecimal integer */
    else if (radix == 16)
    {
        for (i = (int)NCL_ITOA_LOOPS_BASE16; i > 0; i /= 16)
        {
            d = value / i;

            if (d || flag)
            {
                /* If the value is between 0 and 9, add the proper offset */
                if ( (d >= 0) && (d <= 9) )
                    *ptr++ = (char)(d + 0x30);

                /* Otherwise, the value is between 0xa and 0xf, add the proper
                 * offset.
                 */
                else
                    *ptr++ = (char)(d + 0x57);

                value -= (d * i);
                flag = 1;
            }
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return (string);

} /* NCL_Itoa */
