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
*  bezierd.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for bezier drawing.
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
#ifndef _BEZIERD_H_
#define _BEZIERD_H_

extern NU_MEMORY_POOL  System_Memory;

extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
extern VOID FillPolygon(point *pointsparm, INT32 npointsparm, INT32 modeparm, INT32 shape);

#ifndef __max
INT32 __max(INT32 val1, INT32 val2);
#endif

/* Local Functions */
INT32  BEZD_BezierDepth(point *points_in_curve);
VOID   BEZD_BezierDepthRecurse(INT32 point0_x, INT32 point0_y, INT32 point1_x, INT32 point1_y, 
                               INT32 point2_x, INT32 point2_y, INT32 point3_x, INT32 point3_y, 
                               INT32 depth);


#endif /* _BEZIERD_H_ */






