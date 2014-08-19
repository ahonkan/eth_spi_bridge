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
*  m2b4_fil.h
*
* DESCRIPTION
*
*  This file contains the 2/4 bit filler prototypes and externs.
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
#ifndef _M2B4_FIL_H_
#define _M2B4_FIL_H_

VOID M2B4F_FillOXADestM2_4Bit(VOID);
VOID M2B4F_NotDestFillM2_4Bit(VOID);

/* Local Functions */
VOID M2B4F_Fill2_4Bit(blitRcd *fillRec);
VOID M2B4F_DrawSolidFillRectM2_4Bit(blitRcd *blitRec);
VOID M2B4F_SolidFillM2_4Bit(blitRcd *fillRec);
VOID M2B4F_MonoPatternFillM2_4Bit(blitRcd *fillRec);
VOID M2B4F_MultiColorFillM2_4Bit(blitRcd *fillRec);
VOID M2B4F_MultiColorFillRectM2_4Bit(blitRcd *fillRec);
VOID M2B4F_SetDestM2_4Bit(VOID);
VOID M2B4F_InvertDestM2_4Bit(VOID);
VOID M2B4F_MonoFillDestM2_4Bit(VOID);
VOID M2B4F_MonoFillOXADestM2_4Bit(VOID);
VOID M2B4F_NotDestMonoFillM2_4Bit(VOID);

#endif /* _M2B4_FIL_H_ */

