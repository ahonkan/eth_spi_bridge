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
*  optfill.c                                                    
*
* DESCRIPTION
*
*  FloodFill and BoundaryFill functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_OptionalFiller
*  DoSeedFill
*  NotMatchBoundaryColor
*  AllocBlock
*  MatchSeedColor
*  ScanOutSegList
*  PopState
*  PushState
*  FreeList
*  Setup1
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  optfill.h
*  gfxstack.h
* 
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/optfill.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    RS_OptionalFiller
*
* DESCRIPTION
*
*    This can be called if you would like to FloodFill or BoundaryFill an object.
*    This is also were you would add an call to a new filler function that is user created.
*
*    Then based off of what is passed into the function you will then send it to the "action"
*    function that going to be used. 
*
*    NOTE: The graphics port must already be set up and it should be global.
*          This should be called grafPort.
*
* INPUTS
*
*    OptionalFillers fill  - This is for optional filler, this will be set up in 
*                            an enumerate type.
*                            EX. FLOOD
*                                BOUNDARY
*
*    INT32 xStart          - This the x starting point.
*
*    INT32 yStart          - This is the y starting point.
*
*    INT32 bColor          - The boundary color if needed for optional fill.
*                            The flood fill does not require a boundary color so 
*                            the default is set to 
*                            DEFAULT_VALUE which at this time is -1 (global.h).
*
*    rect *fillerLimitRect - This is the rectangle that is used for the 
*                            optional filler functions. This rectangle is 
*                            usually different from the original rectangle 
*                            that is being used by the NU_Arc_Manipulate function.
*
* OUTPUTS
*
*    STATUS                - NU_SUCCESS if successful.
*
***************************************************************************/
STATUS RS_OptionalFiller( OptionalFillers fill,
                         INT32 xStart, INT32 yStart, INT32 bColor, rect *fillerLimitRect)
{
    STATUS  status = ~NU_SUCCESS;
    INT32   shfCnt;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* fill limit rectangle */
    LimitPtr = fillerLimitRect; 

    /* set up limit rect, blitRcd, and primitive GetPixel and Fill vectors,
       convert the seed point to global coords, check that it's inside the limit
       rect, and return the seed point */
    localFeedX = xStart;
    localFeedY = yStart;

    if( Setup1() == 0 )
    {
        /* exit if seed point isn't within limit rect */
        status = NU_SUCCESS; 
    }

    switch(fill)
    {
        case FLOOD:
            if( status != NU_SUCCESS )
            {
                /* get the seed pixel color */
                SeedColor = GetPixelPrimitive(localFeedX, localFeedY, blitRecord.blitDmap);

                /* reject if fill color is solid and matches seed color */
                if( (blitRecord.blitPat == 1) & (SeedColor == blitRecord.blitFore) )
                {
                    status = NU_SUCCESS;
                }

                if( status != NU_SUCCESS )
                {
                    if( (blitRecord.blitPat == 0) & (SeedColor == blitRecord.blitBack) )
                    {
                        status = NU_SUCCESS;
                    }

                    if( status != NU_SUCCESS )
                    {
                        /* pixel is fillable if matches seed color */
                        Qualify = MatchSeedColor;

                        /* same as boundary fill from now on */
                        DoSeedFill();
                        status = NU_SUCCESS;
                    }
                }
            }
            break;

        case BOUNDARY:
            /* Why are we doing this */
            /* boundary color */
            boundaryColor = bColor; 

            /* get modulus of boundary color and bitmap's max color index */
            if( blitRecord.blitDmap->pixPlanes == 1 )
            {
                shfCnt = blitRecord.blitDmap->pixBits;
            }
            else
            {
                shfCnt = blitRecord.blitDmap->pixPlanes;
            }

            boundaryColor = bColor & ( (1 << shfCnt) - 1);

            /* pixel is fillable if it doesn't match the boundary color */
            Qualify = NotMatchBoundaryColor;    

            /* same as flood fill from now on */
            DoSeedFill();
            status = NU_SUCCESS;

            /* to remove paradigm warning */
            (VOID)status;
        default:
            status = 0;
            break;
    }

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();
    
    /* Return to user mode */
    NU_USER_MODE();

    return(status);
}

