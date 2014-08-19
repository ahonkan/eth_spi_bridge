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
*  fonti.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for fonti.c
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
#ifndef _FONTI_H_
#define _FONTI_H_

#include "nucleus.h"
#include "ui/global.h"

typedef struct _PenSetUp
{
    /* NORMAL = -1,RECTANGLE = 0,OVAL = 1 */
    ShapeOfPen  penShape;  
    INT32       penWidth;
    INT32       penHeight;
    INT32       penDash;
    INT32       penCap;
    INT32       penJoin;
    INT32       patt;

    /* limitAngle */
    INT32       penMiter; 
} PenSetUp;

extern PenSetUp     thePenSetUp;

typedef struct _TextSetUp
{
    INT32       boldFactor;
    INT32       face;               

    /* RasterOps */
    INT32       mode;               
    INT32       SPACEExtraSpace;
    INT32       charExtraSpace;
    INT32       underlineGap;
    INT32       descentUnderlineGap;
    INT32       charPath;
    INT32       charHorizontalAlign;
    INT32       charVerticalAlign;
	
#ifdef      USE_STROKEDFONT

    INT32       charWidth;
    INT32       charHeight;
    INT32       charAngle;
    INT32       charSlant;
	
#endif      /* USE_STROKEDFONT */

    INT32       markerCharAngle;
    INT32       markerCharWidth;
    INT32       markerCharStyle;
    INT32       markerCharHeight;
} TextSetUp;
    
extern TextSetUp  theTextSetUp;

/* Local Functions */
VOID RS_Init_Text_And_Pen(VOID);
VOID RS_Reset_Pen(VOID);
VOID RS_Reset_Text(VOID);

#endif /* _FONTI_H_ */






