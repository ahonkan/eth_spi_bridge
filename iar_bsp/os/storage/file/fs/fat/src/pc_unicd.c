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
*       pc_unicd.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       Convert unicode.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       uni2asc                             Unicode to ascii.
*       asc2uni                             Ascii to unicode.
*
*************************************************************************/

#include        "storage/fat_defs.h"


/************************************************************************
* FUNCTION
*
*       uni2asc
*
* DESCRIPTION
*
*       Each long filename entry has Unicode character strings. This
*       routine converts unicode to ascii code.
*       byte order.
*
*
* INPUTS
*
*       ptr                                 Pointer to unicode
*
* OUTPUTS
*
*       Ascii byte.
*
*************************************************************************/
UINT8 uni2asc(UINT8 *ptr)
{

    return(*ptr);
}


/************************************************************************
* FUNCTION
*
*       asc2uni
*
* DESCRIPTION
*
*       Each long filename entry has Unicode character strings. This
*       routine converts ascii to unicode.
*       byte order.
*
*
* INPUTS
*
*       ptr                                 Pointer to unicode
*       asc                                 Ascii character
*
* OUTPUTS
*
*       Ascii byte.
*
*************************************************************************/
UINT8 asc2uni(UINT8 *ptr, UINT8 ascii)
{

    *ptr = ascii;
    *(ptr+1) = 0;

    return(*ptr);
}

