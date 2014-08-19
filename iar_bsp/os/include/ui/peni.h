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
*  peni.h                                                       
*
* DESCRIPTION
*
*  This file contains prototypes and externs for peni.c
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
#ifndef _PENI_H_
#define _PENI_H_


extern VOID PORTOPS_rsDefaultPenValues( rsPort *penPort);
extern VOID SetPort( rsPort *portPtr);
extern VOID LSPC_rsSpecialLinePatternAndSquare( blitRcd *LINEREC);
extern VOID POLYGONS_rsSuperSetPolyLineDrawer( point *pointsparm, INT32 npointsparm, INT32 modeparm);
extern VOID LINE_rsOvalPolyLines( INT16 numpoints, point *points);
extern VOID LSPC_rsDashThinLine( blitRcd *LINEREC);
extern VOID PENS_rsOvalLineGeneric(VOID);
extern VOID V2GSIZE(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);

/* Local Functions */
VOID RS_Get_Pen_Setup(PenSetUp *penPtr);
VOID RS_Pen_Setup(PenSetUp *penPtr, INT32 penColor);

#endif /* _PENI_H_ */




