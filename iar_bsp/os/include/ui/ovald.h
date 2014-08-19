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
*  ovald.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for ovald.c
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
#ifndef _OVALD_H_
#define _OVALD_H_

extern VOID U2GR( rect UserRect, rect *RtnRect, INT16 frame);
extern VOID ARCSD_ArcDash( rect *boundR, INT16 BANGLE, INT16 AANGLE);

/* Local Functions */
STATUS RS_Oval_Draw( ObjectAction action, rect *argRect, INT32 patt);

#endif /* _OVALD_H_ */




