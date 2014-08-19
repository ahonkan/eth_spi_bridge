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
*  m2b4_drv.h
*
* DESCRIPTION
*
*  This file contains the 2/4 bit driver prototypes and externs.
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
#ifndef _M2B4_DRV_H_
#define _M2B4_DRV_H_

/* Externs just for m2b4_drv.c */
extern VOID M2B4F_SetDestM2_4Bit(VOID);    /* m2b4_fil.c */
extern VOID M2B4F_InvertDestM2_4Bit(VOID); /* m2b4_fil.c */

/* Local Functions that are used in screen_i.c */
VOID  M2B4D_BlitMonoToSelf2_4Bit(blitRcd *blitRec);
VOID  M2B4D_BlitSelfToSelf2_4Bit(blitRcd *blitRec);
INT32 M2B4D_GetPixelFor2_4Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap);
VOID  M2B4D_SetPixelFor2_4Bit(blitRcd *setpRec );

/* Local Prototypes */
VOID  M2B4D_DrawRectEntryBlit1M2_4Bit(blitRcd *blitRec);
VOID  M2B4D_DrawRectEntryBlitM2_4MBit(blitRcd *blitRec);
VOID  M2B4D_DrawRectEntry_SetPixel2_4Bit(blitRcd *drwPRec);
VOID  M2B4D_InvertTrans1M2_4Bit(VOID);
VOID  M2B4D_NotDestSolid1M2_4Bit(VOID);
VOID  M2B4D_NotDestSolidM2_4Bit(VOID);
VOID  M2B4D_OXADest1M2_4Bit(VOID);
VOID  M2B4D_OXADestM2_4Bit(VOID);
VOID  M2B4D_RepDest1M2_4Bit(VOID);
VOID  M2B4D_RepDestM2_4Bit(VOID);
VOID  M2B4D_SetTrans1M2_4Bit(VOID);
VOID  M2B4D_WriteImage2_4Bit(blitRcd *blitRec);
VOID  M2B4D_WriteImage1M2_4Bit(blitRcd *blitRec);
VOID  M2B4D_ReadImage2_4Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax,
                            INT32 gblXmax, INT32 gblYmin, INT32 gblXmin);
VOID  M2B4D_DrawRectEntryImgM2_4Bit(blitRcd *blitRec);
VOID  M2B4D_DrawRectEntry1B2_4Bit(blitRcd *blitRec);
VOID  M2B4D_NotDestSolidImgM2_4Bit(VOID);
VOID  M2B4D_RepDestImgM2_4Bit(VOID);
VOID  M2B4D_OXADestImgM2_4Bit(VOID);


#endif /* _M2B4_DRV_H_ */

