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
*  texti.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for texti.c
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
#ifndef _TEXTI_H_
#define _TEXTI_H_


/* contain these prototypes: */
extern INT32 STRKFONT_rsStrokeFontInit( signed char *TEXT, INT32 INDEX, INT32 COUNT , INT32 CHARSIZE );
extern INT32 BMAPF_TextAlignHorizontalCR ( signed char *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE);
extern VOID V2GSIZE( INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);


/* Local Functions */
VOID RS_Get_Text_Setup(TextSetUp *textPtr);
VOID RS_Text_Setup(TextSetUp *textPtr);
VOID RasterOp( INT32 argWMODE);


#endif /* _TEXTI_H_ */