/***************************************************************************
* FUNCTION
*
*    DoSeedFill
*
* DESCRIPTION
*
*    Module SEED1 is the seed fill (flood fill & boundary fill) code.
*    To speed up:
*       1) When scanning to the end of a segment, could calculate max distance
*       to the left or right limit edge and use that as a loop count, rather
*       than performing a clip check after each pixel.
*
*    #define MAX_RECTS_FOR_OPTFILL   4
*
*    This is how many segments can be drawn with a single call to the filler.
*    Making this larger may improve performance a tad, but will steal space from
*    the segment list. Making it smaller will hurt performance a little. This is
*    just a compromise value, which I figure gets rid of up to 75% of the filler
*    overhead while using just 24 extra bytes from the memory pool.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DoSeedFill(VOID)
{
    INT16 Done             = NU_FALSE;
    INT16 JumpDrawLoopDone = NU_FALSE;
    INT16 JumpCheckToLeft  = NU_FALSE;
    INT16 JumpDrawList     = NU_FALSE;
    INT16 JumpContinue     = NU_FALSE;
    INT32 i;
    INT16 scanLineP1;
    INT16 tempEdge;
    INT16 grafErrValue;
    UINT8  *stkBufPtr;
    rect   *sRectPtr;
    SBlock *UnallocatedEnd;
    SIGNED  blitList;

    /* working storage */
    UINT8 stackBuffer[STACK_BUFFER_SIZE_FOR_OPTFILL]; 

    /* to remove paradigm warning */
    (VOID)JumpDrawList;
    (VOID)JumpCheckToLeft;

    /* decide which buffer to use, stack or memory pool */
    /* assume using general memory pool */
    stkBufPtr = mpWorkSpace;    
    stkBufCtr = (SIGNED) (mpWorkEnd - stkBufPtr);

    /* is the memory pool smaller than */
    if (stkBufCtr <= STACK_BUFFER_SIZE_FOR_OPTFILL) 
    {   
        /* the stack buffer? */
        /* set up to use the stack buffer */
        stkBufCtr = STACK_BUFFER_SIZE_FOR_OPTFILL;  
        stkBufPtr = &stackBuffer[0];
    }

    /* save area for blitList rects */
    blitList = (SIGNED) stkBufPtr;

    /* first rect goes right at the start now
       split up the pool; MAX_RECTS_FOR_OPTFILL rects for
       blitList rects, the rest for SBlocks */
    sRectPtr = (rect *) ((SIGNED) stkBufPtr); 

    Unallocated = (SBlock *) ((SIGNED)(sRectPtr + MAX_RECTS_FOR_OPTFILL));
    UnallocatedEnd = (SBlock *) ((SIGNED)(Unallocated - 1) + stkBufCtr
                              - MAX_RECTS_FOR_OPTFILL * sizeof(rect));

    /* empty the free block list to start */
    FreeBlocks = NU_NULL;  

    /* set up to jump into filling as if we've just scanned up from the
       line below the seed point */
    /* mark that the state stack is empty */
    StateStackPtr = NU_NULL;           

    /* initial move direction is up */
    sDirection = -1;                

    /* stop proceeding upward when we reach this line */
    ForwardLimit  = lmtR.Ymin - 1;  

    /* stop proceeding downward when we reach this line */
    BackwardLimit = lmtR.Ymax;      

    /* NULL pointer, because these lists never get added to */
    FLimitPtr     = 0;                  
    BLimitPtr     = 0;

    /* get a sentinel and a single additional block for the fake 1-point
       shadow we're about to use as a basis for scanning out the seed line */
    AllocBlock(UnallocatedEnd);

    /* make it a sentinel */
    blockPtr->sXmin = 0x7FFF;         
    blockPtr->sXmax = 0x7FFF;

    /* remember where head is */
    blockPtrDN = blockPtr;            

    /* get a block for the seed point */
    AllocBlock(UnallocatedEnd);                     
    blockPtrDN->NextBlock = blockPtr;

    /* make it a circular list */
    blockPtr->NextBlock = blockPtrDN; 

    /* this will become the shadow for scanning out the seed line */
    pShadow = blockPtrDN;             

    /* we'll scan from the seed point (yes, this does mean that we read the
       seed point a second time for flood fills; that's life)*/
    blockPtr->sXmin = SeedX;
    blockPtr->sXmax = SeedX + 1;

    /*scan left and right from the seed point to make the initial current
      fill list, consisting of exactly one segment (the seed point has
      already been checked to be fillable) */

    /* generate the one segment from the seed point */
    if( ScanOutSegList(UnallocatedEnd) == 0 )
    {
        /* not enough memory */
        grafErrValue = c_SeedFill +  c_OutofMem;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        Done = NU_TRUE;
    }
    else if( blockPtr->NextBlock == blockPtr)
    {
        /* empty segment list */
        Done = NU_TRUE;  
    }              
    /* (Seed point not fillable; can't happen with
        flood fill, but can with boundary fill) */

    if( !Done )
    {
        /* make the shadow list empty, so we can have an unobstructed backshadow
           from the seed line */
        /* link the sentinel to itself to make the shadow list empty */
        blockPtrBE         = pShadow->NextBlock;    
        pShadow->NextBlock = pShadow;

        /* link it to the free list */
        blockPtrBE->NextBlock = FreeBlocks;

        /* and link the free list to it */
        FreeBlocks = blockPtrBE;            
    }

    /* Draw the segment list (build blitList, flushing when full).
       Assumes non-empty segment list. */
    JumpDrawList = NU_TRUE;

    while( !Done && JumpDrawList )
    {
        JumpDrawList = NU_FALSE;
        
        CurrentSegmentList = blockPtr;

        /* point to first segment */
        blockPtr = blockPtr->NextBlock; 
        scanLineP1 = ScanLine + 1;

        i = blitRecord.blitCnt;
        if( i == MAX_RECTS_FOR_OPTFILL)
        {
            /* draw one full burst, and resets for the next one */
            FillPrimitive(&blitRecord);
            sRectPtr = (rect *) (blitList);
            i = 0;
        }

        while(1)
        {
            for( ;i < MAX_RECTS_FOR_OPTFILL; i++)
            {
                /* rect left edge */
                sRectPtr->Xmin = blockPtr->sXmin;   

                /* rect right edge */
                sRectPtr->Xmax = blockPtr->sXmax;   

                /* rect top edge */
                sRectPtr->Ymin = ScanLine;                  

                /* rect bottom edge */
                sRectPtr->Ymax = scanLineP1;                

                /* point to the next rect in blitList */
                sRectPtr++;

                /* point to next segment */
                blockPtr = blockPtr->NextBlock;

                /* sentinel? */
                if( blockPtr->sXmin == 0x7FFF )
                {
                    JumpDrawLoopDone = NU_TRUE;
                    break; 
                }
            }

            if( JumpDrawLoopDone )
            {
                break; 
            }

            /* draw one full burst, and reset for the next one */
            blitRecord.blitCnt = MAX_RECTS_FOR_OPTFILL;
            FillPrimitive(&blitRecord);
            sRectPtr = (rect *) (blitList);
            i = 0;
        } /* while(1) */ 

        JumpDrawLoopDone = NU_FALSE;

        /* # of count of rects just processed */
        blitRecord.blitCnt = (INT16) ( ( ((SIGNED) sRectPtr) - blitList) >> 4 );

        /* now make up pShadow as the current segment list, minus the old
           shadow with each segment widened by 1 (this represents all the
           pixels we haven't already processed or eliminated in the backward dir) */
        /* start of old shadow list */
        blockPtrDN = pShadow->NextBlock;    

        /* get block for backshadow list header */
        if( AllocBlock(UnallocatedEnd) == NU_NULL )
        {
            /* not enough memory */
            grafErrValue = c_SeedFill +  c_OutofMem;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
            break; 
        }

        blockPtrBE = blockPtr;

        /* this will become the new shadow */
        pShadow = blockPtr;         

        /* also will link to next to become */
        pShadow->sXmin = 0x7FFF;    

        /* the sentinel */
        pShadow->sXmax = 0x7FFF;    

        /* start of current segment list */
        blockPtr = CurrentSegmentList->NextBlock;   

        while( (wXmin = blockPtr->sXmin) != 0x7FFF )
        {
            while( wXmin > blockPtrDN->sXmax )
            {
                /* skip through shadows until we find one that isn't
                   entirely to the left */
                blockPtrCE = blockPtrDN->NextBlock;
                blockPtrDN->NextBlock = FreeBlocks;
                FreeBlocks = blockPtrDN;
                blockPtrDN = blockPtrCE;
            } /* will terminate because of sentinel */

            JumpCheckToLeft = NU_TRUE;
            while( JumpCheckToLeft )
            {
                /* we've found a shadow that's not wholly to the left. We know
                   the current segment can't be to the right of the shadow (we just
                   checked that), so start by checking whether it's to the left */
                JumpCheckToLeft = NU_FALSE;

                if( (blockPtr->sXmax >= blockPtrDN->sXmin) && 
                    (blockPtrDN->sXmin != 0x7FFF) )
                {
                    /* the segment and the shadow overlap */
                    /* right edge */
                    tempEdge = (INT16)(blockPtrDN->sXmin - 1);   
                    if( wXmin < tempEdge )
                    {
                        /* make the part of the segment that sticks out to the
                           left of the shadow into a new segment */
                        /* save current pointer */
                        blockPtrST = blockPtr;      

                        /* get block for this segment */
                        if( AllocBlock(UnallocatedEnd) == NU_NULL )
                        {
                            /* not enough memory */
                            grafErrValue = c_SeedFill +  c_OutofMem;
                            nuGrafErr(grafErrValue, __LINE__, __FILE__);
                            Done = NU_TRUE;
                            break; 
                        }

                        /* restore our place in current list */
                        blockPtrCE = blockPtr;
                        blockPtr = blockPtrST;

                        /* link to the backshadow list */
                        blockPtrBE->NextBlock = blockPtrCE;

                        /* point to the new block */
                        blockPtrBE = blockPtrCE;
                        blockPtrBE->sXmin = wXmin;
                        blockPtrBE->sXmax = tempEdge;
                    }

                    /* figure out whether the current seg extends to the right of
                       this shadow segment and handle accordingly */
                    if( (blockPtr->sXmax - 1) <= blockPtrDN->sXmax )
                    {
                        /* the current seg doesn't reach beyond this shadow seg
                           and we're done with it; time to do the next segment */
                        blockPtr = blockPtr->NextBlock;
                        JumpContinue = NU_TRUE;
                        break; 
                    }

                    /* we're done with the shadow and can trim the segment to the
                       part that extends past the shadow and continue with that */
                    wXmin = blockPtrDN->sXmax + 1;
                    blockPtrCE = blockPtrDN->NextBlock;
                    blockPtrDN->NextBlock = FreeBlocks;
                    FreeBlocks = blockPtrDN;
                    blockPtrDN = blockPtrCE;

                    JumpCheckToLeft = NU_TRUE;
                    /* we know there's no shadow entirely to
                       the left of this segment, because we're smack dab next to the
                       previous shadow, and the next shadow has to be to the right of
                       that shadow, so we can skip that check */
                } /* if( ) */

            } /* while( JumpCheckToLeft ) */

            if( Done )
            {
                /* while( (wXmin = blockPtr->sXmin) != 0x7FFF ) */
                break; 
            }

            if( JumpContinue )
            {
                JumpContinue = NU_FALSE;
                continue;
            }

            /* save block pointer */
            blockPtrST = blockPtr;

            /* get block for this segment */
            if( AllocBlock(UnallocatedEnd) == NU_NULL )
            {
                /* not enough memory */
                grafErrValue = c_SeedFill +  c_OutofMem;
                nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                Done = NU_FALSE;
                
                /* while( (wXmin = blockPtr->sXmin) != 0x7FFF ) */
                break; 
            }

            /* restore our place in current list */
            blockPtrCE = blockPtr;
            blockPtr   = blockPtrST;

            /* link to the backshadow list */
            blockPtrBE->NextBlock = blockPtrCE;

            /* point to the new block */
            blockPtrBE = blockPtrCE;

            /* left edge */
            blockPtrBE->sXmin = wXmin;           

            /* right edge */
            blockPtrBE->sXmax = blockPtr->sXmax; 

            /* move on to the next segment in the current segment list */
            blockPtr = blockPtr->NextBlock;
 
        } /* while( (wXmin = blockPtr->sXmin) != 0x7FFF ) */

        if( Done )
        { 
            /* while( !Done && JumpDrawList ) */
            break;
        }

        /* free up the remaining blocks in the old shadow list (the sentinel, at a minimum) */
        FreeList();

        /* head of the new shadow list */
        blockPtrDN = pShadow;

        /* now decide whether the backshadow is good for anything */
        if( blockPtrBE  == blockPtrDN )
        {
            /* the new shadow is empty */
            /* return the sentinel to the free list */
            blockPtrST = FreeBlocks;
            FreeBlocks = blockPtrBE;
            blockPtrBE = blockPtrST;
            blockPtrDN->NextBlock = blockPtrBE;

            /* make the current segment */
            blockPtrDN = CurrentSegmentList;

            /* list the shadow for next time */
            pShadow = blockPtrDN; 
        }   
        else
        {
            /* link the head of the new list back to the end through the sentinel */
            blockPtrBE->NextBlock = blockPtrDN;

            /* save the current segment list as the pushed shadow for
               when we continue in this direction */
            blockPtrDN = CurrentSegmentList;

            /* save the current state */
            if( PushState(UnallocatedEnd) == 0 )
            {
                /* not enough memory */
                grafErrValue = c_SeedFill +  c_OutofMem;
                nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                Done = NU_TRUE;
                break;  /* while( !Done && JumpDrawList ) */
            }

            /* new scanning limit */
            ForwardLimit = BackwardLimit;          

            /* where to stop if the scan reverses again and comes back this way */
            BackwardLimit = ScanLine + sDirection; 

            /* new segment list to add to if limit is reached */
            FLimitPtr = BLimitPtr;                 

            /* segment list to add to if the above occurs */
            BLimitPtr = CurrentSegmentList;        

            /* reverse direction */
            sDirection = -sDirection;

            /* point to shadow sentinel */
            blockPtrDN = pShadow;
            
        } /* else */


        /* Main seed fill loop. */
        while(1)
        {
            if( Done )
            {
                break;
            }

            /* point past sentinel to first shadow */
            blockPtrDN = blockPtrDN->NextBlock;

            /* advance one scan line up or down */
            ScanLine += sDirection;
            if( ScanLine != ForwardLimit )
            {
                /* have not reached the current limit in this direction */
                /* generate the segment list from the shadow */
                blockPtrDN = pShadow; 
                if( ScanOutSegList(UnallocatedEnd) == 0 )
                {
                    /* not enough memory */
                    grafErrValue = c_SeedFill +  c_OutofMem;
                    nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                    Done = NU_TRUE;
                    break; 
                }

                if( blockPtr != blockPtr->NextBlock)
                {
                    JumpDrawList = NU_TRUE;
                    break; 
                }

                /* list is empty, done with this direction of scanning */
                /* free up the sentinel */
                blockPtr->NextBlock = FreeBlocks;
                FreeBlocks = blockPtr;

                /* point to the shadow that just proved to lead nowhere */
                blockPtrDN = pShadow->NextBlock;

                /* free the shadow */
                FreeList();

                /* none left to pop, so we're done */
                if( PopState() == 0 )
                {
                    /* Draw any remaining rects in the blitRcd. */
                    FillPrimitive(&blitRecord);
                    Done = NU_TRUE;
                    break; 
                }
                continue; /* in the new direction */

            } /* if( ScanLine != ForwardLimit ) */ 

            /* have reached the current limit in this direction */
            if( ScanLine == lmtR.Ymax )
            {
                /* we're at the bottom of the limit rect.  We'll never fill
                   past this point, so this shadow is of no use; dump it so we
                   don't waste space */
                FreeList();
            }
            else
            {
                /* all done */
                if( ScanLine < lmtR.Ymin )
                {
                    /* Draw any remaining rects in the blitRcd. */
                    FillPrimitive(&blitRecord);
                    Done = NU_TRUE;
                    break; /* while(1) Main seed */
                }

                /* add current shadow to forward limit shadow for whenever
                   we return here */

                /* ***assumes non-empty shadow!*** */
                /* point to forward limit segment list header */
                blockPtrBE = FLimitPtr;
                do
                {
                    blockPtr = blockPtrBE->NextBlock;
                    while( blockPtrDN->sXmin >= blockPtr->sXmin )
                    {
                        /* find the limit */
                        blockPtrBE = blockPtr;
                        blockPtr = blockPtrBE->NextBlock;
                    }

                    /* link in shadow */
                    blockPtrBE->NextBlock = blockPtrDN; 

                    /* segment */
                    blockPtrBE = blockPtrDN->NextBlock; 
                    blockPtrDN->NextBlock = blockPtr;
                    blockPtr = blockPtrBE;

                    /* the shadow segment is now the current forward limit element */
                    blockPtrBE = blockPtrDN;    

                    /* next shadow segment */
                    blockPtrDN = blockPtr;              
                    
                } while( blockPtrDN->sXmin != 0x7FFF );  

                /* return the shadow */
                blockPtrDN->NextBlock = FreeBlocks;

                /* sentinel to the free block list */
                FreeBlocks = blockPtrDN; 

            } /* else */

            /* none left to pop, so we're done */
            if( PopState() == 0 )
            {
                break;
            }

            if( Done )
            {
                /* while(1) Main seed */
                break; 
            }
            
        } /* while(1) Main seed */
        
        /* continue in the new direction */
        if( Done )
        {
            break;
        }
        
    } /* while( !Done && JumpDrawList ) */

    if( !Done )
    {
        /* Draw any remaining rects in the blitRcd. */
        FillPrimitive(&blitRecord);
    }
}

