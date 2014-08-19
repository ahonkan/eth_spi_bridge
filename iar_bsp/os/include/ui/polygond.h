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
*  polygond.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for polygond.c
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
#ifndef _POLYGOND_H_
#define _POLYGOND_H_

extern VOID G2UR( rect UserRect, rect *RtnRect);
extern VOID FillPolygon( point *pointsparm, INT32 npointsparm, INT32 modeparm, INT32 shape);

/* Local Functions */
STATUS RS_Polygon_Draw( ObjectAction action, INT32 polyPtsCount, polyHead *polyHeader,
                        point *polyPts , INT32 patt);


#endif /* _POLYGOND_H_ */




