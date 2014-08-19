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
*  line.h                                                       
*
* DESCRIPTION
*
*  This file contains prototypes and externs for line.c
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
#ifndef _LINE_H_
#define _LINE_H_


extern VOID   U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
extern SIGNED PtToAngle(rect * R, point * PT);
extern VOID   OvalPt(rect *argR, INT32 rsangle, point *gblPt);
extern VOID   (*lineExecPntr)();
extern VOID   FillPolygon(point *pointsparm, INT32 npointsparm, INT32 modeparm, INT32 shape);

/* Local Functions */
VOID MoveTo( INT32 argX, INT32 argY);
VOID MoveRel(INT32 argDX, INT32 argDY );
VOID LineTo( INT32 valX, INT32 valY);
VOID LineRel(INT32 valDX, INT32 valDY );
VOID PolySegment( INT16 SEGCNT, segPts *SEGLIST );
VOID LINE_rsOvalPolyLines(INT16 numpoints, point *points);


#endif /* _LINE_H_ */






