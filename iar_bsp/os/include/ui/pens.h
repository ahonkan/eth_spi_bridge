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
*  pens.h                                                       
*
* DESCRIPTION
*
*  This file contains prototypes and externs for pens.c
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
#ifndef _PENS_H_
#define _PENS_H_

extern VOID V2GSIZE( INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);
extern VOID SetPort( rsPort *portPtr);

/* Local Functions */
VOID GetPenState( rsPort *penRcd);
VOID SetPenState( rsPort *penRcd);
VOID HidePen(VOID);
VOID ShowPen(VOID);
VOID DefineDash( INT32 argStyle, dashRcd *dashArray);
VOID DashStyle( INT32 DASHSTYL);
VOID PenOffset( INT32 DSHOFFSET);
VOID PENS_rsOvalLineGeneric(VOID);
VOID BackColor32( SIGNED argColor);
VOID PenMode ( INT32 Mode );
VOID BackColor(INT32 argColor);
VOID BackPattern( INT32 patNDX);

#endif /* _PENS_H_ */




