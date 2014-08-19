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
*  strkfont.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for strkfont.c
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
#ifndef _STRKFONT_H_
#define _STRKFONT_H_


extern VOID (*lineExecPntr)(); /* must be "extern" */
extern VOID G2UP( INT32 GloblX, INT32 GloblY, INT32 *RtnX, INT32 *RtnY);
extern VOID V2GSIZE( INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);

/* Local Functions */
INT32 STRKFONT_rsStrokeText( signed char *TEXT, INT32 INDEX, INT32 COUNT,INT32 CHARSIZE);
INT32 STRKFONT_rsStrokeFontInit( signed char *TEXT, INT32 INDEX, INT32 COUNT , INT32 CHARSIZE );
VOID ScaleNode( INT32 nodeX, INT32 nodeY, point *scaledPoint);
VOID ScalePath( INT32 nodeX, INT32 nodeY, point *scaledPoint);


#endif /* _STRKFONT_H_ */




