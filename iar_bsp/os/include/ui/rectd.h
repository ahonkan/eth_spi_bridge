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
*  rectd.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for rectd.c
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
#ifndef _RECTD_H_
#define _RECTD_H_

#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/edges.h"

extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);
extern VOID V2GSIZE(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);
extern VOID (*lineExecPntr)();

/* Local Functions */
STATUS RS_Rectangle_Draw( ObjectAction action, rect *argRect, INT32 patt, INT32 DiaX, INT32 DiaY);
VOID AddOuterEdges(qarcState *newEdge);

#endif /* _RECTD_H_ */




