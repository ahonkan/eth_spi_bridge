/***************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*  m8b_fil.c
*
* DESCRIPTION
*
*  This file contains the 8-bit memory Fill functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M8BF_Fill8Bit
*  M8BF_DrawSolidFillRectM
*  M8BF_SetDestMem
*  M8BF_InvertDestMem
*  M8BF_FillOXADestMem
*  M8BF_NotDestFillMem
*  M8BF_SolidFillRectM
*  M8BF_MultiColorFillMem
*  M8BF_MultiColorFillRectM
*  M8BF_MonoPatternFillMem
*  M8BF_MonoFillDestMem
*  M8BF_MonoFillOXADestMem
*  M8BF_NotDestMonoFillMem
*  M8BF_TranMonoFillOXADestMem
*  M8BF_TranMonoFillDestMem
*
* DEPENDENCIES
*
*  display_inc.h
*  nu_drivers.h
*  nucleus.h
*  kernel.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"
#include "drivers/display_inc.h"

#ifdef INCLUDE_8_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

extern UINT32   lclpnColr, lclbkColr;

/***************************************************************************
* FUNCTION
*
*    M8BF_Fill8Bit
*
* DESCRIPTION
*
*    A special case optimization for MultiFill, monochrome to memory,
*    rectangular and/or region clipping with color translation. M8BF_Fill8Bit
*    forms the outer processing loop for this mode which internally dispatches
*    to one of several other optimizations based upon the specific rasterOp
*    transfer mode and LCD destination.
*
* INPUTS
*
*    blitRcd * fillRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_Fill8Bit(blitRcd *fillRec)
{

  /* Figure out whether this is a multicolor pattern, and, if so, whether the
     raster op is replaced. All non-multicolor patterns are handled here, as are
     replace-op multicolor, but non-replace multicolor patterns are handled by
     the multiplane fill handler. */
    lclPortMask = fillRec->blitMask;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function pre-processing task. */
    SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    HideCursor();
#endif

#ifdef  FILL_PATTERNS_SUPPORT

    /* save the pattern header address here temporarily */
    if (fillRec->blitPat > 1)
    {
        /* it's a patterned fill */
        patStart = fillRec->blitPatl->patPtr[fillRec->blitPat];
        if( patStart->patBits > 1 )
        {
            /* it's a multicolor fill */
            if( (fillRec->blitRop & 0x0f ) > 0 )
            {
                /* non-replace multicolor, handle through general plane filler */
                fillRec->blitDmap->mapWinPlane = -1;
            }
            else
            {
                /* we'll do a replace multicolor fill locally */
                M8BF_MultiColorFillMem(fillRec);
            }
        }
        else
        {
            /* we'll do a monocolor pattern fill locally */
            M8BF_MonoPatternFillMem(fillRec);
        }
    }
    else

#endif /* FILL_PATTERNS_SUPPORT */

    {
        /* we'll do a solid fill locally */
        M8BF_SolidFillMem(fillRec);
    }

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

      /* Call the function pre-processing task. */
      SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M8BF_MultiColorFillMem
