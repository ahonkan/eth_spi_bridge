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
*  lined.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for lined.c
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
#ifndef _LINED_H_
#define _LINED_H_

extern VOID (*lineExecPntr)();
extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);

/* Local Functions */
STATUS RS_Line_Draw( INT16 pointCount, point *linePts );


#endif /* _LINED_H_ */






