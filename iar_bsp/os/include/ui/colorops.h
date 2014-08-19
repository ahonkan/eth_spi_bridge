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
*  colorops.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for colorops.c
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
#ifndef _COLOROPS_H_
#define _COLOROPS_H_

#define     MAX_DEFWIN_X    640

/* Local Functions */
INT32 FindClosestRGB(palData *RGBCOLOR, palData *RGBPALETTE);
VOID  ColorDiffusionDither(image *imgIn, palData *imgPal, image *imgOut, palData *palPtr);
VOID  QueryRes( INT32 *resX, INT32 *resY);
INT32 QueryColors(VOID);
VOID  WritePalette(INT32 plNum,INT32 bgnIdx, INT32 endIdx, palData *palPtr);
VOID  ReadPalette(INT32 plNum,INT32 bgnIdx, INT32 endIdx, palData *palPtr);
VOID  RS_Set_Alpha(double trans_level, BOOLEAN set_text_trans);
#endif /* _COLOROPS_H_ */






