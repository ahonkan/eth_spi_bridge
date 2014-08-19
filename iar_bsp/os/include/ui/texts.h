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
*  texts.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for texts.c
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
#ifndef _TEXTS_H_
#define _TEXTS_H_


extern INT32 SCREENI_InitBitmap( INT32 argDEVICE, grafMap *argGRAFMAP);
extern VOID  SCREENS_InitRowTable( grafMap *argBitMap, INT32 argInrLve, INT32 argInrSeg, INT32 argInrSiz);
extern INT32  BMAPF_TextBlitHorizontal( VOID *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE);
extern INT32  STRKFONT_rsStrokeFontInit( signed char *TEXT, INT32 INDEX, INT32 COUNT , INT32 CHARSIZE );
extern INT32  TXTD_DrawStringW( UNICHAR *uniString, INT32 index, INT32 count, INT16 charSize);
extern VOID  MoveTo( INT32 argX, INT32 argY);

#define     MAJORVER    0x02
#define     MINORVER    0x20

/* Local Functions */
VOID  SetFont( fontRcd *FONT);
INT32 CharWidth( signed char CHAR8);
INT32 StringWidth( UNICHAR *STRING8);
INT32 TextWidth( UNICHAR *STRING8, INT32 INDEX, INT32 COUNT);
INT32 CharWidth16( UINT16 CHAR16);
INT32 StringWidth16( UNICHAR *STRING16);
INT32 TextWidth16( UNICHAR *STRING16, INT32 INDEX, INT32 COUNT);
VOID  PolyMarker( INT16 NPts, point * XYPts );


#endif /* _TEXTS_H_ */