*
* DESCRIPTION
*
*    A special case optimization for multicolor,patterned fill to
*    memory, rectangular clipping with color translation.
*
* INPUTS
*
*    blitRcd * fillRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_MultiColorFillMem(blitRcd *fillRec)
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    /* retrieve the pattern header address  */
    savePat = patStart;
    rectCnt = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap = fillRec->blitDmap;

    /* height in rows  */
    patHeight = savePat->patHeight;
    patWidthInBytes = savePat->patWidth;
    patLength = patWidthInBytes * patHeight;

    /* distance from one plane to the next */
    patPlaneOffset = patLength;

    M_PAUSE(dstBmap);

    /* set up clipping */
    if( CLIP_Set_Up_Clip(fillRec, &clipR, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if ( !done )
    {
        while( --rectCnt >= 0 )
        {
            dRect = *rListPtr++;

            /* do we need to worry about clipping */
            if( clipToRectFlag != 0 )
            {
                /* yes, do trivial reject */
                if( dRect.Xmin < clipR.Xmin )
                {
                    dRect.Xmin  = clipR.Xmin;
                }
                if( dRect.Ymin < clipR.Ymin )
                {
                    dRect.Ymin  = clipR.Ymin;
                }
                if( dRect.Xmax > clipR.Xmax )
                {
                    dRect.Xmax  = clipR.Xmax;
                }
                if( dRect.Ymax > clipR.Ymax )
                {
                    dRect.Ymax  = clipR.Ymax;
                }

                if( dRect.Ymin >= dRect.Ymax )
                {
                    /* Check to see whether the rectangle was trivially
                       clipped because it was fully below the clip rect,
                       in which case we can discard the rest of a YX banded
                       blitList, or because it was fully above the clip
                       rect, in which case we can whiz ahead through a YX
                       banded blitList until we run out of rects or find a
                       rect that isn't fully above the clip rect. */
                    if( CLIP_Check_YX_Band(&dRect, rListPtr, &clipR, &rectCnt) )
                    {
                           break;
                    }
                    continue;
                }

                if( dRect.Xmin >= dRect.Xmax )
                {
                    continue;
                }

#ifndef NO_REGION_CLIP
                
                /* do we need to worry about region clipping? */
                if( clipToRegionFlag != 0 )
                {
                    /* yes, decide whether to copy top->bottom
                    or bottom->top */
                    FillDrawer = &M8BF_MultiColorFillRectM;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M8BF_MultiColorFillRectM(fillRec);
        }

    }
    nuResume(dstBmap);
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M8BF_SolidFillMem
*
* DESCRIPTION
*
*    Solid fill for 8 bit per pixel.
*
* INPUTS
*
*    blitRcd * fillRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_SolidFillMem(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap = fillRec->blitDmap;

    if( fillRec->blitPat == 0 )
    {
        if( (fillRec->blitRop & 0x10) != 0 )
        {
            /* transparent with background color is a NOP */
            done = NU_TRUE;
        }

        if(!done)
        {
            pnColr = fillRec->blitBack;
        }
    }
    else
    {
        pnColr = fillRec->blitFore;
    }

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr, lclpnColr);
    COLOR_CONVERT(bkColr, lclbkColr);

    if( !done )
    {
        M_PAUSE(dstBmap);

        dstClass = fillRec->blitRop & 0x0f;

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M8BF_SetDestMem;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M8BF_FillOXADestMem;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M8BF_NotDestFillMem;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M8BF_InvertDestMem;
            break;
        }

        /* set up clipping */
        if( CLIP_Set_Up_Clip(fillRec, &clipR, blitMayOverlap, isLine) )
        {
            done = NU_TRUE;
        }

        if ( !done )
        {
            while( --rectCnt >= 0 )
            {
                dRect = *rListPtr;
                rListPtr++;

                /* do we need to worry about clipping */
                if( clipToRectFlag != 0 )
                {
                    /* yes, do trivial reject */
                    if( dRect.Xmin < clipR.Xmin )
                    {
                        dRect.Xmin  = clipR.Xmin;
                    }
                    if( dRect.Ymin < clipR.Ymin )
                    {
                        dRect.Ymin  = clipR.Ymin;
                    }
                    if( dRect.Xmax > clipR.Xmax )
                    {
                        dRect.Xmax  = clipR.Xmax;
                    }
                    if( dRect.Ymax > clipR.Ymax )
                    {
                        dRect.Ymax  = clipR.Ymax;
                    }

                    if( dRect.Ymin >= dRect.Ymax )
                    {
                        /* Check to see whether the rectangle was trivially
                        clipped because it was fully below the clip rect,
                        in which case we can discard the rest of a YX banded
                        blitList, or because it was fully above the clip
                        rect, in which case we can whiz ahead through a YX
                        banded blitList until we run out of rects or find a
                        rect that isn't fully above the clip rect. */
                        if( CLIP_Check_YX_Band(&dRect, rListPtr, &clipR, &rectCnt) )
                        {
                               break;
                        }
                        continue;
                    }

                    if( dRect.Xmin >= dRect.Xmax )
                    {
                        continue;
                    }

#ifndef NO_REGION_CLIP
                
                    /* do we need to worry about region clipping? */
                    if( clipToRegionFlag != 0 )
                    {
                        /* yes, decide whether to copy
                        top->bottom or bottom->top */
                        FillDrawer = &M8BF_DrawSolidFillRectM;

                        if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                        {
                            break;
                        }
                        continue;
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                /* blit the rectangle */
                M8BF_DrawSolidFillRectM(fillRec);
            }
        }
    }
    nuResume(dstBmap);
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M8BF_MonoPatternFillMem
*
* DESCRIPTION
*
*     Set up for monocolor pattern fill.
*
* INPUTS
*
*    blitRcd * fillRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_MonoPatternFillMem(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap  = fillRec->blitDmap;

    pnColr = fillRec->blitFore;
    bkColr = fillRec->blitBack;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr, lclpnColr);
    COLOR_CONVERT(bkColr, lclbkColr);

    /* retrieve the pattern header address */
    savePat = patStart;

    /* skip over the pattern header to the bit pattern  */
    ptrnData = savePat->patData;

    /* height in rows  */
    patHeight       = savePat->patHeight;
    patWidthInBytes = savePat->patWidth >> 3;
    patLength       = patWidthInBytes * patHeight;

    /* distance from one plane to the next */
    patPlaneOffset = patLength;

    M_PAUSE(dstBmap);

    dstClass = fillRec->blitRop & 0x0f;

    /* look up the optimization routine */
    switch (dstClass)
    {
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        if(fillRec->blitRop & 0x10)
        {
            optPtr = &M8BF_TranMonoFillDestMem;
        }
        else
        {
            optPtr = &M8BF_MonoFillDestMem;
        }
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        if(fillRec->blitRop & 0x10)
        {
            optPtr = &M8BF_TranMonoFillOXADestMem;
        }
        else
        {
            optPtr = &M8BF_MonoFillOXADestMem;
        }
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M8BF_NotDestMonoFillMem;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M8BF_InvertDestMem;
        break;
    }

        /* modify the pen color as necessary */
    switch (dstClass)
    {
    case 3:     /* zNANDz  */
    case 4:     /* zNREPz  */
    case 5:     /* zNORz   */
    case 6:     /* zNXORz  */
    case 13:    /* zNORNz  */
    case 15:    /* zNANDNz */
    /* this has the effect of notting the pen color for all operations during this call */
        bkColr = ~bkColr;
        pnColr = ~pnColr;
        lclbkColr = ~lclbkColr;
        lclpnColr = ~lclpnColr;
    case 0:     /* zREPz  */
    case 1:     /* zORz   */
    case 2:     /* zXORz  */
    case 7:     /* zANDz  */
    case 9:     /* zORNz  */
    case 10:    /* zNOPz  */
    case 11:    /* zANDNz */
        break;

    case 8:     /* zCLEARz */
        /* sets all source bits to 0 */
        bkColr = 0;
        pnColr = 0;
        lclbkColr = 0;
        lclpnColr = 0;
        break;

    case 12:    /* zSETz    */
    case 14:    /* zINVERTz */
        /* sets all source bits to 1 */
        bkColr = ~0;
        pnColr = ~0;
        lclbkColr = ~0;
        lclpnColr = ~0;
    }

    /* set up clipping */
    if( CLIP_Set_Up_Clip(fillRec, &clipR, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if ( !done )
    {
        while( --rectCnt >= 0 )
        {
            dRect = *rListPtr++;

            /* do we need to worry about clipping */
            if( clipToRectFlag != 0 )
            {
                /* yes, do trivial reject */
                if( dRect.Xmin < clipR.Xmin )
                {
                    dRect.Xmin  = clipR.Xmin;
                }
                if( dRect.Ymin < clipR.Ymin )
                {
                    dRect.Ymin  = clipR.Ymin;
                }
                if( dRect.Xmax > clipR.Xmax )
                {
                    dRect.Xmax  = clipR.Xmax;
                }
                if( dRect.Ymax > clipR.Ymax )
                {
                    dRect.Ymax  = clipR.Ymax;
                }

                if( dRect.Ymin >= dRect.Ymax )
                {
                    /* Check to see whether the rectangle was trivially
                    clipped because it was fully below the clip rect,
                    in which case we can discard the rest of a YX banded
                    blitList, or because it was fully above the clip
                    rect, in which case we can whiz ahead through a YX
                    banded blitList until we run out of rects or find a
                    rect that isn't fully above the clip rect. */
                    if(CLIP_Check_YX_Band(&dRect, rListPtr, &clipR, &rectCnt) )
                    {
                           break;
                    }
                    continue;
                }

                if( dRect.Xmin >= dRect.Xmax )
                {
                    continue;
                }

#ifndef NO_REGION_CLIP
                
                /* do we need to worry about region clipping? */
                if( clipToRegionFlag != 0 )
                {
                    /* yes, decide whether to copy
                    top->bottom or bottom->top */
                    FillDrawer = &M8BF_DrawSolidFillRectM;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M8BF_DrawSolidFillRectM(fillRec);
        }
    }
    nuResume(dstBmap);
}

/***************************************************************************
* FUNCTION
*
*    M8BF_MultiColorFillRectM
*
* DESCRIPTION
*
*    Fills a multicolored rectangle in the memory.
*
* INPUTS
*
*    blitRcd * blitRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_MultiColorFillRectM(blitRcd *blitRec)
{
    INT32 lclByteCnt, lclLineCnt;
    INT32 ptrnX, ptrnY;

#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lclLineCnt = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = dstBmap->pixBytes - byteCntM1 - 1;

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        lclByteCnt = byteCntM1;
        while (lclByteCnt-- >= 0)
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            *dstPtr = savePat->patData[ptrnX + ptrnY];
            dstPtr++;
            ptrnX++;
            if( ptrnX == patWidthInBytes )
            {
                ptrnX = 0;
            }
        }

        /* advance to next row */
        dstPtr += dstNextRow;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength )
        {
            ptrnY = 0;
        }
    }
#ifdef USING_DIRECT_X
    /* Blit back buffer to screen */
    BlitBacktoFront(&scrnRect);
#endif

}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M8BF_DrawSolidFillRectM
*
* DESCRIPTION
*
*    Fills the rectangle on the screen.
*
* INPUTS
*
*    blitRcd * fillRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_DrawSolidFillRectM(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = dstBmap->pixBytes - byteCntM1 - 1;

    dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

    /* blit the rectangle */
    optPtr();

#ifdef USING_DIRECT_X
    /* Blit back buffer to screen */
    BlitBacktoFront(&scrnRect);
#endif

}


/***************************************************************************
* FUNCTION
*
*    M8BF_InvertDestMem
*
* DESCRIPTION
*
*     Inverts the destination pixel data.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_InvertDestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;

    /* set up local pointers */
    lclDstPtr = dstPtr;

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            *lclDstPtr = ~(*lclDstPtr);
            lclDstPtr++;
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M8BF_SetDestMem
*
* DESCRIPTION
*
*    Sets the destination pixel data.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_SetDestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;

    /* set up local pointers */
    lclDstPtr = dstPtr;

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
            lclDstPtr++;
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M8BF_NotDestFillMem
*
* DESCRIPTION
*
*    Handles memory "not dst" by inverting the destination first.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_NotDestFillMem(VOID)
{
    /* invert the destination */
    M8BF_InvertDestMem();

    /* fill this rectangle */
    M8BF_FillOXADestMem();
}

/***************************************************************************
* FUNCTION
*
*   M8BF_NotDestMonoFillMem
*
* DESCRIPTION
*
*    Handles memory "not dst" by inverting the destination first.
*
* INPUTS
*
*    blitRcd * fillRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_NotDestMonoFillMem(VOID)
{
    /* invert the destination */
    M8BF_InvertDestMem();

    /* fill this rectangle */
    M8BF_MonoFillOXADestMem();
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M8BF_MonoFillDestMem
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr if the source is 1 else bkColr.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_MonoFillDestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX    = dRect.Xmin % patWidthInBytes;
        ptrnBt   = (128 >> (dRect.Xmin & 7));
        ptrnByte = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if (ptrnByte & ptrnBt)
            {
                /* source is 1 */
                *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
            }
            else
            {
                /* source is 0 */
                *lclDstPtr = (UINT8)(lclbkColr & lclPortMask);
            }

            lclDstPtr++;

            ptrnBt = ptrnBt >> 1;
            if( ptrnBt == 0 )
            {
                ptrnBt = 128;

                ptrnX++;
                if( ptrnX == patWidthInBytes )
                {
                    ptrnX = 0;
                }

                ptrnByte = savePat->patData[ptrnX + ptrnY];
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength )
        {
            ptrnY = 0;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M8BF_TranMonoFillDestMem
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr if the source is 1 else nothing
*    is done.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_TranMonoFillDestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        ptrnBt = (128 >> (dRect.Xmin & 7));
        ptrnByte = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( ptrnByte & ptrnBt )
            {
                *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
            }

            lclDstPtr++;

            ptrnBt = ptrnBt >> 1;
            if( ptrnBt == 0 )
            {
                ptrnBt = 128;

                ptrnX++;
                if( ptrnX == patWidthInBytes )
                {
                    ptrnX = 0;
                }
                ptrnByte = savePat->patData[ptrnX + ptrnY];
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength )
        {
            ptrnY = 0;
        }
    }
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M8BF_FillOXADestMem
*
* DESCRIPTION
*
*    Sets the destination pixel data based on the logical function "OR",
*    "XOR" or "AND".
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_FillOXADestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    UINT8 lclMask;

    lclMask = (UINT8)(lclPortMask & lclpnColr);

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        while ( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
                *lclDstPtr = *lclDstPtr | lclMask;
                break;
            case 2: /* "XOR" */
                *lclDstPtr = *lclDstPtr ^ lclMask;
                break;
            case 3: /* "AND" */
                *lclDstPtr = *lclDstPtr & lclMask;
            }

            lclDstPtr++;
        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M8BF_MonoFillOXADestMem
*
* DESCRIPTION
*
*    Sets the destination pixel data based on the logical function "OR",
*    "XOR" or "AND" and the pattern bit.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_MonoFillOXADestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;
    UINT8 lclMask;

    /* set up local pointers */
    lclDstPtr = dstPtr;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX    = dRect.Xmin % patWidthInBytes;
        ptrnBt   = (128 >> (dRect.Xmin & 7));
        ptrnByte = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( ptrnByte & ptrnBt )
            {
                ptrnColr = lclpnColr;
            }
            else
            {
                ptrnColr = lclbkColr;
            }

            lclMask = (UINT8)(lclPortMask & ptrnColr);

            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
                *lclDstPtr = *lclDstPtr | lclMask;
                break;
            case 2: /* "XOR" */
                *lclDstPtr = *lclDstPtr ^ lclMask;
                break;
            case 3: /* "AND" */
                *lclDstPtr = *lclDstPtr & lclMask;
            }

            lclDstPtr++;
            ptrnBt = ptrnBt >> 1;
            if( ptrnBt == 0 )
            {
                ptrnBt = 128;

                ptrnX++;
                if( ptrnX == patWidthInBytes)
                {
                    ptrnX = 0;
                }
                ptrnByte = savePat->patData[ptrnX + ptrnY];
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength )
        {
            ptrnY = 0;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M8BF_TranMonoFillOXADestMem
*
* DESCRIPTION
*
*    Sets the destination pixel data based on the logical function "OR",
*    "XOR" or "AND" and the pattern bit. If the pattern bit is 0 nothing
*    will be done.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M8BF_TranMonoFillOXADestMem(VOID)
{
    UINT8 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;
    UINT8 lclMask;

    /* set up local pointers */
    lclDstPtr = dstPtr;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX    = dRect.Xmin % patWidthInBytes;
        ptrnBt   = (128 >> (dRect.Xmin & 7));
        ptrnByte = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if (ptrnByte & ptrnBt )
            {
                ptrnColr = lclpnColr;

                /* Added software plane masking to routine */
                lclMask = (UINT8)(lclPortMask & ptrnColr);

                switch (logFnc)
                {
                case 0: /* handled elsewhere */
                case 1: /* "OR" */
                    *lclDstPtr = *lclDstPtr | lclMask;
                    break;
                case 2: /* "XOR" */
                    *lclDstPtr = *lclDstPtr ^ lclMask;
                    break;
                case 3: /* "AND" */
                    *lclDstPtr = *lclDstPtr & lclMask;
                }
            }

            lclDstPtr++;

            ptrnBt = ptrnBt >> 1;
            if( ptrnBt == 0 )
            {
                ptrnBt = 128;

                ptrnX++;
                if( ptrnX == patWidthInBytes)
                {
                    ptrnX = 0;
                }
                ptrnByte = savePat->patData[ptrnX + ptrnY];
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength)
        {
            ptrnY = 0;
        }
    }
}

#endif /* FILL_PATTERNS_SUPPORT */

#endif /* #ifdef INCLUDE_8_BIT */

