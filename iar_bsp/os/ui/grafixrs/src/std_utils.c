/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  std_utils.c                                                  
*
* DESCRIPTION
*
*  This file contains the GRAFIX string utility functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  STD_a_toi
*  STD_abs_val
*  STD_i_toa
*  STD_is_alnum
*  STD_is_alpha
*  STD_is_digit
*  STD_is_space
*  STD_l_toa
*  STD_number_to_a_worker
*  
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  std_utils.h
*  str_utils.h
*
***************************************************************************/

#include "ui/rs_base.h" 
#include "ui/rsfonts.h"
#include "ui/std_utils.h" 
#include "ui/str_utils.h"

#define _INTBITS    32 
#define INT_MAX     ~(~0<<(_INTBITS-1))
#define SINEGATE(uvalue)   ( ( uvalue <= INT_MAX )                \
                ?  ( - (INT32) uvalue )                 \
                :  ( - (INT32)(uvalue-INT_MAX ) - INT_MAX ) )

/***************************************************************************
* FUNCTION
*
*    STD_a_toi
*
* DESCRIPTION
*
*    Function STD_a_toi is the re-entrant atoi function.
*
* INPUTS
*
*    const UNICHAR *nptr - Input ASCII string to convert.
*
* OUTPUTS
*
*    INT32 - Integer value
*
***************************************************************************/
INT32 STD_a_toi(const UNICHAR *nptr)
{   
    const UNICHAR *ptr = (const UNICHAR *)nptr;
    INT32       c = *ptr;
    INT32       neg = 0;
    INT32       ret_val = 0;
    UINT8       done = NU_FALSE;
    UINT32      num = 0;

    while( STD_is_space(c) )
    {
        /* skip over whitespace chars */
        c = *++ptr; 
    }

    /* get an optional sign */
    if( c == '-' ) 
    {   
        neg = 1;
        c = *++ptr;
    } 
    else if( c == '+' )
    {
        c = *++ptr;
    }

    while( STD_is_digit(c) )
    {
        num = ( 10 * num ) + ( c - '0' );
        c = *++ptr;
    }

    if ( neg )
    {
        ret_val = ( SINEGATE(num) );
        done = NU_TRUE;
    }

    if ( !done )
    {
        ret_val = (INT32) num;
    }

    return ret_val;
}

/***************************************************************************
* FUNCTION
*
*    STD_abs_val
*
* DESCRIPTION
*
*    Function STD_abs_val is the re-entrant abs function.
*
* INPUTS
*
*    INT32 val - value to find absolute value of.
*
* OUTPUTS
*
*    INT32 - absolute value of val.
*
***************************************************************************/
INT32 STD_abs_val(INT32 val)
{
    /* Is the value passed in less than zero? */
    if (val < 0)
    {
        /* It is, so make it positive */
        val = -val;
    }

    /* return the absolute value */    
    return val;
}

/***************************************************************************
* FUNCTION
*
*    STD_i_toa
*
* DESCRIPTION
*
*    Function STD_i_toa is the re-entrant itoa function. Only Supports base10 at
*    this time.
*
* INPUTS
*
*    INT32 val    - Integer to convert.
*    UNICHAR *str - Input ASCII string.
*    INT32 radix  - radix base 10 only.
*
* OUTPUTS
*
*    UNICHAR - ASCII string.
*
***************************************************************************/
UNICHAR *STD_i_toa(INT32 value, UNICHAR *string, INT32 radix)
{
    UINT32          i, d;
    INT             flag = 0;
    UNICHAR         *ptr = string;

    switch( radix )
    {
    case 10:

        /* Check if value is 0 */
        if (!value)
        {
            /* Set string value to ascii 0 */
            *ptr = '0';
            ptr++;
        }
        else
        {
            /* Start with billions and work down
               to tens column */
            for (i = 1000000000UL; i > 0; i /= 10)
            {
                /* Isolate correct column */
                d = value / i;
    
                /* Check if isolated column is non-zero or if
                   a non-zero column has been previously
                   calculated */
                if (d || flag)
                {
                    /* Set ascii value in string and
                       move pointer to next character */
                    *ptr = (CHAR)(d + '0');
                    ptr++;
    
                    /* Remove column from value being
                       converted */
                    value -= (d * i);
    
                    /* Set flag to show a non-zero column
                       has been calculated - all future
                       columns are added to string */
                    flag = 1;
    
                }   /* if (d || flag) */
    
            }   /* for */
    
        } /* if (!value) */
    
        /* Null terminate the string. */
        *ptr = 0;
        break;
    
    default:
        /* Return empty string. No support for radices other than 10 
           yet. */
        *string = 0;
        break;
    }
    
    /* Return ascii string to caller */
    return  (string);    
}

