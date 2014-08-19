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
*  arcsd.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes,externs, and defines for arcs drawing.
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
#ifndef _ARCSD_H_
#define _ARCSD_H_

extern VOID OvalPt(rect *argR, INT32 rsangle, point *gblPt);
extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);


/* Local Functions */
VOID ARCSD_ArcDash( rect *bound_rect, INT16 bgn_angle, INT16 arc_angle);


#define VT      0    
#define LN      0xff 
#define TEND    0xff 
#define DOWN    1    
#define UP      0xff 
#define NA      0
#define AETYPE  0    
#define AEBGNX  1    
#define AEBGNY  2    
#define AEENDX  3    
#define AEENDY  4    
#define AEDIR   5    
#define AEEND   6    
#define AXMIN   0
#define AYMIN   1
#define AXMAX   2
#define AYMAX   3
#define GXMIN   4
#define GYMIN   5
#define GXMAX   6
#define GYMAX   7
#define BGNX    8
#define BGNY    9
#define ENDX    10
#define ENDY    11

static UINT8 type_tbl[20][5][7] = {
/* type00 */
{{VT, GXMAX,  ENDY,    NA,  BGNY, DOWN,    0},
{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type01 */
{{VT, GXMAX, GYMIN,    NA,  BGNY, DOWN,    0},
{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY,   UP,    0},
{VT, GXMIN, GYMIN,    NA,  ENDY,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type02 */
{{VT, GXMAX, GYMIN,    NA,  BGNY, DOWN,    0},
{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY, DOWN,    0},
{VT, GXMIN, GYMIN,    NA,  ENDY,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type03 */
{{VT, GXMAX, GYMIN,    NA,  BGNY, DOWN,    0},
{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY, DOWN,    0},
{VT, GXMAX,  ENDY,    NA, GYMAX, DOWN,    0},
{VT, GXMIN, GYMIN,    NA, GYMAX,   UP, TEND}},
/* type04 */
{{VT, GXMAX, GYMIN,    NA,  BGNY, DOWN,    0},
{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY,   UP,    0},
{VT, GXMAX,  ENDY,    NA, GYMAX, DOWN,    0},
{VT, GXMIN, GYMIN,    NA, GYMAX,   UP, TEND}},
/* type05 */
{{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY,   UP,    0},
{VT, GXMIN,  BGNY,    NA,  ENDY,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type06 */
{{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY, DOWN,    0},
{VT, GXMIN,  BGNY,    NA,  ENDY,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type07 */
{{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY, DOWN,    0},
{VT, GXMAX,  ENDY,    NA, GYMAX, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type08 */
{{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY,   UP,    0},
{VT, GXMAX,  ENDY,    NA, GYMAX, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type09 */                                 
{{VT, GXMAX, GYMIN,    NA, GYMAX, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX,   UP,    0},
{LN,  BGNX,  BGNY, AXMAX, AYMAX, DOWN,    0},
{LN, AXMAX, AYMAX,  ENDX,  ENDY,   UP,    0},
{VT, GXMIN, GYMIN,    NA,  ENDY,   UP, TEND}},
/* type10 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMIN,  BGNY,    NA,  ENDY, DOWN,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type11 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX, DOWN,    0},
{VT, GXMAX,  ENDY,    NA, GYMAX,   UP,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type12 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX, DOWN,    0},
{VT, GXMAX,  ENDY,    NA, GYMAX,   UP,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX, DOWN, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type13 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX, DOWN,    0},
{VT, GXMAX, GYMIN,    NA, GYMAX,   UP,    0},
{VT, GXMIN, GYMIN,    NA,  ENDY, DOWN,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX, DOWN, TEND}},
/* type14 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMIN,  BGNY,    NA, GYMAX, DOWN,    0},
{VT, GXMAX, GYMIN,    NA, GYMAX,   UP,    0},
{VT, GXMIN, GYMIN,    NA,  ENDY, DOWN,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX,   UP, TEND}},
/* type15 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMAX,  ENDY,    NA,  BGNY,   UP,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX,   UP, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type16 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMAX,  ENDY,    NA,  BGNY,   UP,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX, DOWN, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type17 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{VT, GXMAX, GYMIN,    NA,  BGNY,   UP,    0},
{VT, GXMIN, GYMIN,    NA,  ENDY, DOWN,    0},
{LN,  ENDX,  ENDY, AXMAX, AYMAX, DOWN, TEND},
{NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type18 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{        VT, GXMAX, GYMIN,    NA,  BGNY,   UP,    0},
{        VT, GXMIN, GYMIN,    NA,  ENDY, DOWN,    0},
{        LN,  ENDX,  ENDY, AXMAX, AYMAX,   UP, TEND},
{        NA,    NA,    NA,    NA,    NA,   NA,   NA}},
/* type19 */
{{LN, AXMAX, AYMAX,  BGNX,  BGNY, DOWN,    0},
{        VT, GXMAX, GYMIN,    NA,  BGNY,   UP,    0},
{        VT, GXMIN, GYMIN,    NA, GYMAX, DOWN,    0},
{        VT, GXMAX,  ENDY,    NA, GYMAX,   UP,    0},
{        LN,  ENDX,  ENDY, AXMAX, AYMAX,   UP, TEND}}};


/*  This is the function and comments for the arcsd.c file: */

/* The following table defines the line segment insertions for the 20 arc
    type cases.

For vertical "VT" lines the definitions provide the following
information for the call EDGES_rsSetUPVerticalLineEdge to insert the vertical edge:

db  VT,    X,   minY,   n/a,  maxY, dir, more

EDGES_rsSetUPVerticalLineEdge(edgePtr, X, minY, maxY - minY, dir)10


For arc edge "LN" lines the definitions provide the following
information for the "mwSLE" call to insert the line edge:

db  LN,  BGNX,  BGNY,  ENDX,  ENDY, dir, more

EDGES_rsSetUPStraightLineEdge(edgePtr, BGNX, BGNY, ENDX, ENDY, dir);

In both cases "dir" indicates the edge trace direction (1=DOWN, -1=UP),
"more" indicates of another edge follows (0) or not (TEND). 
*/

#endif /* _ARCSD_H_ */

