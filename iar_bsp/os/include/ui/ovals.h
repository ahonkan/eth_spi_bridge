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
*  ovals.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for ovals.c
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
#ifndef _OVALS_H_
#define _OVALS_H_

extern VOID U2GR( rect UserRect, rect *RtnRect, INT16 frame);
extern VOID G2UP( INT32 GloblX, INT32 GloblY, INT32 *RtnX, INT32 *RtnY);
extern VOID InsetRect( rect * R , INT32 dltX , INT32 dltY);

/* Local Functions */
VOID OvalPt( rect *argR, INT32 rsangle, point *gblPt);


#endif /* _OVALS_H_ */




