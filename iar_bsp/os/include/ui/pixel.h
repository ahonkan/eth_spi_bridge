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
*  pixel.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for pixel.c
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
#ifndef _PIXEL_H_
#define _PIXEL_H_

extern VOID U2GP( INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);

/* Local Functions */
SIGNED GetPixel( INT32 argX, INT32 argY);
VOID   SetPixel( INT32 argX, INT32 argY);


#endif /* _PIXEL_H_ */




