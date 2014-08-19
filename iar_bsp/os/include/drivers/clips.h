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
*   clips.h
*
* DESCRIPTION
*
*  This file contains the Clipping prototypes and externs.
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
#ifndef _CLIPS_H_
#define _CLIPS_H_

/* Externs needed for MNT clipping */
#ifdef USING_DIRECT_X
  extern VOID BlitBacktoFront (rect *scrnRect);
#endif


/* Local Line Clipping function prototypes */
VOID  CLIP_ClipBottomOrRightTocYmaxOrcXmax(VOID);
VOID  CLIP_ClipBottomOrLeftTocYmaxOrcXmin(VOID);
VOID  CLIP_ClipTopTocYmin(VOID);
INT32 CLIP_ClipTopOrLeftTocYminOrcXmin(VOID);
INT32 CLIP_ClipTopOrRightTocYminOrcXmax(VOID);
VOID  CLIP_ClipAndDrawEntry(VOID);
INT32 CLIP_HandleFirstPointNull(VOID);
INT32 CLIP_LineClipping(VOID);

/* Local Region Clipping function prototypes */
INT32 CLIP_Blit_Clip_Region(blitRcd *fcRec, rect *fillRect, rect *sRect);
INT32 CLIP_Fill_Clip_Region(blitRcd *fcRec, rect *fillRect);
INT32 CLIP_Set_Up_Clip(blitRcd *clipBlit, rect *clipR, INT32 blitMayOverlap, INT32 isLine);
INT32 CLIP_Check_YX_Band_Blit(rect *bandRect, rect *blitRectPtr, rect *clipR, INT32 *rectCnt);
INT32 CLIP_Check_YX_Band(rect *bandRect, rect *blitRectPtr, rect *clipR, INT32 *rectCnt);
VOID  CLIP_Line_Clip_Region(INT32 rectCnt, lineS *listPtr);
VOID  CLIP_Blit_Clip_Region_BT_RL(blitRcd *fcRec, rect *fillRect, rect *sRect);
VOID  CLIP_Fill_Clip_Region_BT_RL(blitRcd *fcRec, rect *fillRect);
VOID  CLIP_Blit_Clip_Region_BT_LR(blitRcd *fcRec, rect *fillRect, rect *sRect);
VOID  CLIP_Fill_Clip_Region_BT_LR(blitRcd *fcRec, rect *fillRect);
INT32 CLIP_Blit_Clip_Region_TB_RL(blitRcd *fcRec, rect *fillRect, rect *sRect);
INT32 CLIP_Fill_Clip_Region_TB_RL(blitRcd *fcRec, rect *fillRect);

#endif /* _CLIPS_H_ */
