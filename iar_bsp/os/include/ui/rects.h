/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  rects.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for rects.c
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
#ifndef _RECTS_H_
#define _RECTS_H_

INT32   RectHeight(rect *r);
INT32   RectWidth(rect *r);
INT32   EqRectWidth(rect *r, rect *r1);
INT32   EqRectHeight(rect *r, rect *r1);


#endif /* _RECTS_H_ */


