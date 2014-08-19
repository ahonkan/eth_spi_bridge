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
*  m24b_drv.h
*
* DESCRIPTION
*
*  This file contains the 24 bit driver prototypes and externs.
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
#ifndef _M24B_DRV_H_
#define _M24B_DRV_H_

/* Local Functions */
VOID  M24BD_BlitMonoToSelf24Bit(blitRcd *blitRec);
VOID  M24BD_BlitSelfToSelf24Bit(blitRcd *blitRec);
VOID  M24BD_DrawRectEntryImg24Bit(blitRcd *blitRec);
VOID  M24BD_DrawRectEntryBlitMM24Bit(blitRcd *blitRec);
VOID  M24BD_DrawRectEntryImg1B24(blitRcd *blitRec);
VOID  M24BD_DrawRectEntryBlit1M24Bit(blitRcd *blitRec);
VOID  M24BD_DrawRectEntry_SetPixel24(blitRcd *drwPRec);
INT32 M24BD_GetPixelFor24Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap);
VOID  M24BD_InvertTrans1M24Bit(VOID);
VOID  M24BD_NotDestSolid1M24Bit(VOID);
VOID  M24BD_NotDestSolidM24Bit(VOID);
VOID  M24BD_OXADest1M24Bit(VOID);
VOID  M24BD_OXADestM24Bit(VOID);
VOID  M24BD_ReadImage24Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                          INT32 gblYmin, INT32 gblXmin);
VOID  M24BD_RepDestM24Bit(VOID);
VOID  M24BD_RepDest1M24Bit(VOID);
VOID  M24BD_SetPixelFor24Bit(blitRcd *setpRec);
VOID  M24BD_SetTrans1M24Bit(VOID);
VOID  M24BD_WriteImage24Bit(blitRcd *blitRec);
VOID  M24BD_WriteImage1M24Bit(blitRcd *blitRec);

VOID  M24BD_Transparency(VOID);
VOID  M24BD_Text_Transparency(VOID);

#endif /* _M24B_DRV_H_ */

