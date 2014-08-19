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
*  textd.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for textd.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _TEXTD_H_
#define _TEXTD_H_

#include "ui/rsfonts.h"

extern BOOLEAN strAsChar;

typedef enum _DrawTextType
{
    CH      = 0,
    CH16    = 1,  
    CHWIDE  = 2,
    STR     = 3,
    STR16   = 4,
    STRWIDE = 5,    
    TXT     = 6,
    TXT16   = 7,
    TXTWIDE = 8    
} DrawTextType;

/* Local Functions */
STATUS RS_Text_Draw( VOID * strings, DrawTextType  textType, INT32 index, INT32 count);

/* character mapping record */
typedef struct _charMap    
{
    /* starting Unicode character code  */
    UINT16   charStart;         

    /* ending Unicode character code    */
    UINT16   charEnd;           

    /* starting glyph index             */
    UINT16   glyphStart;        

    /* glyph row position               */
    UINT16   glyphRow;          
    
} charMap;

#ifdef USE_UNICODE

/* maximum string length for UNICODE */
#define MAXUNICHAR_STRLEN  256             

/* evaluate TRUE if val is between lo and hi inclusive. */
#define _InRange(lo, val, hi) (((lo) <= (val)) && ((val) <= (hi)))

INT32  TXTD_DrawStringW(UNICHAR *uniString, INT32 index, INT32 count, INT16 charSize);

#endif

/* _TEXTD_H_ */
#endif 





