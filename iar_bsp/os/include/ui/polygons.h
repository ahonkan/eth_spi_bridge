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
*  polygons.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for polygons.c
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
#ifndef _POLYGONS_H_
#define _POLYGONS_H_

/* Global Variables */

#define halfWidthLeft   halfWidth[0];
#define halfWidthRight  halfWidth[1];
#define halfHeightUp    halfWidth[2];
#define halfHeightDown  halfWidth[3];

#define VerticalRestart 1
#define DiagonalRestart 2
#define XMajorRestart   3
#define YMajorRestart   4

#define PEN_LEFT    0
#define PEN_RIGHT   1
#define PEN_UP      2
#define PEN_DOWN    3

extern VOID U2GP( INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
extern VOID G2UR( rect UserRect, rect *RtnRect);
extern VOID MapPt( point * PT, rect * srcR , rect * dstR );

/* Structure used to store the state of suspended edge tracing and later
restore it. */
typedef struct _restartStruc
{
    /* if this is the first time scanning an edge in the restartable scanning code, 0 else */
    UINT8 scanEdgeFirstPass;  
    UINT8 filler[3];
    
    /* case vector to restart routine */
    SIGNED restartVector;     

    /* # of scan lines left in suspended edge */
    SIGNED restartRemaining;  

    /* various registers and variables saved to */
    SIGNED restartAE;         

    /*  restart various routines */
    SIGNED restartCE;         
    SIGNED restartDE;

    SIGNED restartSN;
    SIGNED restartBR;

    SIGNED restartAdvanceAmt;
    SIGNED restartHeight;
} restartStruc;

/* Local Functions */
VOID FillPolygon( point *pointsparm, INT32 npointsparm, INT32 modeparm, INT32 shape);

VOID POLYGONS_rsScanAndDrawConvex( UINT8 *lclScratchBufferPtr, SIGNED lclScratchBufferSize,
                                   point * points, INT32 npoints, INT32 mode,
                                   SIGNED lclXAdjust, SIGNED lclYAdjust);

VOID POLYGONS_rsSuperSetPolyLineDrawer( point *pointsparm, INT32 npointsparm, INT32 modeparm);

VOID OffsetPoly( INT32 POLYCNT, polyHead *POLYHDR, point *POLYPTS, INT32 tDX, INT32 tDY);
VOID MapPoly( INT32 POLYCNT, polyHead *POLYHDR, point *POLYPTS, rect *srcR, rect *dstR);
VOID FillRule( INT32 POLYRULE);


#endif /* _POLYGONS_H_ */


