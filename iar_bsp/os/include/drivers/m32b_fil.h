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
*  m32b_fil.h
*
* DESCRIPTION
*
*  This file contains the 32 bit filler prototypes and externs.
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
#ifndef _M32B_FIL_H_
#define _M32B_FIL_H_

/* Local Functions */
VOID  M32BF_Fill32Bit(blitRcd *fillRec);
VOID  M32BF_FillOXADestMem32(VOID);
VOID  M32BF_InvertDestM32Bit(VOID);
VOID  M32BF_InvertDestMem32(VOID);
VOID  M32BF_MonoFillDestMem32(VOID);
VOID  M32BF_MonoFillOXADestMem32(VOID);
VOID  M32BF_MonoPatternFillMem32(blitRcd *fillRec);
VOID  M32BF_MultiColorFillMem32(blitRcd *fillRec);
VOID  M32BF_MultiColorFillRectM32(blitRcd *fillRec);
VOID  M32BF_NotDestFillMem32(VOID);
VOID  M32BF_NotDestMonoFillMem32(VOID);
VOID  M32BF_SetDestM32Bit(VOID);
VOID  M32BF_SetDestMem32(VOID);
VOID  M32BF_SolidFillMem32(blitRcd *fillRec);
VOID  M32BF_SolidFillRectM32(blitRcd *fillRec);

VOID  M32_FillTransparency(VOID);

#endif /* _M32B_FIL_H_ */

