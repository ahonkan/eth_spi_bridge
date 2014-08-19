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
*       utl_z.c
*
* DESCRIPTION
*
*       This file contains the implementation of UTL_Zero.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UTL_Zero
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       UTL_Zero
*
*   DESCRIPTION
*
*       This function clears an area of memory to all zeros.
*
*   INPUTS
*
*       *ptr                    Pointer to starting address.
*       size                    The number of bytes of memory to zero.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID UTL_Zero(VOID *ptr, UNSIGNED size)
{
    UINT8       HUGE *byteStartP;
    UINT8       HUGE *byteEndP;
    INT         HUGE *wordStartP;
    INT         HUGE *wordEndP;

    UINT8       clearByte = 0;
    INT         clearWord = 0;

    wordStartP = (INT HUGE *)(((UNSIGNED)(ptr)) + sizeof(INT) - 1);
    wordStartP = (INT HUGE *)(((UNSIGNED)(wordStartP)) / sizeof(INT));
    wordStartP = (INT HUGE *)(((UNSIGNED)(wordStartP)) * sizeof(INT));

    byteStartP = (UINT8 HUGE *)ptr;
    byteEndP = (UINT8 HUGE *)(wordStartP);

    /* Clear leading unaligned bytes */
    while(byteStartP < byteEndP)
    {
        *byteStartP = clearByte;
        ++byteStartP;
    }

    wordEndP = (INT HUGE *)((((UNSIGNED)(ptr)) + size) / sizeof(INT));
    wordEndP = (INT HUGE *)(((UNSIGNED)(wordEndP)) * sizeof(INT));

    /* Clear the aligned bytes as entire words */
    while(wordStartP < wordEndP)
    {
        *wordStartP = clearWord;
        ++wordStartP;
    }

    /* Clear trailing unaligned bytes */
    byteStartP = (UINT8 HUGE *)(wordEndP);
    byteEndP = (UINT8 HUGE *)(((UNSIGNED)ptr) + size);

    while(byteStartP < byteEndP)
    {
        *byteStartP = clearByte;
        ++byteStartP;
    }

} /* UTL_Zero */
