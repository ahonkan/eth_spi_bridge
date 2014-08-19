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
*  globalrsv.h                                                  
*
* DESCRIPTION
*
*  This file global variable externs for Nucleus GRAFIX RS library
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
#ifndef _GLOBALRSV_H_
#define _GLOBALRSV_H_

#include "ui/edges.h"

/* Global Variables */
extern INT32 currentY;      
extern INT32 rectCount;     
extern rect *highWater;     
                            
extern blitRcd *lclFillRcd; 
extern rect *rectPtr;       
extern rect *rectBase;      
extern signed char sameAsLast;     
                                   
extern rsPort *thePortPtr;      

extern INT32 deltaX;         
extern INT32 deltaY;

extern signed char lclFillRule;        
extern lineEdgeV **AETPtrPtr;          
                                       
extern double alpha_level;
extern BOOLEAN text_trans;
extern BOOLEAN set_back_color;

/* Variables used by the rect routines */
extern rect rXmin;                  
extern INT32 rXWidth;               
extern INT32 rXHeight;              
extern qarcState qaEdgeBuffer[13];  
extern qarcState *GETPtr;           
extern INT32 xRadius;
extern INT32 yRadius;
extern INT16 nextTopEdge;
extern INT16 nextBottomEdge;
extern INT16 thisLeftEdge;
extern INT16 thisRightEdge;
extern rFillRcdType rFillRcd;

/* screen semaphore */
extern NU_SEMAPHORE    ScreenSema;


#endif /* _GLOBALRSV_H_ */





