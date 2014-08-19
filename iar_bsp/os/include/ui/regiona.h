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
*  regiona.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for regiona.c.
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
#ifndef _REGIONA_H_
#define _REGIONA_H_

extern region *RS_Regions_Merge( INT32 rgnOP, region *rgn1, region *rgn2);
extern VOID U2GR( rect UserRect, rect *RtnRect, INT16 frame);
extern VOID V2GSIZE( INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);
extern UINT32 REGMERGE_rsMergeRegion( INT32 numRects1, rect *rectList1,
                                      INT32 numRects2, rect *rectList2,
                                      INT32 sizeRGN,  region *destRGN, INT32 rgnOP);
extern INT32 GetPixel(INT32 argX, INT32 argY);

extern NU_MEMORY_POOL  System_Memory;

/* Local Functions */
VOID   REGIONA_rsRectangleListConversionSort( INT32 numRects, rect *srcRects, rect *destRects);
VOID   OffsetRegion( region *RGN, INT32 dltX, INT32 dltY);
VOID   OpenRegion(VOID);
VOID   REGIONA_rsRegionCaptureRectangle( blitRcd *BlitRec);
VOID   ClipRegion( region *RGN);
VOID   NullRegion( region *argRegion);
INT32  EmptyRegion ( region * argRegion);
VOID   DestroyRegion( region *rgn);
INT32  EqualRegion( region *rg1, region *rg2 );
region *RectListToRegion( INT32 NUMRECTS, rect *RECTLIST);
region *RectRegion( rect *srcRECT);
region *SetRegion( INT32 X1, INT32 Y1, INT32 X2, INT32 Y2);
region *CloseRegion( VOID );
region *DupRegion( region *argRegion);
region *BitmapToRegion( rect *srcRect, INT32 transPColor );


#endif /* _REGIONA_H_ */




