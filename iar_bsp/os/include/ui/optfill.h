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
*   optfill.h                                                   
*
* DESCRIPTION
*
*   This file contains prototypes and externs for optfill.c
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
#ifndef _OPTFILL_H_
#define _OPTFILL_H_

#define MAX_RECTS_FOR_OPTFILL   4
#define STACK_BUFFER_SIZE_FOR_OPTFILL   0x0800

/* Structure for storing one scan line segment in a linked list.   */
typedef struct _SBlock
{
    /* pointer to next block in list */
    struct _SBlock *NextBlock;    

    /* left end of segment           */
    SIGNED sXmin;                 

    /* right end of segment plus 1   */
    SIGNED sXmax;                 
} SBlock;


/* Local/Global Variables */
/* color to flood over */
INT32   SeedColor;          

/* boundary color */
INT32   boundaryColor;      
INT32   localFeedX;
INT32   localFeedY;

/* pointer to limit rect */
rect    *LimitPtr;          

/* working blit record */
blitRcd blitRecord;         

/* common variables */
/* pointer to start of list of free blocks in the
   memory pool. 0 if no more blocks */
SBlock *FreeBlocks;         

/* seed point X in global coordinates */
INT32  SeedX;               

/* scan line currently being filled, in global
   coordinates */
INT32  ScanLine;           

/* 1 or -1, to move down or up */
INT32  sDirection;         

/* scan line at which to stop when  reached in current
   scanning direction (1 past last line to do) */
INT32  ForwardLimit;        

/* scan line at which to stop if current scanning
   direction is reversed (1 past last line to do) (becomes
   ForwardLimit if direction is reversed) */
INT32  BackwardLimit;       

/* pointer to segment list for filling that's already
   occurred on line before ForwardLimit, used to add any
   new segments when limit hit */
SBlock *FLimitPtr;          

/* becomes FLimitPtr if direction is reversed */
SBlock *BLimitPtr;          

/* scan line currently being scanned */
INT32  WorkingScanLine;     

/* X of point to qualify */
INT32  qualifyX;            

/* working pointer to SBlock that's head of segment list */
SBlock *TempListHead;       

/* offset of sentinel for current segment list */
SBlock *CurrentSegmentList; 

/* fill limit rect (X-style) */
rect   lmtR;                

/* offset of state stack top */
SBlock *StateStackPtr;      

/* pointer to current shadow segment list */
SBlock *pShadow;            

/* pointer to start of as-yet unallocated portion of
   memory pool */
SBlock *Unallocated;        

/* pointer to last byte from which SBlocks can be allocated in memory pool */
SIGNED stkBufCtr;
INT32  wXmin;
SBlock *blockPtr;
SBlock *blockPtrBE;
SBlock *blockPtrCE;
SBlock *blockPtrDN;
SBlock *blockPtrST;


extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);


/* Local Functions */
STATUS RS_OptionalFiller( OptionalFillers fill, INT32 xStart, INT32 yStart,
                          INT32 bColor, rect *fillerLimitRect);
VOID  DoSeedFill(VOID);
INT32 NotMatchBoundaryColor(VOID);
INT32 AllocBlock(SBlock *UnallocatedEnd);
INT32 MatchSeedColor(VOID);
INT32 ScanOutSegList(SBlock *UnallocatedEnd);
INT32 PopState(VOID);
INT32 PushState(SBlock *UnallocatedEnd);
VOID  FreeList(VOID);
INT32 Setup1(VOID);

/* routine to call to determine if a pixel qualify as fillable */
INT32 (*Qualify)(VOID);        

/* vector to GetPixel primitive for the destination bitmap */
SIGNED (*GetPixelPrimitive)(); 

/* vector to Fill primitive for the destination bitmap */
VOID (*FillPrimitive)();       

#endif /* _OPTFILL_H_ */

