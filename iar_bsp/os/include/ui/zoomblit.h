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
*  zoomblit.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for zoomblit.c
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
#ifndef _ZOOMBLIT_H_
#define _ZOOMBLIT_H_

#define Zoom_Down_Color 1
#define Zoom_Down_RGB   2


extern VOID Port2Gbl(rect *inpRect, rsPort *inpPort, rect *dstRect);
extern VOID COORDS_rsGblClip(rsPort *inpClpPort, rect *outClipR);
extern SIGNED (*PixelPrimitive)(); 

/* Local Functions */
INT32 ZoomBlit(rsPort *srcPORT ,rsPort *dstPORT, rect *argSrcR, rect *argDstR); 

#endif /* _ZOOMBLIT_H_ */




