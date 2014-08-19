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
*  bmapfont.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes,externs, and defines for bitmap fonts.
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
#ifndef _BMAPFONT_H_
#define _BMAPFONT_H_

extern VOID  G2UP(INT32 GloblX, INT32 GloblY, INT32 *RtnX, INT32 *RtnY);
extern INT32 QueryColors(VOID);
extern VOID  ReadPalette(INT32 plNum,INT32 bgnIdx, INT32 endIdx, palData *palPtr);
extern INT32 FindClosestRGB(palData *RGBCOLOR, palData *RGBPALETTE);


/* Local Functions */
INT32 BMAPF_TextBlitHorizontal (VOID *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE); 
INT32 BMAPF_TextAlignHorizontalCR (signed char *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE); 

#define maxChars    128 /* Maximum length of input string  */


#endif /* _BMAPFONT_H_ */





