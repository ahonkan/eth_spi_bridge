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
*  regmerge.c                                                   
*
* DESCRIPTION
*
*  This file contains the REGMERGE_rsMergeRegion function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  REGMERGE_rsMergeRegion
*  SetupEmptyRegion
*  HandleChange
*
* DEPENDENCIES
*
*  rs_base.h
*  regmerge.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/regmerge.h"

/***************************************************************************
* FUNCTION
*
*    REGMERGE_rsMergeRegion
*
* DESCRIPTION
*
*    Function REGMERGE_rsMergeRegion() merges one or two YX sorted rectangle lists to 
*    create an new region rectangle list (also YX banded and sorted). NUMRECT1 and 
*    NUMRECT2 define the number of rectangles contained RECTLIST1 and RECTLIST2,
*    respectively.  Rectangle lists passed to REGMERGE_rsMergeRegion() must be YX sorted
*    on the upper left corner, and must be in global coordinates.  A zero (0)
*    value for NUMRECT2 and "NULL" pointer for RECTLIST2 may be passed to create
*    a region from the sorted rectangles in RECTLIST1 only.
*
*    SIZERGN specifies the number of bytes available in the destination region,
*    DESTRGN.  To determine the required space for a region, a "NULL" DESTRGN 
*    pointer with a zero (0) SIZERGN value may be passed (no region is created,
*    only the size required is returned).
*
*    (For the region sizing case, the general memory pool in GRAFIX_DATA is used
*    to store the current YX band if and only if the general pool is larger than
*    half the size of the buffer allocated on the stack; otherwise, half the
*    stack buffer is used for storing the current YX band, and the other half is
*    used for the AET. If the general memory pool is used, or when a region is
*    actually being built, then the entire stack buffer is used for the AET. It
*    would be nice if the AET could be stored in the memory pool when it was
*    larger, so that larger numbers of rects on the same scan line could be
*    handled, but it's really handy to have the AET in SS.  Beside, we *do*
*    handle up to 768 edges (384 rects) on one scan line, which should be
*    adequate for most purposes!)
*
*    RGNOP specifies the region merge operation to be performed:
*
*        0   Union (combine) RECTLIST1 with RECTLIST2.
*        1   Intersection of RECTLIST1 and RECTLIST2.
*        2   Subtract - RECTLIST1 with RECTLIST2 subtracted from it.
*        3   XOR - areas of RECTLIST1 and RECTLIST2 not in both.
*
*    REGMERGE_rsMergeRegion() returns returns two types of values dependent on if a
*    DESTRGN pointer is passed or not (NULL).  If SIZERGN is zero and DESTRGN
*    is NULL, REGMERGE_rsMergeRegion() computes and returns the size (number of bytes)
*    needed for DESTRGN.  If SIZERGN is non-zero and DESTRGN in not NULL, 
*    REGMERGE_rsMergeRegion() returns the GrafErr status from creating the new region: 0 = no
*    errors, non-0 = GrafError() value.
*
* INPUTS
*
*    INT32 numRects1 - Number of rectangles in list 1.
*
*    rect *rectList1 - Pointer to the rectangle list 1.
*
*    INT32 numRects2 - Number of rectangles in list 2.
*
*    rect *rectList2 - Pointer to the rectangle list 1.
*
*    INT32 sizeRGN   - Number of bytes available in the destination region,
*
*    region *destRGN - Pointer to the destination region.
*
*    INT32 rgnOP     - Specifies the region merge operation to be performed:
*                          0 Union (combine) RECTLIST1 with RECTLIST2.
*                          1 Intersection of RECTLIST1 and RECTLIST2.
*                          2 Subtract - RECTLIST1 with RECTLIST2 subtracted from it.
*                          3 XOR - areas of RECTLIST1 and RECTLIST2 not in both.
*
* OUTPUTS
*
*    INT32           - Returns the GrafErr status from creating the new region:
*                      0 = no errors.
*                      Else returns GrafError() value.
*
***************************************************************************/
UINT32 REGMERGE_rsMergeRegion( INT32 numRects1, rect *rectList1,
                      INT32 numRects2, rect *rectList2,
                      INT32 sizeRGN, region *destRGN, INT32 rgnOP)
{
    INT16  Done             = NU_FALSE;
    INT16  JumpFinishRegion = NU_FALSE;
    INT16  JumpDoRectList2  = NU_FALSE;
    INT16  JumpHaschanged   = NU_FALSE;
    INT16  JumpAETScanDone  = NU_FALSE;
    INT16  JumpStoreNewRect = NU_FALSE;
    INT16  JumpScanOutFirst = NU_FALSE;
    UINT8  memPool[STACK_BUFFER_SIZE_FOR_REGION];    
    signed char   makeRegion;
    INT32  destSizeLeft;
    INT32  baseDestSize;
    UINT32 regionSize;
    INT32  rXmin, rXmax;
    rect   *destScanMax;
    INT32  curtY;
    SIGNED freeBlocks;
    INT32  rlistID;
    INT32  edgeState;
    INT16  grafErrValue;
    INT32  RLmin;
    rect   *temRect;
    INT32  temNUM;
    INT32  ret_val;  
    INT16  alTest,ahTest;
    struct RBlock *dnPtr;
    struct RBlock *snPtr;
    struct RBlock aetHead;              
    struct RBlock *curtLeftEdge;
    struct RBlock *bePtr;
    rect *destPtr;
    rect *destSave;

    /* To remove paradigm warning */
    (VOID)JumpStoreNewRect;

    baseDestSize = mpWorkEnd - mpWorkSpace;

    if( destRGN == 0 )
    {
        /* Mark that not building region, just find how large the 
           buffer. If the general memory pool is bigger than half
           the stack, store the current YX band.  */
        makeRegion = 0;

        /* If smaller than half split stack memPool */  
        if( baseDestSize <= (STACK_BUFFER_SIZE_FOR_REGION/2) )
        {
            baseDestSize = STACK_BUFFER_SIZE_FOR_REGION/2;
            destPtr = (rect *)(&memPool[baseDestSize]);
            temNUM = baseDestSize / sizeof(struct RBlock);
        }
        else
        {
            temNUM = STACK_BUFFER_SIZE_FOR_REGION / sizeof(struct RBlock);
            destPtr = (rect *) mpWorkSpace;
        }
    }
    /*********** Setup to build a region **********/
    else
    {
        makeRegion = 1;
        destSizeLeft = sizeRGN - sizeof(region) - sizeof(rect);
        if ( destSizeLeft < 0 )
        {
            grafErrValue = c_RectList + c_RgnOflow;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 

            SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
            regionSize = 0;
            Done = NU_TRUE;
        }

        if( !Done )
        {
            destRGN->rgnAlloc = sizeRGN;

            destPtr = (rect *)((SIGNED) destRGN + sizeof(region));
 
            /* value with which will fill initial sentinel */
            destPtr->Xmin = rgnBOS;
            destPtr->Xmax = rgnBOS;
            destPtr->Ymin = rgnBOS;
            destPtr->Ymax = rgnBOS;
            destPtr++;

            temNUM = STACK_BUFFER_SIZE_FOR_REGION/sizeof(struct RBlock);
        }
    } /* else */

    if( !Done )
    {
        destScanMax = destPtr;
        destSave = destPtr;
        regionSize = sizeof(region) + sizeof(rect);

        /* Allocate CE rects at memPool entirely to free list */
        freeBlocks = (SIGNED) memPool;
        temNUM--;

        /* Point to the next linked block */
        /* start of AET space Block  */
        snPtr = (struct RBlock *) memPool;    
        do
        {
            /* snPtr->NextBlock = ++snPtr */
            snPtr->NextBlock = snPtr + 1; 
            snPtr = snPtr->NextBlock;
        } while( --temNUM > 0 );

        /* Mask the last available block */
        snPtr->NextBlock = 0;

        aetHead.NextBlock = &aetHead;

        /* Point to itself so aetHead is also the tail */
        aetHead.rX = 32767;

        /* end of list is a right edge because sort left edges to the 
        left of right edges at equal X coordinates  */   
        aetHead.rDir = (signed char)RGHT_EDGE;

        /* Initial X range so that the first valid rect will set it */
        rXmin =  32767;
        rXmax = -32768;

        /********** Verify rectangle lists to insure they are sorted  */
        if( (numRects1 | numRects2) == 0 )
        {
            JumpFinishRegion = NU_TRUE;
        }

    } /* if( !Done ) */

    if( !Done && !JumpFinishRegion )
    {
        /********** Determine min Y in list 1 or 2  *********/
        curtY = 32767;  
    
        if( numRects1 != 0 )
        {
            curtY = rectList1->Ymin;
        }
        if( (numRects2 != 0) && (rectList2->Ymin < curtY) )
        {
            curtY = rectList2->Ymin;
        }
        
        /* Add any rects of positive width and height on the current
           scan line from the build rect list to the AET in x-sorted 
           order; advance build rect list pointer  */

        JumpScanOutFirst = NU_TRUE;
        while( JumpScanOutFirst )
        {   
            JumpScanOutFirst = NU_FALSE;
            if( numRects1 == 0 )
            {
                JumpDoRectList2 = NU_TRUE;
            }
            else
            {
                temRect = rectList1;
                rlistID = RECT_LST1;
                curtLeftEdge = &aetHead;
            }
            
            while(1)
            {
                if( !JumpDoRectList2 && temRect->Ymin == curtY )
                {
                    if( (temRect->Ymax > temRect->Ymin) &&
                        (temRect->Xmax > temRect->Xmin)  )
                    {
                        /* Take the Minimum value for Xmin */
                        if( temRect->Xmin < rXmin)
                        {
                            rXmin = temRect->Xmin;
                        }
                        bePtr = curtLeftEdge;
                        do
                        {
                            snPtr = bePtr;
                            bePtr = snPtr->NextBlock;
                        } while( bePtr->rX < temRect->Xmin );

                        bePtr = (struct RBlock *) freeBlocks;
                        if( freeBlocks == NU_NULL )
                        {
                            /* There is no space left then get out */
                            grafErrValue = c_RectList + c_OutofMem;
                            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                            SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                            regionSize = 0;
                            Done = NU_TRUE;
                            break; /* while(1) */
                        }
                        freeBlocks = (SIGNED) bePtr->NextBlock;

                        /* Set the edge X coord to left edge of rect */
                        bePtr->rX = temRect->Xmin;
                        bePtr->rCount = temRect->Ymax - temRect->Ymin;
                        bePtr->rDir = LEFT_EDGE;
                        bePtr->rList = rlistID;

                        /* Link preceding AET block to new one and new one to the next */
                        dnPtr = snPtr->NextBlock;
                        snPtr->NextBlock = bePtr;
                        bePtr->NextBlock = dnPtr;

                        /* Remember where to start scanning for the insertion point */
                        curtLeftEdge = bePtr;

                        /* Do link for the right Edge */
                        /* Link in the right edge and find insertion point for Xmax edge  */
                        if( temRect->Xmax  > rXmax )
                        {
                            rXmax = temRect->Xmax;
                        }

                        /* Start scanning right after the left edge block just added. For sorting
                           order, right edges always go to the right of left edges when they have 
                           same X coordinates */
                        do
                        {
                            snPtr = bePtr;
                            bePtr = snPtr->NextBlock;
                        } while(  (bePtr->rX < temRect->Xmax) || 
                                 ((bePtr->rX == temRect->Xmax) &&
                                  (bePtr->rDir == LEFT_EDGE)));

                        bePtr = (struct RBlock *) freeBlocks;
                        if( freeBlocks == NU_NULL )
                        {
                            /* There is no space left then get out */
                            grafErrValue = c_RectList + c_OutofMem;
                            nuGrafErr(grafErrValue, __LINE__, __FILE__); 

                            SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                            regionSize = 0;
                            Done = NU_TRUE;
                            break; /* while(1) */
                        }

                        freeBlocks = (SIGNED) bePtr->NextBlock;
                        bePtr->rX = temRect->Xmax;
                        bePtr->rCount = temRect->Ymax - temRect->Ymin;
                        bePtr->rDir = (signed char)RGHT_EDGE;
                        bePtr->rList = (UINT8) rlistID;

                        /* Link preceding AET block to new one and new one to the next */
                        dnPtr = snPtr->NextBlock;
                        snPtr->NextBlock = bePtr;
                        bePtr->NextBlock = dnPtr;
                    } /* if( ) - end at AddRectLoopBottom */
                    temRect++;

                    if( rlistID == RECT_LST1 )
                    {
                        numRects1--;
                        if( numRects1 > 0 )
                        {
                            continue;
                        }
                    }
                    else
                    {
                        numRects2--;
                        if( numRects2 > 0 )
                        {
                            continue;
                        }
                    }
                } /* end at AddRectDone */
               
                
                if( !JumpDoRectList2 )
                {
                    if( rlistID != RECT_LST1 )
                    {
                        rectList2 = temRect;
                        break; /* while(1) */
                    }

                    rectList1 = temRect;
                }

                JumpDoRectList2 = NU_FALSE;
                if( numRects2 == 0)
                {
                    break; /* while(1) */
                }

                rlistID = RECT_LST2;
                temRect = rectList2;
                curtLeftEdge = &aetHead;

            } /* end while(1) */
    
            if( !Done )
            {
                /* Scan out the AET by widening the rule. If this scan-out proves
                   to be different from the last one, add a new YX band  */
                destPtr = destSave;
                snPtr = aetHead.NextBlock;
                bePtr = &aetHead;
                switch (rgnOP & 3)
                {
                case 0: /* UNION */
                    while(snPtr != bePtr)
                    {
                        /* Point to next edge  */           
                        alTest = LEFT_EDGE;

                        /* Save the current X coordinate of edge  */
                        temNUM = snPtr->rX; 
                        do
                        {
                            snPtr = snPtr->NextBlock;
                            alTest += snPtr->rDir;
                        } while( alTest != 0 );

                        /* Check at the end of the previous YX band? */
                        if( (destScanMax == destPtr)  ||
                            (destPtr->Xmin != temNUM) ||
                            (destPtr->Xmax != snPtr->rX ) )
                        {
                            JumpHaschanged = NU_TRUE;
                            break; 
                        }

                        destPtr++;
                        snPtr = snPtr->NextBlock;

                    } /* while(snPtr != bePtr) */
                    break; /* case 0: UNION */

                case 1: /* INTERSECTION */ 
                    /* Find the matching Left Edge  */
                    alTest = ahTest = 0;
                    while( snPtr != bePtr )
                    {
                        /* Check for the edge of List1 or List2 */
                        if( snPtr->rList == RECT_LST1 )
                        {
                            alTest += snPtr->rDir;
                            if( (alTest == 0) || (ahTest == 0) )
                            {
                                snPtr = snPtr->NextBlock;
                                continue;
                            }
                        }
                        else
                        {
                            ahTest += snPtr->rDir;              
                            if( (ahTest == 0) || (alTest == 0) )
                            {
                                snPtr = snPtr->NextBlock;
                                continue;
                            }
                        }

                        /* Find the matching Right Edge */
                        /* Check for the edge of List1 or List2 */
                        /* Save the current X coordinate of edge  */
                        temNUM = snPtr->rX; 

                        /* Point to the next edge  */
                        snPtr = snPtr->NextBlock;   

                        if( snPtr == bePtr )
                        {
                            /* End of AET  */
                            break; 
                        }

                        if( snPtr->rList == RECT_LST1 )
                        {
                            alTest += (INT16) snPtr->rDir;
                        }
                        else
                        {
                            ahTest += (INT16) snPtr->rDir;
                        }

                        /* Found the matching pair of edges */
                        edgeState = (ahTest << 8) + alTest;

                        if( (destScanMax == destPtr)  ||
                            (destPtr->Xmin != temNUM) ||
                            (destPtr->Xmax != snPtr->rX) )
                        {
                            JumpHaschanged = NU_TRUE;

                            /* while( snPtr != bePtr ) */
                            break; 
                        }
                        destPtr++;
                        snPtr = snPtr->NextBlock;
                    }
                    break; /* case 1: INTERSECTION */

                case 2: /* SUBTRACTION */ 
                    /* Find the matching Left Edge */
                    alTest = ahTest = 0;
                    while( snPtr != bePtr )
                    {
                        /* Check for the edge of List1 or List2 */
                        if( snPtr->rList == RECT_LST1 )
                        {
                            alTest += snPtr->rDir;
                            if( (alTest == 0) || (ahTest != 0) )
                            {
                                snPtr = snPtr->NextBlock;
                                continue;
                            }
                        }
                        else
                        {
                            ahTest += snPtr->rDir;
                            if( (ahTest != 0) || (alTest == 0) )
                            {
                                snPtr = snPtr->NextBlock;
                                continue;
                            }
                        }

                        /* Find the matching Right Edge */
                        /* Check for the edge of List1 or List2  */
                        /* Save the current X coordinate of edge  */
                        temNUM = snPtr->rX; 

                        /* Point to the next edge  */
                        snPtr = snPtr->NextBlock;   
                        if( snPtr == bePtr )
                        {
                            /* while( )   End of AET  */
                            break; 
                        }

                        if( snPtr->rList == RECT_LST1 )
                        {
                            alTest += (INT16) snPtr->rDir;
                        }
                        else
                        {
                            ahTest += (INT16) snPtr->rDir;
                        }

                        /* Found the matching pair of edges */
                        edgeState = (ahTest << 8) + alTest;

                        if( (destScanMax == destPtr)  ||
                            (destPtr->Xmin != temNUM) ||
                            (destPtr->Xmax != snPtr->rX) )
                        {
                            JumpHaschanged = NU_TRUE;

                            /* while( snPtr != bePtr ) */
                            break; 
                        }
                        destPtr++;
                        snPtr = snPtr->NextBlock;
                    }
                    break; /* case 2: SUBTRACTION */

                case 3: /* XOR */  
                    /* Find the matching Left Edge  */
                    alTest = ahTest = 0;
                    while( snPtr != bePtr )
                    {
                        /* Check for the edge of List1 or List2 */
                        if( snPtr->rList == RECT_LST1 )
                        {
                            alTest += snPtr->rDir;
                            if( alTest == 0 )
                            {
                                if( ahTest == 0 )
                                {
                                    snPtr = snPtr->NextBlock;
                                    continue;
                                }
                            }
                            else
                            {
                                if( ahTest != 0 )
                                {
                                    snPtr = snPtr->NextBlock;
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            ahTest += snPtr->rDir;
                            if( ahTest == 0 )
                            {
                                if( alTest == 0 )
                                {
                                    snPtr = snPtr->NextBlock;
                                    continue;
                                }
                            }
                            else
                            {
                                if( alTest != 0 )
                                {
                                    snPtr = snPtr->NextBlock;
                                    continue;
                                }
                            }
                        }

                        /* Find the matching Right Edge */
                        /* Check for the edge of List1 or List2 */
                        /* Save the current X coordinate of edge */
                        temNUM = snPtr->rX; 

                        /* Point to the next edge */
                        snPtr = snPtr->NextBlock; 
                        if( snPtr == bePtr )
                        {
                            /* End of AET  */
                            break; 
                        }

                        if( snPtr->rList == RECT_LST1 )
                        {
                            alTest += (INT16) snPtr->rDir;
                        }
                        else
                        {
                            ahTest += (INT16) snPtr->rDir;
                        }

                        /* Found the matching pair of edges */
                        edgeState = (ahTest << 8) + alTest;

                        if( (destScanMax == destPtr)  ||
                            (destPtr->Xmin != temNUM) ||
                            (destPtr->Xmax != snPtr->rX) )
                        {
                            JumpHaschanged = NU_TRUE;

                            /* while( snPtr != bePtr ) */
                            break; 
                        }
                        destPtr++;
                        snPtr = snPtr->NextBlock;

                    } /* while( snPtr != bePtr ) */

                } /* switch */

            } /* if( !Done ) */

            /* End DO JUMP ONE  */

            if( !Done && !JumpHaschanged )
            {
                /* CheckForChange */
                if( destScanMax != destPtr )
                {
                    ret_val = HandleChange(&destSizeLeft, destScanMax,
                                           baseDestSize, curtY, makeRegion,
                                           destPtr, destSave);
                    if( ret_val != 0 ) 
                    {
                        nuGrafErr((INT16) ret_val, __LINE__, __FILE__);
                        SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                        regionSize = 0;
                        Done = NU_TRUE;
                    }

                    if( !Done )
                    {
                        destScanMax = destPtr;
                        regionSize += ((SIGNED) destPtr - (SIGNED) destSave);
                        if( regionSize > 32767 )
                        {
                            grafErrValue = c_RectList + c_RgnOflow;
                            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                            SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                            regionSize = 0;
                            Done = NU_TRUE;
                        }
                    }
                } /* if( destScanMax != destPtr ) */
                JumpAETScanDone = NU_TRUE;

            } /* if( !Done && !JumpHaschanged ) */
            if( !Done && !JumpHaschanged )
            {
            JumpAETScanDone = NU_TRUE;
            }

            if( !Done && !JumpAETScanDone )
            {
                JumpHaschanged = NU_FALSE;
                ret_val = HandleChange(&destSizeLeft, destScanMax,
                                       baseDestSize, curtY, makeRegion,
                                       destPtr, destSave);

                if( ret_val != 0 ) 
                {
                    nuGrafErr((INT16) ret_val, __LINE__, __FILE__); 
                    SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                    regionSize = 0;
                    Done = NU_TRUE;
                }

                if( !Done )
                {
                    bePtr = &aetHead;
                    JumpStoreNewRect = NU_TRUE;
                    while( JumpStoreNewRect)
                    {
                        JumpStoreNewRect = NU_FALSE;
                        if( temNUM < snPtr->rX )
                        {
                            destSizeLeft -= sizeof(rect);
                            if( destSizeLeft < 0 )
                            {
                                /*Out of space in dest; return either out of memory error (if just finding
                                  size, because we ran out of single-YX-band buffer space), or region
                                  overflow (if building region, because we ran out of YX band storage). */

                                if( makeRegion == 0 )
                                {
                                    grafErrValue = c_RectList + c_OutofMem;
                                }
                                else
                                {
                                    grafErrValue = c_RectList + c_RgnOflow;
                                }

                                nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                                SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                                regionSize = 0;
                                Done = NU_TRUE;
                            }

                            if( !Done )
                            {
                                destPtr->Ymin = curtY;
                                destPtr->Xmin = temNUM;
                                destPtr->Xmax = snPtr->rX;
                                destPtr++;
                            }
                        } /* if( temNUM < snPtr->rX ) */

                        if( !Done )
                        {
                            snPtr = snPtr->NextBlock;

                            /* This suppose to be equal but not */ 
                            if( snPtr != bePtr )
                            {
                                switch (rgnOP & 3)
                                {
                                case 0: /* Do Union 2  */
                                    temNUM = snPtr->rX;
                                    alTest = LEFT_EDGE;
                                    do
                                    {
                                        snPtr = snPtr->NextBlock;
                                        alTest +=  (INT16) snPtr->rDir;
                                    } while( alTest != 0 );

                                    JumpStoreNewRect = NU_TRUE;

                                    /* switch (rgnOP & 3) */
                                    break; 

                                case 1: /* Do Intersection */
                                    alTest = edgeState & 0xff;
                                    ahTest = edgeState >> 8;
                                    while( snPtr != bePtr )
                                    {
                                        if( snPtr->rList == RECT_LST1 )
                                        {
                                            alTest += snPtr->rDir;
                                            if( (alTest == 0) || (ahTest == 0) )
                                            {
                                                snPtr = snPtr->NextBlock;
                                                continue;
                                            }
                                        }
                                        else
                                        {
                                            ahTest += snPtr->rDir;
                                            if( (ahTest == 0) || (alTest == 0) )
                                            {
                                                snPtr = snPtr->NextBlock;
                                                continue;
                                            }
                                        }

                                        temNUM = (INT16) snPtr->rX;
                                        snPtr = snPtr->NextBlock;

                                        if( snPtr == bePtr )
                                        {
                                            break;
                                        }

                                        if( snPtr->rList == RECT_LST1 )
                                        {
                                            alTest += (INT16) snPtr->rDir;
                                        }
                                        else
                                        {
                                            ahTest += (INT16) snPtr->rDir;
                                        }

                                        edgeState = (ahTest << 8) + alTest;
                                        JumpStoreNewRect = NU_TRUE;

                                        /* while( snPtr != bePtr ) */
                                        break; 

                                    }

                                    /* case 1: Do Intersection */
                                    break; 

                                case 2: /* Do Subtraction */
                                    alTest = edgeState & 0xff;
                                    ahTest = edgeState >> 8;
                                    while( snPtr != bePtr )
                                    {
                                        if( snPtr->rList == RECT_LST1 )
                                        {
                                            alTest += snPtr->rDir;
                                            if( (alTest == 0) || (ahTest != 0) )
                                            {
                                                snPtr = snPtr->NextBlock;
                                                continue;
                                            }
                                        }
                                        else
                                        {
                                            ahTest += snPtr->rDir;
                                            if( (ahTest != 0) || (alTest == 0) )
                                            {
                                                snPtr = snPtr->NextBlock;
                                                continue;
                                            }
                                        }

                                        temNUM = snPtr->rX;
                                        snPtr = snPtr->NextBlock;

                                        if( snPtr == bePtr )
                                        {
                                            break;
                                        }

                                        if( snPtr->rList == RECT_LST1 )
                                        {
                                            alTest += (INT16) snPtr->rDir;
                                        }
                                        else
                                        {
                                            ahTest += (INT16) snPtr->rDir;
                                        }

                                        edgeState = (ahTest << 8) + alTest;
                                        JumpStoreNewRect = NU_TRUE;

                                        /* while( snPtr != bePtr ) */
                                        break; 
                                    }

                                    /* case 2: Do Subtraction */
                                    break; 

                                case 3: /* Do XOR */
                                    alTest = edgeState & 0xff;
                                    ahTest = edgeState >> 8;
                                    while( snPtr != bePtr )
                                    {
                                        if( snPtr->rList == RECT_LST1 )
                                        {
                                            alTest += snPtr->rDir;
                                            if( alTest == 0 )
                                            {
                                                if( ahTest == 0 )
                                                {
                                                    snPtr = snPtr->NextBlock;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                if( ahTest != 0 )
                                                {
                                                    snPtr = snPtr->NextBlock;
                                                    continue;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            ahTest += snPtr->rDir;
                                            if( ahTest == 0 )
                                            {
                                                if( alTest == 0 )
                                                {
                                                    snPtr = snPtr->NextBlock;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                if( alTest != 0 )
                                                {
                                                    snPtr = snPtr->NextBlock;
                                                    continue;
                                                }
                                            }
                                        }

                                        temNUM = snPtr->rX;
                                        snPtr = snPtr->NextBlock;

                                        if( snPtr == bePtr )
                                        {
                                            break;
                                        }   

                                        if( snPtr->rList == RECT_LST1 )
                                        {
                                            alTest += (INT16) snPtr->rDir;
                                        }
                                        else
                                        {
                                            ahTest += (INT16) snPtr->rDir;
                                        }  

                                        edgeState = (ahTest << 8) + alTest;
                                        JumpStoreNewRect = NU_TRUE;

                                        /* while( snPtr != bePtr ) */
                                        break;  

                                    } /* while( snPtr != bePtr ) */

                                } /* switch (rgnOP & 3) */

                            } /* if( snPtr != bePtr ) */

                            /* Jump back to StorenewRect */
                            if( !JumpStoreNewRect )
                            {
                                destScanMax = destPtr;
                                regionSize += ((SIGNED) destPtr - (SIGNED) destSave);  

                                if( regionSize > 32767 )
                                {
                                    grafErrValue = c_RectList + c_RgnOflow;
                                    nuGrafErr(grafErrValue, __LINE__, __FILE__); 

                                    SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                                    regionSize = 0;
                                    Done = NU_TRUE;
                                }
                            } /* if( !JumpStoreNewRect ) */

                        } /* if( !Done */

                    } /* while( !JumpStoreNewRect) */

                } /* if( !Done ) */

            } /* if( !Done && !JumpAETScanDone ) */    
            if( !Done )
            {
                JumpAETScanDone = NU_FALSE;
                        if( ((numRects1 | numRects2) != 0) || ( aetHead.NextBlock != &aetHead) )
                        /* if not goto FinishRegion  */ 

                        /***********************************************************/
                        /*Main Loop for Constructing YX banded list from rect list */
                        /***********************************************************/
                        /* Length in AET, distance to top of next rect in build rect list)
                           (one of the two distances is guaranteed to exist, because we can
                           only get here from bottom of loop, which checks for exactly the
                           condition that there's either an AET or remaining build rects)  */
                        {       
                            /* Find shortest distance in AET */
                            snPtr = aetHead.NextBlock;

                            temNUM = 32767;
                            while( snPtr != &aetHead )
                            {
                                if( snPtr->rCount < temNUM )
                                {
                                    temNUM = (INT16) snPtr->rCount;
                                }
                                snPtr = snPtr->NextBlock;   
                            }

                            /* Find shortest distance in RectList1 or RectList2  */
                            if( (numRects1 | numRects2) != 0 )
                            {
                                if( numRects1 != 0 )
                                {
                                    if( (numRects2 != 0) && (rectList2->Ymin < rectList1->Ymin) )
                                    {
                                        RLmin = rectList2->Ymin;
                                    }
                                    else
                                    {
                                        RLmin = rectList1->Ymin;
                                    }
                                }
                                else
                                {
                                    RLmin = rectList2->Ymin;
                                }

                                /* temNUM has the shortest AET or RectList distance  */
                                RLmin -= curtY;

                                if(RLmin < temNUM)
                                {
                                    temNUM = RLmin;
                                }
                            }

                            /* Shortest AET or RectList distance to advance  */
                            /* Advance Y to the top of the band  */
                            curtY += temNUM; 

                            snPtr = aetHead.NextBlock;
                            dnPtr = &aetHead;
                            while( snPtr != &aetHead )
                            {
                                snPtr->rCount -= temNUM;

                                /* Still length remain  */
                                if( snPtr->rCount == 0 ) 
                                {
                                    /* Link the previous block to the next block  */
                                    bePtr = snPtr->NextBlock;
                                    dnPtr->NextBlock = bePtr;
                                    curtLeftEdge = snPtr;
                                    snPtr->NextBlock = (struct RBlock *) freeBlocks;
                                    freeBlocks = (SIGNED) curtLeftEdge;
                                    snPtr = bePtr;
                                }
                                else
                                {
                                    dnPtr = snPtr;
                                    snPtr = snPtr->NextBlock;
                                }
                            } /* while( snPtr != &aetHead ) */

                    JumpScanOutFirst = NU_TRUE;
                } /* if( ((numRects1 | numRects2) != 0) || ( aetHead.NextBlock != &aetHead) ) */

            } /* if( !Done ) AETScanDone: */


        } /* while( JumpScanOutFirst ) */


    } /* if( !Done && !JumpFinishRegion ) */



    JumpFinishRegion = NU_FALSE;

    /* Done building YX banded rect list form input rect list 
       add the tail sentinel and fill in the region structure  */
    if( !Done )
    {
        if( makeRegion == 1 )
        {
            destSizeLeft -= sizeof(rect);
            if( destSizeLeft < 0 )
            {
                /* Not enough space for the end rect  */
                grafErrValue = c_RectList + c_RgnOflow;
                nuGrafErr(grafErrValue, __LINE__, __FILE__); 

                /* Set up the end rect  */
                SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
                regionSize = 0;
                Done = NU_TRUE;
            }

            if( !Done )
            {
                /* Fill the last rect  */   
                destSave->Xmin = rgnEOS;    
                destSave->Xmax = rgnEOS;
                destSave->Ymin = rgnEOS;
                destSave->Ymax = rgnEOS;
            }

        } /* if( makeRegion == 1 ) */

    } /* if( !Done ) */

    if( !Done )
    {
        /* Count the tail sentinel  */
        regionSize += sizeof(rect);
        if( regionSize > 32767 )
        {
            grafErrValue = c_RectList + c_RgnOflow;
            nuGrafErr(grafErrValue, __LINE__, __FILE__);

            SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
            regionSize = 0;
            Done = NU_TRUE;
        }
    } /* if( !Done ) */

    if( !Done )
    {
        /* Set up more space before return  */
        if( (rXmin > rXmax) || (regionSize == (sizeof(region) + 2*sizeof(rect))) )
        {
            SetupEmptyRegion(destRGN, makeRegion, sizeRGN);
        }
        else
        {
            if( makeRegion != 0 )
            {
                if( regionSize  == (sizeof(region) + 3*sizeof(rect)) )
                {
                    destRGN->rgnFlags = rfRect;
                }
                else
                {
                    destRGN->rgnFlags = 0;
                }

                destRGN->rgnSize = regionSize;
            
                /* set bounding rects  */
                destRGN->rgnRect.Xmin = rXmin;  
                destRGN->rgnRect.Xmax = rXmax; 
                temRect = (rect *)((SIGNED) destRGN + 
                        sizeof(region) + sizeof(rect));
                destRGN->rgnRect.Ymin = temRect->Ymin;  
                destRGN->rgnList = (rect *) ((SIGNED) destRGN 
                        + sizeof(region) + sizeof(rect));           
                destRGN->rgnListEnd = destPtr - 1;  
                destRGN->rgnRect.Ymax = destRGN->rgnListEnd->Ymax; 
            } /* if( makeRegion != 0 ) */
        } /* else */

    } /* if( !Done ) */

    return(regionSize);
}

/***************************************************************************
* FUNCTION
*
*    SetupEmptyRegion
*
* DESCRIPTION
*
*    If region building is enabled, makes a NULL region at 
*    DESTRGN, followed by a NULL region list, if there's enough 
*    room.
*
* INPUTS
*
*    region *destRGN  - Pointer to the destination region.
*
*    INT32 makeRegion - Set to 1 to enable.
*
*    INT32 sizeRGN    - Sign of region.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetupEmptyRegion(region *destRGN, INT32 makeRegion, INT32 sizeRGN)
{
    region *tem;    

    if( makeRegion == 1 )
    {
        if( sizeRGN >= (INT32)sizeof(region) )
        {
            destRGN->rgnSize = sizeof(region);
    
            /* It is a null region (empty)  */
            destRGN->rgnFlags = (INT16)  rfNull;
                
            destRGN->rgnRect.Xmin = 0;
            destRGN->rgnRect.Xmax = 0;
            destRGN->rgnRect.Ymin = 0;
            destRGN->rgnRect.Ymax = 0;
    
            /* No viable pointers to region data  */
            destRGN->rgnList = 0;
            destRGN->rgnListEnd = 0;

            if( sizeRGN >= (INT32)(sizeof(region) + 2*sizeof(rect)) )  
            {
                destRGN->rgnSize = (sizeof(region) + 2*sizeof(rect));
            
                /* point to initial and tail sentinel  */
                tem = destRGN + 1;
                destRGN->rgnListEnd = (rect *) tem;
                destRGN->rgnList = (rect *) ((SIGNED)tem + sizeof(rect));

                /* Fill the tail sentinel  */
                destRGN->rgnList->Xmin = rgnEOS;    
                destRGN->rgnList->Xmax = rgnEOS;
                destRGN->rgnList->Ymin = rgnEOS;
                destRGN->rgnList->Ymax = rgnEOS;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    HandleChange
*
* DESCRIPTION
*
*    To be called immediately when it is detected that the current YX band
*    differs from the previous YX band. Finishes setting the previous band (sets
*    Ymax), copies the identical portion of the previous band to a new band,
*    sets the new band's Ymin, and adjusts pointers.
*
*    On entry, ES:DN points to the rect after the last rect in the previous YX
*    band that matched the corresponding rect in the current YX band.
*
*    On exit, ES:DN points to storage location for next YX band rect. DS is
*    preserved. Other registers are not preserved.
*    Returns AE = 0 for success, = error code for GrafErr if error occurred.
*
* INPUTS
*
*    INT32 *destSizeLeft - Pointer to the size left.
*
*    rect *destScanMax   - Pointer to the destination scan maximum size.
*
*    INT32 baseDestSize  - Base destination size.
*
*    INT32 curtY         - Current destination maximum y value.
*
*    INT32 makeRegion    - 0 to enable.
*
*    rect *destPtr       - Destination Pointer.
*
*    rect *destSave      - Previously saved destination pointer.
*
* OUTPUTS
*
*    INT32               - Returns 0 for success.
*                          Else returns a error code for GrafErr.
*
***************************************************************************/
INT32 HandleChange(INT32 *destSizeLeft, rect *destScanMax, INT32 baseDestSize, 
                   INT32 curtY, INT32 makeRegion, rect *destPtr, rect *destSave)
{
    INT16  Done = NU_FALSE;
    INT32  value;
    SIGNED temNUM;
    INT16  grafErrValue;
    rect   *tmpDestPtr;
    

    temNUM = ((SIGNED) destPtr - (SIGNED) destSave);
    if( makeRegion == 0 )
    {
        *destSizeLeft = baseDestSize - temNUM; 
        value = 0;
        Done  = NU_TRUE;
    }
    else
    {   
        *destSizeLeft -= temNUM; 
        if( *destSizeLeft < 0 )
        {
            grafErrValue = c_RectList + c_RgnOflow;
            value = grafErrValue;
            Done  = NU_TRUE;
        }
        else if( !Done )
        {
            tmpDestPtr = destSave;
            destPtr = destSave;
            while( destPtr != destScanMax )
            {
                destPtr->Ymax = curtY;
                destPtr++;
            }

            destSave = destScanMax;
            while( temNUM > 0 )
            {
                *destPtr = *tmpDestPtr;
                destPtr->Ymin = curtY;
                destPtr++;
                tmpDestPtr++;
                temNUM -= sizeof(rect);
            }

            value = 0;
        }

    } /* else */

    /* to remove paradigm warning */
    (VOID)Done;
    return(value);
}
