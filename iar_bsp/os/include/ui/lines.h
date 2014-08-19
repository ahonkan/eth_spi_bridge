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
*  lines.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for lines.c
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
#ifndef _LINES_H_
#define _LINES_H_


#define     MAX_RECTS       128
#define     CAP_DASH_START  0
#define     CAP_DASH_END    1

typedef struct _lineState {
    INT32   lsDXmin;
    INT32   lsDYmin;
    INT32   lsDXmax;
    INT32   lsDYmax;
    INT32   lsErrTerm;
    INT32   lsErrTermAdjUp;
    INT32   lsErrTermAdjDown;
    INT32   lsMajorAxisLengthM1;
    INT32   lsLineDir;
    SIGNED    lsMaxRects;
    signed char lsSkipStat;
    signed char pad[3];         
} lnSt;

typedef struct _dashPoly{
    point   FirstVert[6];
    INT16   numPoints;
    INT16 pad;                  
} dashPoly;

struct _blitRcd *lclblitRec;

SIGNED    freeblitList;
SIGNED    freeblit;
blitRcd   *work_blitRcd;
UINT8     *dashListStart;
INT32     dashElements;
INT32     dashIndex;
SIGNED    dashCount;
INT32     dashFlags;
INT32     blitListPtr;
INT32     secondListPtr;
INT32     workingPnPat;
INT32     workingBkPat;
SIGNED    *blitRcdPtr;
UINT8     firstTime, lclCapStyle, dashCap;
INT32     halfWidthLeft, halfWidthRight;
INT32     halfHeightUp, halfHeightDown;
INT32     halfWidthRightRoDn; 
INT32     halfWidthRightRoUp;
INT32     halfHeightDownRoDn;
INT32     halfHeightDownRoUp;
INT32     oddPenSizeFlag;

#endif /* _LINES_H_ */




