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
*  regmerge.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for regmerge.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _REGMERGE_H_
#define _REGMERGE_H_

struct RBlock
{
    struct RBlock* NextBlock;
    SIGNED rX;
    signed char rDir;
    UINT8  rList;
    INT16  rAlign;
    SIGNED rCount;
};

#define STACK_BUFFER_SIZE_FOR_REGION 0x1000  

#define LEFT_EDGE    1
#define RGHT_EDGE   -1
#define RECT_LST1    1
#define RECT_LST2   -1


/* Local Functions */
UINT32 REGMERGE_rsMergeRegion( INT32 numRects1, rect *rectList1,
                               INT32 numRects2, rect *rectList2,
                               INT32 sizeRGN, region *destRGN, INT32 rgnOP);

VOID SetupEmptyRegion( region *destRGN, INT32 makeRegion, INT32 sizeRGN);
INT32 HandleChange( INT32 *destSizeLeft, rect *destScanMax,
                    INT32 baseDestSize, INT32 curtY, INT32 makeRegion,
                    rect *destPtr, rect *destSave);

#endif /* _REGMERGE_H_ */




