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
*  gfxstack.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for gfxstack.c
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
#ifndef _GFXSTACK_H_
#define _GFXSTACK_H_

extern VOID SetPort(rsPort *portPtr);

/* Local Functions */
VOID PushGrafix(INT32 *argTS);
VOID PopGrafix(INT32 *argTS);

UINT32      GFX_GetScreenSemaphore(VOID);
UINT32      GFX_ReleaseScreenSemaphore(VOID);

#endif /* _GFXSTACK_H_ */






