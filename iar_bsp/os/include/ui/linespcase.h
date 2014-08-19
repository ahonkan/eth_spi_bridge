/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
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
*  linespcase.h                                                 
*
* DESCRIPTION
*
*  This file contains prototypes and externs for linespcase.c
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
#ifndef _LINESPCASE_H_
#define _LINESPCASE_H_

extern INT32 rectCnt;
extern VOID POLYGONS_rsScanAndDrawConvex(UINT8 *lclScratchBufferPtr, SIGNED lclScratchBufferSize,
                point * points, INT32 npoints, INT32 mode,
                SIGNED lclXAdjust, SIGNED lclYAdjust);

/* Local Functions */
VOID  LSPC_rsSpecialLinePatternAndSquare(blitRcd *LINEREC);
VOID  LSPC_rsDashThinLine(blitRcd *LINEREC);

#endif /* _LINESPCASE_H_ */






