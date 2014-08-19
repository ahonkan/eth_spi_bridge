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
*  memrymgr.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for memrymgr.c
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
#ifndef _MEMRYMGR_H_
#define _MEMRYMGR_H_

#ifdef __cplusplus      /* If C++, disable "mangling" */
extern "C" {
#endif

#include "ui/rsfonts.h"
    
/* Local Functions - MMU API */

/* Local Functions - non MMU non API */
VOID* MEM_calloc(UINT32 cnt, UINT32 size);
VOID* MEM_malloc( UINT32 size );

#ifdef __cplusplus
}
#endif

#endif /* _MEMRYMGR_H_ */