/***************************************************************************
* FUNCTION
*
*    STD_is_alnum
*
* DESCRIPTION
*
*    Function STD_is_alnum is the re-entrant isalnum function.
*
* INPUTS
*
*    INT32 c - ASCII value of character to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 if a number or letter.
*
***************************************************************************/
INT32 STD_is_alnum(INT32 c)
{
    return (c > 0x20);
}

/***************************************************************************
* FUNCTION
*
*    STD_is_alpha
*
* DESCRIPTION
*
*    Function STD_is_alpha is the re-entrant isalpha function.
*
* INPUTS
*
*    INT32 c - ASCII value of character to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 if a letter.
*
***************************************************************************/
INT32 STD_is_alpha(INT32 c)
{
    return (   ((c >= 'A') && (c <= 'Z')) ||
                ((c >= 'a') && (c <= 'z')) 
           );
}

/***************************************************************************
* FUNCTION
*
*    STD_is_digit
*
* DESCRIPTION
*
*    Function STD_is_digit is the re-entrant isdigit function.
*
* INPUTS
*
*    INT32 c - ASCII value of character to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 if digit.
*
***************************************************************************/
INT32 STD_is_digit(INT32 c)
{
    return ((c >= '0') && (c <= '9'));
}

/***************************************************************************
* FUNCTION
*
*    STD_is_space
*
* DESCRIPTION
*
*    Function STD_is_space is the re-entrant isspace function.
*
* INPUTS
*
*    INT32 c - ASCII value of character to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 if a space.
*
***************************************************************************/
INT32 STD_is_space(INT32 c)
{
    return c == 0x20 || (c >= 0x09 && c <= 0x0D); 
}

/*************************************************************************
*
*   FUNCTION
*
*       STD_l_toa
*
*   DESCRIPTION
*
*       Return the ascii representation of a long number.
*
*   INPUTS
*
*       value               Value to convert.
*       destination         Character in question.
*       radix               Radix to convert to.
*
*   OUTPUTS
*
*       char*               returns pointer to string representing value
*
*************************************************************************/

UNICHAR* STD_l_toa(INT32 value, UNICHAR* destination, INT32 radix)
{
    /* Do we have a negative number? */

    if ((radix == 10) && (value < 0))
    {
        /* ... yes.  */
        STD_number_to_a_worker((UINT32)value, destination, radix, NU_TRUE);
    }
    else
    {
        /* ...no. */
        STD_number_to_a_worker((UINT32)value, destination, radix, NU_FALSE);
    }

    /* Return pointer to beginning of the stuffed string. */
    return(destination);
}

/***************************************************************************
* FUNCTION
*
*    STD_number_to_a_worker
*
* DESCRIPTION
*
*    Function STD_number_to_a_worker is the re-entrant STD_number_to_a_worker 
*    function.
*
* INPUTS
*
*    UINT32     value
*    UINT8      *destination
*    INT32      radix
*    BOOLEAN    isNegative
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID STD_number_to_a_worker(UINT32 value, UNICHAR *destination, INT32 radix, 
                     BOOLEAN isNegative)
{
    UNICHAR* firstDigitLocation = destination;
    UNICHAR  swap;

    /* Is the value negative? */
    if(isNegative)
    {
        /* ... yes. Output the minus sign. */
        *destination++ = '-';

        /* Take the absolute value of the value for the rest of the algorithm. */
        value = ((UINT32) ((INT32)(((INT32)0) - ((INT32)value))));
    }
    /* We will build the number in the reverse order, then swap at end. */
    /* So, we need to save a pointer to the first digit's location. */

    do
    {
        /* Get the remainder after dividing by the radix. */

        unsigned remainder = (unsigned)(value % radix);

        /* Get 'rid' of the remainder, getting the next digit. */

        value /= radix;

        /* Convert the remainder to ascii and store it in the destination. */
        /* Is the remainder a 'digit' (non decimal)? */

        if( remainder > 9 )
        {
            /* ... yes. Store it as a letter. */
            *destination++ = (UINT8)(remainder - 10 + 'a');
        }
        else
        {
            /* ... no. Store it as a number. */
            *destination++ = (UINT8)(remainder + '0');
        }

        /* Continue to do the above algorithm while the value is positive. */

    } while(value > 0);

    /* We now have the number in the reverse direction, swap it around. */
    /* While we are at the end of the string, terminate it and back up. */
    *destination-- = '\0';

    do
    {
        /* Continue to swap until we are at the first digit location.  */
        /* While we swap, back up the destination pointer and advance  */
        /* the first digit location. We will essentially meet halfway. */

        swap = *destination;

        *destination--          = *firstDigitLocation;
        *firstDigitLocation++   = swap;

    } while(firstDigitLocation < destination);
}
