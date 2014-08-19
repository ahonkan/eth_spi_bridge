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
*  m1b_drv.h
*
* DESCRIPTION
*
*  This file contains the 1 bit driver prototypes and externs.
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
#ifndef _M1B_DRV_H_
#define _M1B_DRV_H_

/* Local Functions that are used in screen_i.c */
VOID  M1BD_BlitMonoToSelf1Bit(blitRcd *blitRec);
VOID  M1BD_BlitSelfToSelf1Bit(blitRcd *blitRec);
VOID  M1BD_WriteImage1Bit(blitRcd *blitRec);
VOID  M1BD_ReadImage1Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax,
                         INT32 gblXmax, INT32 gblYmin, INT32 gblXmin);

/* Local Prototypes */
VOID  M1BD_DrawRectEntryBlitM1Bit(blitRcd *blitRec);
VOID  M1BD_DrawRectEntryBlit1M1Bit(blitRcd *blitRec);
VOID  M1BD_DrawRectEntry_SetPixel1Bit(blitRcd *drwPRec);
VOID  M1BD_DrawRectEntry1_1Bit(blitRcd *blitRec);
VOID  M1BD_InvertTrans1M1Bit(VOID);
VOID  M1BD_OXADest1M1Bit(VOID);
VOID  M1BD_OXADestM1Bit(VOID);
VOID  M1BD_OXADestImgM1Bit(VOID);
VOID  M1BD_NotDestSolidM1Bit(VOID);
VOID  M1BD_NotDestSolid1M1Bit(VOID);
VOID  M1BD_NotDestSolidImgM1Bit(VOID);
VOID  M1BD_RepDestM1Bit(VOID);
VOID  M1BD_RepDest1M1Bit(VOID);
VOID  M1BD_RepDestImgM1Bit(VOID);
VOID  M1BD_SetTrans1M1Bit(VOID);
INT32 M1BD_GetPixelFor1Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap);
VOID  M1BD_SetPixelFor1Bit(blitRcd *setpRec );

#endif /* _M1B_DRV_H_ */

