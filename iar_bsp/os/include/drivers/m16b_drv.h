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
*  m16b_drv.h
*
* DESCRIPTION
*
*  This file contains the 16 bit driver prototypes and externs.
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
#ifndef _M16B_DRV_H_
#define _M16B_DRV_H_

/* Local Functions */
VOID M16BD_BlitMonoToSelf16Bit(blitRcd *blitRec);
VOID M16BD_BlitSelfToSelf16Bit(blitRcd *blitRec);
VOID M16BD_DrawRectEntryBlit1M16Bit(blitRcd *blitRec);
VOID M16BD_DrawRectEntryImg1B16(blitRcd *blitRec);
VOID M16BD_DrawRectEntry_SetPixel16(blitRcd *drwPRec);
VOID M16BD_DrawRectEntryImg16Bit(blitRcd *blitRec);
VOID M16BD_DrawRectEntryBlitMM16Bit(blitRcd *blitRec);
INT32  M16BD_GetPixelFor16Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap);
VOID M16BD_InvertTrans1M16Bit(VOID);
VOID M16BD_NotDestSolidM16Bit(VOID);
VOID M16BD_NotDestSolid1M16Bit(VOID);
VOID M16BD_OXADestM16Bit(VOID);
VOID M16BD_OXADest1M16Bit(VOID);
VOID M16BD_ReadImage16Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                          INT32 gblYmin, INT32 gblXmin);
VOID M16BD_RepDestM16Bit(VOID);
VOID M16BD_RepDest1M16Bit(VOID);
VOID M16BD_SetTrans1M16Bit(VOID);
VOID M16BD_SetPixelFor16Bit(blitRcd *setpRec);
VOID M16BD_WriteImage16Bit(blitRcd *blitRec);
VOID M16BD_WriteImage1M16Bit(blitRcd *blitRec);
VOID M16BD_Transparency(VOID);
VOID M16BD_Text_Transparency(VOID);


#endif /* _M16B_DRV_H_ */

