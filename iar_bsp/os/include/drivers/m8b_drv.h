/***************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
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
*  m8b_drv.h
*
* DESCRIPTION
*
*  This file contains the 8 bit driver prototypes and externs.
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
#ifndef _M8B_DRV_H_
#define _M8B_DRV_H_

/* Local Functions */
VOID M8BD_BlitMonoToSelf8Bit(blitRcd *blitRec);
VOID M8BD_BlitSelfToSelf8Bit(blitRcd *blitRec);
VOID M8BD_DrawRectEntryImg(blitRcd *blitRec);
VOID M8BD_DrawRectEntryBlit1M(blitRcd *blitRec);
VOID M8BD_DrawRectEntryBlitMM(blitRcd *blitRec);
VOID M8BD_DrawRectEntryImg1Bit(blitRcd *blitRec);
VOID M8BD_DrawRectEntry_SetPixel(blitRcd *drwPRec);
INT32  M8BD_GetPixelFor8Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap);
VOID M8BD_InvertTrans1M8Bit(VOID);
VOID M8BD_NotDestSolidM8Bit(VOID);
VOID M8BD_NotDestSolid1M8Bit(VOID);
VOID M8BD_OXADest1M8Bit(VOID);
VOID M8BD_OXADestM8Bit(VOID);
VOID M8BD_ReadImage8Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                        INT32 gblYmin, INT32 gblXmin);
VOID M8BD_RepDest1M8Bit(VOID);
VOID M8BD_RepDestM8Bit(VOID);
VOID M8BD_SetTrans1M8Bit(VOID);
VOID M8BD_SetPixelFor8Bit(blitRcd *setpRec );
VOID M8BD_WriteImage8Bit(blitRcd *blitRec);
VOID M8BD_WriteImage1M8Bit(blitRcd *blitRec);

#endif /* _M8B_DRV_H_ */

