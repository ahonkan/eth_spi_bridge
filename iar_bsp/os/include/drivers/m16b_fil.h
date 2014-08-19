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
*  m16b_fil.h
*
* DESCRIPTION
*
*  This file contains the 16 bit filler prototypes.
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
#ifndef _M16B_FIL_H_
#define _M16B_FIL_H_

/* Local Functions */
VOID M16BF_Fill16Bit(blitRcd *fillRec);
VOID M16BF_FillOXADestMem16(VOID);
VOID M16BF_InvertDestM16Bit(VOID);
VOID M16BF_InvertDestMem16(VOID);
VOID M16BF_MonoPatternFillMem16(blitRcd *fillRec );
VOID M16BF_MonoFillDestMem16(VOID);
VOID M16BF_MonoFillOXADestMem16(VOID);
VOID M16BF_MultiColorFillMem16(blitRcd *fillRec);
VOID M16BF_MultiColorFillRectM16(blitRcd *blitRec);
VOID M16BF_NotDestFillMem16(VOID);
VOID M16BF_NotDestMonoFillMem16(VOID);
VOID M16BF_SetDestM16Bit(VOID);
VOID M16BF_SetDestMem16(VOID);
VOID M16BF_SolidFillMem16(blitRcd *fillRec );
VOID M16BF_SolidFillRectM16(blitRcd *blitRec);

#endif /* _M16B_FIL_H_ */

