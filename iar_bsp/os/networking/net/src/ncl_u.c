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
*       ncl_u.c
*
* DESCRIPTION
*
*       This file contains the implementation of NCL_Ultoa.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NCL_Ultoa
*
* DEPENDENCIES
*
*       None.
*
************************************************************************/

/*************************************************************************
*
*   FUNCTION
*
*       NCL_Ultoa
*
*   DESCRIPTION
*
*       Binary-to-ascii conversion for an unsigned long.  Returns a
*       pointer to a string built somewhere in "buf".  This routine will
*       handle bases from 8 to 16.
*
*   INPUTS
*
*       value                   The integer to convert to ASCII
*       *string                 Pointer to where the string should be built
*       radix                   Base to be used for conversion
*
*   OUTPUTS
*
*       A pointer to the converted long
*
*************************************************************************/
char *NCL_Ultoa(unsigned long value, char *string, int radix)
{
    unsigned long   i, d;
    int             flag = 0;
    char            *ptr = string;

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return (string);
    }

    if (radix == 10)
    {
        for (i = 1000000000UL; i > 0; i /= 10)
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

    else if (radix == 16)
    {
        for (i = 0x10000000UL; i > 0; i /= 16)
        {
            d = value / i;

            if (d || flag)
            {
                /* If the value is between 0 and 9, add the proper offset */
                if (d <= 9)
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

} /* NCL_Ultoa */
