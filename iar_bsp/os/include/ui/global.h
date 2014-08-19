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
*  global.h                                                     
*
* DESCRIPTION
*
*  This file contains API defines and data types. 
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
#ifndef _GLOBAL_H_ 
#define _GLOBAL_H_

/* These are the GLOBAL enumerated types that we will use:
The error definitions that need to be set for for STATUS checking
Error Return Status Range: -4000 to -4250 */

/* return values */
#define NU_INVALID_DRAW_OP      -4000
#define NU_INVALID_WIDTH        -4001
#define NU_INVALID_HEIGHT       -4002
#define NU_INVALID_ANGLE        -4003
#define NU_INVALID_COMMAND      -4004

#define NU_INVALID_BPP          -4005
#define NU_INVALID_BMP          -4006
#define NU_INVALID_GIF          -4007
#define NU_INVALID_JPEG         -4008
#define NU_UNKNOWN_IMAGE        -4009
#define NU_INVALID_PALETTE      -4010
#define NU_INVALID_FLAG         -4011
#define NU_INVALID_ARGUMENTS    -4012
#define NU_NOT_BI_RGB           -4013

#define NU_TASKS_UNAVAILABLE    -4014
#define NU_TASK_NOT_STARTED     -4015
#define NU_DUPLICATE_TASK_NAME  -4016

#define NU_INVALID_SRC_PORT     -4017
#define NU_INVALID_DST_PORT     -4018

#define NU_INVALID_SRC_RECT     -4019
#define NU_INVALID_DST_RECT     -4020

/* Default  value can be set to whatever you would like to use but at the moment it works with -1. */
#define DEFAULT_VALUE    -1
#define DEFAULT_UNSIGNED_VALUE    0xFFFFFFFF

/* This is the define used by the GFX_GetScreenSemaphore call to determine */
/* how long to wait for the semaphore to become available.  This can be */
/* modified to fit the applications specifics. */
#define RS_SUSPEND_TIME         100

/* This is the action enumerated data type */
typedef enum _ObjectAction
{
    FRAME  = 0,
    PAINT  = 1,
    FILL   = 2,
    ERASE  = 3,
    INVERT = 4,  
    POLY   = 5, /* only used by BEZIER */
	TRANS  = 6  /* set alpha_value for transparency level */
} ObjectAction;

/* This is the Optional fillers that can be done in the API's */
typedef enum _OptionalFillers
{
    NONE        = 0,
    BOUNDARY    = 1,
    FLOOD       = 2
} OptionalFillers;


/* This is the shape of pen data type */
typedef enum _ShapeOfPen
{
    NORMAL      = -1,
    RECTANGLE   = 0,
    OVAL        = 1
} ShapeOfPen;

extern STATUS Debug_Allocation(NU_MEMORY_POOL *pool_ptr,VOID **alloc,unsigned int size,
						unsigned long suspend, unsigned long line, const char* file);
extern STATUS Debug_Deallocation(VOID *ptr);

extern VOID* MEM_calloc(UINT32 cnt, UINT32 size);
extern VOID* MEM_malloc( UINT32 size );

/* Used only for debugging memory allocation errors */
/* #define GRAFIX_DEBUG_ALLOC_FOR_SYSTEM_MEMORY */


#if defined (GRAFIX_DEBUG_ALLOC_FOR_SYSTEM_MEMORY)
#define      GRAFIX_Allocation(a,b,c,d)    Debug_Allocation(a,b,c,d, __LINE__, __FILE__)
#define      GRAFIX_Deallocation           Debug_Deallocation
#else
#define      GRAFIX_Allocation             NU_Allocate_Memory
#define      GRAFIX_Deallocation           NU_Deallocate_Memory
#endif
#endif /* _GLOBAL_H_ */ 




