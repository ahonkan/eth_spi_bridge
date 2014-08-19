/***************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME
*
*  m32b_drv.h
*
* DESCRIPTION
*
*  This file contains the 32 bit driver prototypes and externs.
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
#ifndef _M32B_DRV_H_
#define _M32B_DRV_H_

/* Local Functions */
VOID  M32BD_BlitMonoToSelf32Bit(blitRcd *blitRec);
VOID  M32BD_BlitSelfToSelf32Bit(blitRcd *blitRec);
VOID  M32BD_DrawRectEntryImg32Bit(blitRcd *blitRec);
VOID  M32BD_DrawRectEntryBlitMM32Bit(blitRcd *blitRec);
VOID  M32BD_DrawRectEntryImg1B32(blitRcd *blitRec);
VOID  M32BD_DrawRectEntryBlit1M32Bit(blitRcd *blitRec);
VOID  M32BD_DrawRectEntry_SetPixel32(blitRcd *drwPRec);
INT32 M32BD_GetPixelFor32Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap);
VOID  M32BD_InvertTrans1M32Bit(VOID);
VOID  M32BD_NotDestSolid1M32Bit(VOID);
VOID  M32BD_NotDestSolidM32Bit(VOID);
VOID  M32BD_OXADest1M32Bit(VOID);
VOID  M32BD_OXADestM32Bit(VOID);
VOID  M32BD_ReadImage32Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                          INT32 gblYmin, INT32 gblXmin);
VOID  M32BD_RepDestM32Bit(VOID);
VOID  M32BD_RepDest1M32Bit(VOID);
VOID  M32BD_SetPixelFor32Bit(blitRcd *setpRec);
VOID  M32BD_SetTrans1M32Bit(VOID);
VOID  M32BD_WriteImage32Bit(blitRcd *blitRec);
VOID  M32BD_WriteImage1M32Bit(blitRcd *blitRec);

VOID  M32BD_Transparency(VOID);
VOID  M32BD_Text_Transparency(VOID);

#endif /* _M32B_DRV_H_ */

