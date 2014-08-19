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
*  m24b_fil.h
*
* DESCRIPTION
*
*  This file contains the 24 bit filler prototypes and externs.
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
#ifndef _M24B_FIL_H_
#define _M24B_FIL_H_

/* Local Functions */
VOID  M24BF_Fill24Bit(blitRcd *fillRec);
VOID  M24BF_FillOXADestMem24(VOID);
VOID  M24BF_InvertDestM24Bit(VOID);
VOID  M24BF_InvertDestMem24(VOID);
VOID  M24BF_MonoFillDestMem24(VOID);
VOID  M24BF_MonoFillOXADestMem24(VOID);
VOID  M24BF_MonoPatternFillMem24(blitRcd *fillRec);
VOID  M24BF_MultiColorFillMem24(blitRcd *fillRec);
VOID  M24BF_MultiColorFillRectM24(blitRcd *fillRec);
VOID  M24BF_NotDestFillMem24(VOID);
VOID  M24BF_NotDestMonoFillMem24(VOID);
VOID  M24BF_SetDestM24Bit(VOID);
VOID  M24BF_SetDestMem24(VOID);
VOID  M24BF_SolidFillMem24(blitRcd *fillRec);
VOID  M24BF_SolidFillRectM24(blitRcd *fillRec);

#endif /* _M24B_FIL_H_ */

