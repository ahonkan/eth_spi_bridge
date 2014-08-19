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
*  xypt.h                                                       
*
* DESCRIPTION
*
*  This file contains prototypes and externs for xypt.c
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
#ifndef _XYPT_H_
#define _XYPT_H_

extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);
extern VOID VectRestore(VOID);
extern VOID VectSetup(VOID);
extern VOID POLYGONS_rsSuperSetPolyLineDrawer( point *pointsparm, INT32 npointsparm, INT32 modeparm);

extern STATUS RS_Polygon_Draw( ObjectAction action, INT32 polyPtsCount, polyHead *polyHeader,
                               point *polyPts , INT32 patt);

/* Local Functions */
VOID   AddPt( point *srcPt, point *dstPt);
VOID   SubPt( point *srcPt, point *dstPt);
VOID   SetPt( point *arg , INT32 argX, INT32 argY);
VOID   DupPt( point *srcPt , point *dstPt);
INT32  EqualPt( point * Pt1, point *Pt2);
VOID   ScalePt( point *PT, rect *srcR, rect *dstR);
VOID   MapPt( point *PT, rect *srcR , rect *dstR);
SIGNED PtToAngle( rect *R, point *PT);
INT32  PtOnPoly( INT32 rspolycnt , polyHead *rspolyhdrs, point *rspolypts);


#endif /* _XYPT_H_ */




