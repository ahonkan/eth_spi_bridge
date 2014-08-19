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
*   m1b_fil.h
*
* DESCRIPTION
*
*  This file contains the 1 bit filler prototypes and externs.
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
#ifndef _M1B_FIL_H_
#define _M1B_FIL_H_

/* Function called by screen_i.c */
VOID M1BF_Fill1Bit(blitRcd *fillRec);

/* Local to this file but used by m1b_drv.c */
VOID M1BF_SetDestM1Bit(VOID);
VOID M1BF_InvertDestM1Bit(VOID);

/* Local prototypes just used in m1b_fil.c */
VOID M1BF_SolidFillM1Bit(blitRcd *fillRec );
VOID M1BF_MonoPatternFillM1Bit(blitRcd *fillRec );
VOID M1BF_DrawSolidFillRectM1Bit(blitRcd *blitRec);

VOID M1BF_FillOXADestM1Bit(VOID);
VOID M1BF_NotDestFillM1Bit(VOID);
VOID M1BF_InvertDestM1Bit(VOID);
VOID M1BF_MonoFillDestM1Bit(VOID);
VOID M1BF_MonoFillOXADestM1Bit(VOID);
VOID M1BF_NotDestMonoFillM1Bit(VOID);


#endif /* _M1B_FIL_H_ */

