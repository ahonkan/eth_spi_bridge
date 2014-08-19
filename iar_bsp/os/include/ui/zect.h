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
*  zect.h                                                       
*
* DESCRIPTION
*
*  This file contains prototypes and externs for zect.c
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
#ifndef _ZECT_H_
#define _ZECT_H_

extern VOID MapPt(point *PT, rect *srcR , rect *dstR );
extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);
extern VOID V2GSIZE(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);

/* Local Functions */
VOID  SetRect( rect *R, INT32 X1, INT32 Y1, INT32 X2, INT32 Y2);
VOID  DupRect( rect *srcR, rect *dstR);
INT32 EqualRect( rect *Rect1, rect *Rect2);
VOID  MapRect( rect *R, rect *srcR, rect *dstR);
VOID  InsetRect( rect *R, INT32 dltX, INT32 dltY);
VOID  OffsetRect( rect *R, INT32 dltX, INT32 dltY);
INT32 INCEPT( INT32 Min1, INT32 Max1, INT32 Min2, INT32 Max2 , rect *dstR , INT32 test);
INT32 InceptRect( rect *Rect1, rect *Rect2, rect *dstR);
VOID  UnionRect( rect *Rect1, rect *Rect2, rect *dstR);
VOID  CenterRect( point *PT, INT32 widthX, INT32 heightY, rect *dstR);
VOID  Pt2Rect( point *PT1, point *PT2, rect *R);
INT32 ShiftRect( rect *R, INT32 dltX, INT32 dltY, rect *Rect1, rect *Rect2);
VOID  ScrollRect( rect *areaR, INT32 valDX, INT32 valDY);
VOID  ScreenRect(rect *SCRNRECT);
VOID  NullRect(rect *argR);
INT32 EmptyRect(rect *argR);
VOID  InitRect(rect *r, INT32 xStart, INT32 yStart, INT32 width, INT32 height);


#endif /* _ZECT_H_ */




