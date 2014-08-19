/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       ring.h                                                   
*
*   DESCRIPTION
*
*       This file contains the data structures and function definitions
*       used in the file RING.C.
*
*   DATA STRUCTURES
*
*       RingPos_s
*       Ring_s
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef RING_H
#define RING_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

typedef struct RingPos_s
{
    INT32       Start;
    INT32       Len;
} RingPos_t;

typedef struct Ring_s
{
    INT32       Size;
    INT32       Start;
    INT32       Stop;
    RingPos_t   *PosList;
    INT32       PosCount;
    INT32       PosMax;
    UINT8       *Buffer;
} Ring_t;

Ring_t  *RingAlloc(INT32, INT32);
VOID    RingFree(Ring_t *);
BOOLEAN RingSetMaxElems(Ring_t *, INT32);
INT32   RingGetMaxElems(Ring_t *);
INT32   RingGetElems(Ring_t *);
BOOLEAN RingPutMem(Ring_t *);
BOOLEAN RingPeekMem(Ring_t *, INT32, INT32, UINT8 **, UINT8 *, INT32 *,
                    INT32);
INT32   RingAvailMem(Ring_t *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
