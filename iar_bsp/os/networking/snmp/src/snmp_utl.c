/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_utl.c                                                    
*
*   DESCRIPTION
*
*       This file contains general utility functions used by SNMP.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       UTL_Hex_To_Char
*       UTL_Admin_String_Cmp
*
*   DEPENDENCIES
*
*       utl.h
*
*************************************************************************/

#include "networking/snmp_utl.h"

/************************************************************************
*
*   FUNCTION
*
*       UTL_Hex_To_Char
*
*   DESCRIPTION
*
*       This function converts a Hexadecimal number to a Character.
*
*   INPUTS
*
*       hex_num         A hexadecimal number.
*
*   OUTPUTS
*
*       Character equivalent for the hexadecimal number
*
*************************************************************************/
CHAR UTL_Hex_To_Char(UINT8 hex_num)
{
    if(hex_num > 0x0F)
        return ('0');

    else if(hex_num <= 9)
        return ((CHAR)(0x30 + ((UINT8)hex_num)));

    else
        return ((CHAR)(0x37 + ((UINT8)hex_num)));

} /* UTL_Hex_To_Char */

/************************************************************************
*
*   FUNCTION
*
*       UTL_Admin_String_Cmp
*
*   DESCRIPTION
*
*       This function is used to compare two admin strings. This
*       comparison is slightly different from a conventional string
*       comparison. The length of the string plays a bigger role
*       in determining which string is greater. For example, the
*       string "abraham" is greater than the string "john". Although,
*       "john" is lexicographically greater. This is because "abraham"
*       has a bigger length.
*
*   INPUTS
*
*       *left_side              String to be compared.
*       *right_side             String to be compared.
*
*   OUTPUTS
*
*       < 0                     Left side is smaller than right side.
*       0                       Both strings are equal.
*       > 0                     Left side is greater than right side.
*
*************************************************************************/
INT UTL_Admin_String_Cmp(const CHAR *left_side, const CHAR *right_side)
{
    INT             cmp;

    /* Calculate the difference in string lengths of the passed strings.
     */
    cmp = (INT)strlen(left_side) - (INT)strlen(right_side);

    /* If the lengths are equal, then compare the actual strings. */
    if(cmp == 0)
    {
        cmp = strcmp(left_side, right_side);
    }

    /* Return result of comparison. */
    return (cmp);

} /* UTL_Admin_String_Cmp */


