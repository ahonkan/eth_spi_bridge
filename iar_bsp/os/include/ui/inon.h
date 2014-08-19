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
*  inon.h                                                       
*
* DESCRIPTION
*
*  This file contains prototypes and externs for inon.c
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
#ifndef _INON_H_
#define _INON_H_


extern VOID INON_rsRectInList(blitRcd *FillRcd);
extern VOID MoveTo(INT32 argX, INT32 argY);
extern VOID LineTo(INT32 valX, INT32 valY);
extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);

/* Local Functions */
INT32  PtInRect(point *fpTESTPT, rect *argRect, INT32 sizX, INT32 sizY);
INT32  XYInRect(INT32 valX, INT32 valY, rect *varR);
VOID   VectSetup(VOID);
VOID   VectRestore(VOID);
INT32  PtInOval(point *fpTESTPT, rect *fpRect, INT32 sizX, INT32 sizY);
INT32  PtInRoundRect(point *fpTESTPT, rect *fpRect, INT32 diaX, INT32 diaY,
                     INT32 sizX, INT32 sizY);
INT32  PtInArc(point *fpTESTPT, rect *fpRect, INT16 bgn, INT16 arcangle,
               INT32 sizX, INT32 sizY);
INT32  PtInPoly(point *fpTESTPT, INT32 fpPOLYCNT, polyHead *fpPOLYHDR,
                point *fpPOLYPTS, INT32 sizX, INT32 sizY);
VOID   LSPC_rsSpecialLinePatternAndSquare(blitRcd *LINEREC);
INT32  PtOnLine(point *fpTESTPT, point *PT1, point *PT2, INT32 sizX, INT32 sizY);
INT32  PtOnRect(point *fpTESTPT, rect *fpRect, INT32 sizX, INT32 sizY);
INT32  PtOnOval(point *fpTESTPT, rect *fpRect, INT32 sizX, INT32 sizY);
INT32  PtOnRoundRect(point *fpTESTPT, rect *fpRect, INT32 diaX, INT32 diaY,
                     INT32 sizX, INT32 sizY);
INT32  PtOnArc(point *fpTESTPT, rect *fpRect, INT16 bgnANG, INT16 arcANG,
               INT32 sizX, INT32 sizY);
INT32  PtInFlood(point *fpTESTPT, point *seedPt, rect *fpRect, INT32 sizX, INT32 sizY);
INT32  PtInBoundary(point *fpTESTPT, point *seedPt, INT32 fpBCOLOR, rect *fpRect,
                    INT32 sizX, INT32 sizY);
VOID   INON_rsRectInList(blitRcd *FillRcd);
INT32  PtInRegion( point *pt, region *rgn, INT32 pxwid, INT32 pxht);
INT32  RectInRegion( rect *r , region *rgn);


#endif /* _INON_H_ */






