/***************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*  mb_extrn.h
*
* DESCRIPTION
*
*  This file contains the externs for all bits per pixel.
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
#ifndef _MB_EXTRN_H_
#define _MB_EXTRN_H_

#ifdef USING_DIRECT_X
  VOID BlitBacktoFront (rect *scrnRect);
#endif

/* Prototypes */
INT32 CLIP_Set_Up_Clip(blitRcd *clipBlit, rect *clipR, INT32 blitMayOverlap, INT32 isLine);
INT32 CLIP_Blit_Clip_Region(blitRcd *fcRec, rect *fillRect, rect *sRect);
INT32 CLIP_Check_YX_Band_Blit(rect *bandRect, rect *blitRectPtr, rect *clipR, INT32 *rectCnt);
INT32 CLIP_Check_YX_Band(rect *bandRect, rect *blitRectPtr, rect *clipR, INT32 *rectCnt);
INT32 CLIP_Fill_Clip_Region(blitRcd *fcRec, rect *dRectect);
INT32 CLIP_Blit_Clip_Region_TB_RL(blitRcd *fcRec, rect *fillRect, rect *sRect);
VOID  CLIP_Blit_Clip_Region_BT_RL(blitRcd *fcRec, rect *fillRect, rect *sRect);
VOID  CLIP_Blit_Clip_Region_BT_LR(blitRcd *fcRec, rect *fillRect, rect *sRect);
VOID  CLIP_Line_Clip_Region(INT32 rectCnt, lineS *destRect);
VOID  CLIP_ClipAndDrawEntry(VOID);
VOID  SCREENS_Nop(VOID);
VOID  nuResume(grafMap *argGRAFMAP);

#endif /* _MB_EXTRN_H_ */

