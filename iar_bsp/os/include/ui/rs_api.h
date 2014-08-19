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
*  rs_api.h                                                     
*
* DESCRIPTION
*
*  Rendering Service API calls
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
#ifndef _RS_API_H_
#define _RS_API_H_

#include "ui/global.h"
#include "ui/textd.h"
#include "ui/fonti.h"

STATUS RS_Arc_Draw( ObjectAction action, rect *argRect, INT16 bgnAngle, INT16 arcAngle,
                    INT16 patt);
STATUS RS_Bezier_Draw( ObjectAction action, point *bezierPts, INT32 pointCount, INT32 patt);
STATUS RS_Line_Draw( INT16 pointCount, point *linePts);
STATUS RS_OptionalFiller( OptionalFillers fill,INT32 xStart,INT32 yStart,
                          INT32 bColor,rect *fillerLimitRect);
STATUS RS_Oval_Draw( ObjectAction action, rect *argRect, INT32 patt);
STATUS RS_Text_Draw( VOID * strings, DrawTextType  textType, INT32 index, INT32 count);
VOID   RS_Reset_Pen(VOID);
STATUS RS_Polygon_Draw( ObjectAction action, INT32 polyPtsCount, polyHead *polyHeader,
                            point *polyPts ,INT32 patt);
STATUS RS_Rectangle_Draw( ObjectAction action, rect *argRect, INT32 patt, INT32 DiaX, INT32 DiaY);
STATUS RS_Region_Draw( ObjectAction action, region *argRegion, INT32 regPattern);

VOID   RS_Init_Text_And_Pen(VOID);
VOID   RS_Reset_Text(VOID);

VOID   RS_Get_Pen_Setup(PenSetUp *penPtr);
VOID   RS_Pen_Setup(PenSetUp *penPtr, INT32 penColor);
VOID   RS_Get_Text_Setup(TextSetUp *textPtr);
VOID   RS_Text_Setup(TextSetUp *textPtr);

STATUS RS_Display_Image(CHAR *path, image *bmpImg, palData *img_palette, INT32 x_coord, 
						INT32 y_coord, UINT32 flg);

#ifdef FAL_INCLUDED
STATUS RS_Delete_Animated_Gif_Task(CHAR *gif_task_name);  /*  Animated Gif deletion routine */
STATUS RS_Suspend_Animated_Gif_Task(CHAR *gif_task_name); /*  Suspend Animated Gif */
STATUS RS_Resume_Animated_Gif_Task(CHAR *gif_task_name);  /*  Resume Animated Gif */
#endif

#endif /* RS_API_H_ */


