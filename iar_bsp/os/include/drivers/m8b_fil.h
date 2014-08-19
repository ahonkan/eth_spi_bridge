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
*  m8b_fil.h
*
* DESCRIPTION
*
*  This file contains the 8 bit filler prototypes and externs.
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
#ifndef _M8B_FIL_H_
#define _M8B_FIL_H_

/* Used by m8b_drv.c */
VOID M8BF_InvertSrcLCD(VOID);
VOID M8BF_SetSrcLCD(VOID);
VOID M8BF_Fill8Bit(blitRcd *fillRec);
VOID M8BF_SolidFillMem(blitRcd *fillRec);
VOID M8BF_DrawSolidFillRectM(blitRcd *fillRec);
VOID M8BF_SetDestMem(VOID);
VOID M8BF_InvertDestMem(VOID);
VOID M8BF_FillOXADestMem(VOID);
VOID M8BF_NotDestFillMem(VOID);
VOID M8BF_MultiColorFillMem(blitRcd *fillRec);
VOID M8BF_MultiColorFillRectM(blitRcd *fillRec);
VOID M8BF_MonoPatternFillMem(blitRcd *fillRec);
VOID M8BF_MonoFillDestMem(VOID);
VOID M8BF_MonoFillOXADestMem(VOID);
VOID M8BF_NotDestMonoFillMem(VOID);
VOID M8BF_TranMonoFillOXADestMem(VOID);
VOID M8BF_TranMonoFillDestMem(VOID);

#endif /* _M8B_FIL_H_ */