/***************************************************************************
* FUNCTION
*
*    NotMatchBoundaryColor
*
* DESCRIPTION
*
*    Function NotMatchBoundaryColor returns 1 if pixel at qualifyX,
*    WorkingScanLine is fillable (not the boundary color), else 0.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns 1 if pixel at qualifyX.
*
***************************************************************************/
INT32 NotMatchBoundaryColor(VOID)
{
    INT32 value = 1;

    if( GetPixelPrimitive(qualifyX, WorkingScanLine, blitRecord.blitDmap) == boundaryColor)
    {
        value = 0;
    }
    return(value);
}

/***************************************************************************
* FUNCTION
*
*    MatchSeedColor
*
* DESCRIPTION
*
*    Function MatchSeedColor returns 1 if pixel at qualifyX, WorkingScanLine
*    is fillable (the seed color), else 0.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns 1 if pixel at qualifyX.
*
***************************************************************************/
INT32 MatchSeedColor(VOID)
{
    INT32 value = 0;

    if(GetPixelPrimitive(qualifyX, WorkingScanLine, blitRecord.blitDmap) == SeedColor)
    {
        value = 1;
    }

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    AllocBlock
*
* DESCRIPTION
*
*    Function AllocBlock allocates a block from the memory pool.  The end of
*    the block list (out of free blocks) is marked by a return value of 0.
*
* INPUTS
*
*    SBlock *UnallocatedEnd        - Un-allocated buffer.
*
* OUTPUTS
*
*    INT32 - Returns 1 if successful.
*            Returns 0 if not successful.
*
***************************************************************************/
INT32 AllocBlock(SBlock *UnallocatedEnd)
{
    INT32 value = 1;

    /* point to the list of free blocks */
    blockPtr = FreeBlocks;  

    if( blockPtr == NU_NULL )
    {
        /* see if we can get more from the pool */
        blockPtr = Unallocated;
        
        /* point to the next block of as-yet unallocated memory */
        Unallocated++;

        if( blockPtr <= UnallocatedEnd )
        {
            /* okay */
            value = 1; 
        }
        else
        {
            /* out of free memory */
            value = 0; 
        } 
    }

    else
    {
        /* the new head of the free block list */
        FreeBlocks = blockPtr->NextBlock; 
    }

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    ScanOutSegList
*
* DESCRIPTION
*
*    Function ScanOutSegList generates a fill segment list for a scan
*    line (all segments on the current scan line that should be filled,
*    given the shadow list from the last line).  The sentinel links around
*    to the first segment, and is also linked to the last segment. If
*    sentinel is linked to itself, it's not possible to advance in this
*    direction.  Returns 1 for success, 0 for failure.
*
* INPUTS
*
*    SBlock *UnallocatedEnd        - Un-allocated buffer.
*
* OUTPUTS
*
*    INT32 - Returns 1 if successful.
*            Returns 0 if not successful.
*
***************************************************************************/
INT32 ScanOutSegList(SBlock *UnallocatedEnd)
{
    INT16 Done                  = NU_FALSE;
    INT16 JumpSOSLSREntry       = NU_FALSE;
    INT16 JumpSOSLFindFirstLoop = NU_FALSE;
    INT16 LoopDone              = NU_FALSE;
    INT16 JumpContinue          = NU_FALSE;
    INT32 value;
    INT32 qualifyXSave;

    WorkingScanLine = ScanLine;

    /* to remove paradigm warning */
    (VOID)LoopDone;
    (VOID)JumpSOSLFindFirstLoop;

    /* point to first shadow block */
    blockPtrDN = blockPtrDN->NextBlock;
    if( AllocBlock(UnallocatedEnd) == NU_NULL )
    {
        /* couldn't do it */
        value = 0; 
        Done  = NU_TRUE;
    }

    if( !Done )
    {
        /* remember where the head of list is */
        TempListHead = blockPtr;    
    }
    
    LoopDone = NU_FALSE;
    while( !Done && (qualifyX = blockPtrDN->sXmin) != 0x7FFF )
    {
        if( Done || LoopDone)
        {
            break;
        }
   
        /* is the min point fillable? */
        if( Qualify() == 1 )
        {
            /* scan left to find the start of the fill segment we're
               currently in (begin with the point to the left of the
               shadow left edge, since we already checked the shadow
               left edge point) */
            /* remember scanning start point */
            qualifyXSave = qualifyX;    
            while ( --qualifyX >= lmtR.Xmin )
            {
                /* scan left to find the start of the segment or the left edge */
                if( Qualify() == 0 )
                {
                    break;
                }
            }

            wXmin           = qualifyX + 1;
            qualifyX        = qualifyXSave;
            JumpSOSLSREntry = NU_TRUE;
        }

        /* min point not fillable; scan right until either qualify
           or this shadow segment runs out */
        if( !JumpSOSLSREntry )
        {
            if( ++qualifyX >= blockPtrDN->sXmax )
            {
                /* found right edge */
                blockPtrDN = blockPtrDN->NextBlock;
                continue;
            }
        } /* if( !JumpSOSLSREntry ) */

        JumpSOSLFindFirstLoop = NU_TRUE;
        while( JumpSOSLFindFirstLoop )
        {
            JumpSOSLFindFirstLoop = NU_FALSE;
            
            if( Done || LoopDone)
            {
                break;
            }
            
            if( !JumpSOSLSREntry )
            {
                /* scan right to find the end of the segment */
                if( Qualify() == 0 )
                {
                    if( ++qualifyX >= blockPtrDN->sXmax )
                    {
                        /* found right edge */
                        blockPtrDN = blockPtrDN->NextBlock;
                        JumpContinue = NU_TRUE;
                        break;
                    }
                    else
                    {
                        JumpSOSLFindFirstLoop = NU_TRUE;
                    }
                }

                if( !JumpSOSLFindFirstLoop )
                {
                    /* remember where the left edge of this fill segment is */
                    wXmin = qualifyX; 
                }
            }
            JumpSOSLSREntry = NU_FALSE;

            /* found right end of segment */
            if( !JumpSOSLFindFirstLoop )
            {
                /* pointer to current last block */
                blockPtrBE = blockPtr;  

                /* out of memory? */
                if( AllocBlock(UnallocatedEnd) == NU_NULL )
                {
                    value = 0;
                    Done  = NU_TRUE;
                    break; 
                }

                /* add the next block at the end of the list */
                blockPtrBE->NextBlock = blockPtr;   

                /* set the left edge of this fill segment */
                blockPtr->sXmin = wXmin;            

                if( ++qualifyX >= lmtR.Xmax )
                {
                    /* found right edge */
                    /* set right edge of fill segment */
                    blockPtr->sXmax = qualifyX; 
                    LoopDone = NU_TRUE;
                    break;  
                }

                /* scan right to find the end of the segment */
                while( Qualify() == 1 )
                {
                    if( ++qualifyX >= lmtR.Xmax )
                    {
                        /* found right edge */
                        blockPtr->sXmax = qualifyX;
                        LoopDone = NU_TRUE;
                        break;
                    }
                }

                /* set right edge of fill segment */
                blockPtr->sXmax = qualifyX;

                /* point to the first possible start pixel for the next fill segment */
                wXmin = qualifyX + 1;

                /* at the right edge of the limit now? */
                if( wXmin >= lmtR.Xmax)
                {
                    LoopDone = NU_TRUE;
                    break;
                }

                if( wXmin < blockPtrDN->sXmax)
                {
                    /* continue scanning out this shadow */
                    JumpSOSLFindFirstLoop = NU_TRUE; 
                }
            } /* if( !JumpSOSLFindFirstLoop ) */

            if( !JumpSOSLFindFirstLoop )
            {
                do
                {
                    blockPtrDN = blockPtrDN->NextBlock;
                } while( wXmin >= blockPtrDN->sXmax );   

                /* found the next shadow with which to work */
                if( wXmin >= blockPtrDN->sXmin )
                {
                    /* when X is the resume-scanning point, there's no need to scan left,
                       because we just came from there */
                    JumpSOSLFindFirstLoop = NU_TRUE; 
                }

            } /* if( !JumpSOSLFindFirstLoop ) */

        } /* while( JumpSOSLFindFirstLoop ) */
        
        if( JumpContinue )
        {
            JumpContinue = NU_FALSE;
            continue;
        }
    }   /* continue scanning out shadow */

    /* successfully done! - relink to make the head of the list also the
       sentinel at the end */
    if( !Done )
    {
        /* return pointer to head of segment */
        blockPtrBE = TempListHead;

        /* link the head at the end */
        blockPtr->NextBlock = blockPtrBE;

        /* and make it a sentinel */
        blockPtrBE->sXmin = 0x7FFF;
        blockPtrBE->sXmax = 0x7FFF;

        /* return pointer to the sentinel */
        blockPtr = blockPtrBE;

        value = 1;
        Done = NU_TRUE;

        /* to remove paradigm warning */
        (VOID)Done;
    }

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    PopState
*
* DESCRIPTION
*
*    Function PopState pops a scanning state from the scanning state stack.
*    Returns 1 for success, 0 for state stack empty, popped shadow in blockPtrDN
*    as well as in pShadow[br]. Also restores ForwardLimit, FLimitPtr,
*    BackwardLimit, BLimitPtr, ScanLine, and reverses sDirection.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns 1 if successful.
*            Returns 0 if not successful.
*
***************************************************************************/
INT32 PopState(VOID)
{
    INT32 value = 1;

    /* is it empty? */
    if( StateStackPtr == NU_NULL)
    {
        value = 0;
    }
    else
    {
        /* reverse direction */
        sDirection = -sDirection;

        /* forward limit becomes backward limit */
        BackwardLimit = ForwardLimit;   
        BLimitPtr = FLimitPtr;          

        /* link in the two blocks we're about to free & blockPtrCE points to head */
        blockPtrCE = FreeBlocks;
        FreeBlocks = StateStackPtr;

        /* pushed forward limit */
        ForwardLimit = StateStackPtr->sXmin;         

        /* pushed forward limit pointer */
        FLimitPtr = (SBlock *) StateStackPtr->sXmax; 

        /* other half of info is in this block */
        StateStackPtr = StateStackPtr->NextBlock;    

        /* pushed scan line */
        ScanLine = StateStackPtr->sXmin;             

        /* pushed shadow pointer */
        pShadow = (SBlock *) StateStackPtr->sXmax;   

        /* new top of stack */
        blockPtrDN = StateStackPtr->NextBlock;       

        /* link to the rest of the free block list */
        StateStackPtr->NextBlock = blockPtrCE;      
        StateStackPtr = blockPtrDN;
        blockPtrDN = pShadow;
    } /* else */

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    PushState
*
* DESCRIPTION
*
*    Function PushState pushes a scanning state onto the scanning state
*    stack.  Preserves ForwardLimit, FLimitPtr, ScanLine, and the pointer
*    to a segment list in blockPtrDN.  Returns 1 for success, 0 for out of
*    memory.
*
* INPUTS
*
*    SBlock *UnallocatedEnd        - Un-allocated buffer.
*
* OUTPUTS
*
*    INT32 - Returns 1 if successful.
*            Returns 0 if not successful.
*
***************************************************************************/
INT32 PushState(SBlock *UnallocatedEnd)
{
    signed char  Done = NU_FALSE;
    INT32 value;

    /* out of memory ?*/
    if( AllocBlock(UnallocatedEnd) == NU_NULL )
    {
        value = 0;
        Done  = NU_TRUE;
    }

    while( !Done )
    {
        /* point state stack to the new block */
        blockPtrCE = StateStackPtr;
        StateStackPtr = blockPtr;

        /* push forward limit */
        blockPtr->sXmin = ForwardLimit;

        /* push backward limit */
        blockPtr->sXmax = (SIGNED) FLimitPtr;

        blockPtrBE = blockPtr;
    
        /* out of memory ? */
        if( AllocBlock(UnallocatedEnd) == NU_NULL )
        {
            value = 0;
            Done  = NU_TRUE;

            /* to remove paradigm warning */
            (VOID)Done;

            break;
        }
        /* other half of info is in this block */
        blockPtrBE->NextBlock = blockPtr;       

        /* push scan line */
        blockPtr->sXmin = ScanLine;             

        /* push shadow pointer */
        blockPtr->sXmax = (SIGNED) blockPtrDN;    

        /* link to the rest of the free block */
        blockPtr->NextBlock = blockPtrCE;

        value = 1;
        Done  = NU_TRUE;

        /* to remove paradigm warning */
        (VOID)Done;
        break;
    } /* while( !Done ) */

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    FreeList
*
* DESCRIPTION
*
*    Function FreeList frees up the linked list of blocks pointed to by
*    blockPtrDN, by adding it to the free list.  List at blockPtrDN must be
*    terminated with a 7FFF sentinel.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID FreeList(VOID)
{

    /* get the current pointer to the start of the free block list */
    blockPtrST = FreeBlocks;

    /* put the new list at the start of the free block list */
    FreeBlocks = blockPtrDN;

    while( blockPtrDN->sXmin != 0x7FFF )
    {
        blockPtrDN = blockPtrDN->NextBlock;
    }

    /* link the old free blocks at the end of the new list */
    blockPtrDN->NextBlock = blockPtrST;

}

/***************************************************************************
* FUNCTION
*
*    Setup1
*
* DESCRIPTION
*
*    Function Setup1 sets up stuff common to flood and boundary fill.
*    Sets up limit rect (in global coords, clipped to bitmap), blitRcd, and
*    primitive GetPixel and Fill vectors. Converts the seed point to global
*    coords, stores it in (SeedX,ScanLine), checks that it's inside the limit
*    rect, and returns it in (localFeedX,localFeedY).  Returns 1 for success,
*    0 for failure (seed point outside limit rect or limit rect fully clipped).
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns 1 if successful.
*            Returns 0 if not successful.
*
***************************************************************************/
INT32 Setup1(VOID)
{
    INT16 Done = NU_FALSE;
    INT32 value;

    /* copy the default blit record over */
    blitRecord = grafBlit;  

    /* no rectangles in blitList yet */
    blitRecord.blitCnt = 0; 

    /* convert seed point to global coords */
    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GP(localFeedX, localFeedY, &localFeedX, &localFeedY, 1);
    }

    /* initial X coordinate */
    SeedX = localFeedX;     

    /* initial scan line */
    ScanLine = localFeedY;  

    /* now get the limit rect and clip it to the bitmap (or use the
       bitmap limits if the limit rect pointer is NULL) */
    if( LimitPtr == NU_NULL )
    {
        /* just use the bitmap limits */
        lmtR.Xmin = 0;
        lmtR.Ymin = 0;
        lmtR.Xmax = blitRecord.blitDmap->pixWidth;
        lmtR.Ymax = blitRecord.blitDmap->pixHeight;
    }
    else
    {
        /* get the limit rect */
        lmtR = *LimitPtr;   

        /* convert limit rect to global coords */
        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(lmtR, &lmtR, 0);
        }

        /* and clip it to the bitmap limits */
        if( lmtR.Xmin < 0 )
        {
            lmtR.Xmin = 0;
        }
        if( lmtR.Ymin < 0 )
        {
            lmtR.Ymin = 0;
        }
        if( lmtR.Xmax > blitRecord.blitDmap->pixWidth )
        {
            lmtR.Xmax = blitRecord.blitDmap->pixWidth;
        }
        if( lmtR.Ymax > blitRecord.blitDmap->pixHeight )
        {
            lmtR.Ymax = blitRecord.blitDmap->pixHeight;
        }
        if( lmtR.Xmin >= lmtR.Xmax )
        {
            /* trivial reject in x? */
            value = 0;  
            Done  = NU_TRUE;
        }
        if( !Done && lmtR.Ymin >= lmtR.Ymax )
        {
            /* trivial reject in y? */
            value = 0;  
            Done = NU_TRUE;
        }
    }

    /* make sure the pixel is within the working limit rect */
    if( !Done && ( (SeedX < lmtR.Xmin) 
                || (SeedX >= lmtR.Xmax)
                || (ScanLine < lmtR.Ymin)
                || (ScanLine >= lmtR.Ymax)) )
    {
        value = 0;
        Done  = NU_TRUE;
    }

    if( !Done )
    {
        /* pixel is within the limit rect set up the GetPixel & Fill
           primitive vectors */
        GetPixelPrimitive = blitRecord.blitDmap->prGetPx;
        FillPrimitive = blitRecord.blitDmap->prFill;
    
        value = 1;
    }

    return(value);
}
