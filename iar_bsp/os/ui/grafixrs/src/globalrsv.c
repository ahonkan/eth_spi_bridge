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
*  globalrsv.c                                                  
*
* DESCRIPTION
*
*  Contains global RS library variables.
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
*  rs_base.h
*  edges.h
*
***************************************************************************/

#include "ui/rs_base.h"
#include "ui/edges.h"

/* scan line currently being scanned */
INT32 currentY;       

/* # of rects in blitRcd */
INT32 rectCount;      

/* last address at which a rect can start in the blitRcd and not be the last 
   rect in the list */
rect  *highWater;     

/* local copy of fillRcd */
blitRcd *lclFillRcd;  

/* pointer to next free rect in the fillRcd */
rect *rectPtr;        

/* pointer to beginning of rect list in the fillRcd */
rect *rectBase;       

/* 1 if the current line has exactly the same edges as the preceding line, 0 else */
signed char sameAsLast;      

#ifdef  GLOBAL_ALPHA_SUPPORT

double alpha_level;
BOOLEAN text_trans = 0;
                                                
#endif  /* GLOBAL_ALPHA_SUPPORT */

BOOLEAN set_back_color = 0;

/* 1 for winding rule fill, 0 for odd/even fill */
signed char lclFillRule;        

/* pointer to AET pointer in low-memory case (at start of working buffer) */
lineEdgeV **AETPtrPtr;          

/* Global Variables used by the rect routines */
rFillRcdType rFillRcd;

/* temporary rectangle */
rect rXmin;                 

/* rectangle width */
INT32 rXWidth;        

/* rectangle height */
INT32 rXHeight;             

/* edge buffer */
qarcState qaEdgeBuffer[13]; 

/* pointer to global edge table */
qarcState *GETPtr;          

INT32 xRadius;
INT32 yRadius;
INT16 nextTopEdge;
INT16 nextBottomEdge;
INT16 thisLeftEdge;
INT16 thisRightEdge;

/* screen semaphore */
NU_SEMAPHORE    ScreenSema;
