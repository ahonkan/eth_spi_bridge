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
*  m2b4_fil.c
*
* DESCRIPTION
*
*  This file contains the 2/4-bit memory Fill functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M2B4F_DrawSolidFillRectM2_4Bit
*  M2B4F_Fill2_4Bit
*  M2B4F_SolidFillM2_4Bit
*  M2B4F_MonoPatternFillM2_4Bit
*  M2B4F_SetDestM2_4Bit
*  M2B4F_FillOXADestM2_4Bit
*  M2B4F_NotDestFillM2_4Bit
*  M2B4F_InvertDestM2_4Bit
*  M2B4F_MonoFillDestM2_4Bit
*  M2B4F_MonoFillOXADestM2_4Bit
*  M2B4F_MultiColorFillM2_4Bit
*  M2B4F_MultiColorFillRectM2_4Bit
*  M2B4F_NotDestMonoFillM2_4Bit
*
* DEPENDENCIES
*
*  nucleus.h
*  nu_kernel.h
*  nu_drivers.h
*  display_inc.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"
#include "drivers/display_inc.h"

#ifdef INCLUDE_2_4_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */


extern  UINT32  lclpnColr, lclbkColr;

/***************************************************************************
* FUNCTION
*
*    M2B4F_Fill2_4Bit
*
* DESCRIPTION
*
*    A special case optimization for MultiFill, monochrome to 2/4 bit memory,
*    rectangular and/or region clipping with color translation. M2B4F_Fill2_4Bit
*    forms the outer processing loop for this mode which internally dispatches
*    to one of several other optimizations based upon the specific rasterOp
*    transfer mode and memory destination.
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
VOID M2B4F_Fill2_4Bit(blitRcd *fillRec)
{
    /* Figure out whether this is a multicolor pattern, and, if so, whether the
    raster op is replaced. All non-multicolor patterns are handled here, as are
    replace-op multicolor, but non-replace multicolor patterns are handled by
    the multiplane fill handler.  */

    lclPortMask = fillRec->blitMask;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function pre-processing task. */
    SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    HideCursor();
#endif

#ifdef  FILL_PATTERNS_SUPPORT

    /* save the pattern header address here temporarily  */
    if( fillRec->blitPat > 1 )
    {
        /* it's a patterned fill */
        patStart = fillRec->blitPatl->patPtr[fillRec->blitPat];

        if( patStart->patBits > 1 )
        {
            /* it's a multicolor fill */
            if( (fillRec->blitRop & 0x0f) > 0 )
            {
                /* non-replace multicolor, handle thru general plane filler */
                fillRec->blitDmap->mapWinPlane = -1;
            }
            else
            {
                /* we'll do a replace multicolor fill locally */
                M2B4F_MultiColorFillM2_4Bit(fillRec);
            }
        }
        else
        {
            /* we'll do a monocolor pattern fill locally */
            M2B4F_MonoPatternFillM2_4Bit(fillRec);
        }
    }
    else
        
#endif /* FILL_PATTERNS_SUPPORT */
        
    {
        /* we'll do a solid fill locally */
        M2B4F_SolidFillM2_4Bit(fillRec);
    }
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

      /* Call the function post-processing task. */
      SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M2B4F_MultiColorFillM2_4Bit
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
VOID M2B4F_MultiColorFillM2_4Bit(blitRcd *fillRec)
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    /* retrieve the pattern header address */
    savePat = patStart;
    rectCnt = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap = fillRec->blitDmap;

    /* height in rows */
    patHeight       = savePat->patHeight;
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

            /* do we need to worry about clipping? */
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
                    FillDrawer = &M2B4F_MultiColorFillRectM2_4Bit;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M2B4F_MultiColorFillRectM2_4Bit(fillRec);
        }

    }
    nuResume(dstBmap);
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M2B4F_SolidFillM2_4Bit
*
* DESCRIPTION
*
*    Solid fill for 2/4 bit per pixel.
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
VOID M2B4F_SolidFillM2_4Bit(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap  = fillRec->blitDmap;

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

        dstWidth = dstBmap->pixBytes;

        /* determine number of bits per pixel */
        shfCnt = dstBmap->pixBits;
        if( shfCnt == 2 )
        {
            /* 2 bpp */
            flipMask    = 2;
            shiftMask   = 0xc0;
            firstOffset = 0x03;
            pnColr = pnColr & 0x03;
            lclpnColr = lclpnColr & 0x03;
        }
        else
        {
            /* 4 bpp */
            flipMask    = 1;
            shiftMask   = 0xf0;
            firstOffset = 0x01;
            pnColr = pnColr & 0x0f;
            lclpnColr = lclpnColr & 0x0f;
        }

        dstClass = fillRec->blitRop & 0x0f;

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M2B4F_SetDestM2_4Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M2B4F_FillOXADestM2_4Bit;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M2B4F_NotDestFillM2_4Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M2B4F_InvertDestM2_4Bit;
            break;
        }

        /* set up clipping */
        if( CLIP_Set_Up_Clip(fillRec, &clipR, blitMayOverlap, isLine) )
        {
            done = NU_TRUE;
        }
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
                    /* yes, decide whether to copy
                    top->bottom or bottom->top */
                    FillDrawer = &M2B4F_DrawSolidFillRectM2_4Bit;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }
            /* blit the rectangle */
            M2B4F_DrawSolidFillRectM2_4Bit(fillRec);
        }
    }
    nuResume(dstBmap);
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M2B4F_MonoPatternFillM2_4Bit
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
VOID M2B4F_MonoPatternFillM2_4Bit(blitRcd *fillRec )
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

    /* retrieve the pattern header address  */
    savePat = patStart;

    /* skip over the pattern header to the bit pattern  */
    ptrnData  = savePat->patData;

    /* height in rows */
    patHeight       = savePat->patHeight;
    patWidthInBytes = savePat->patWidth >> 3;
    patLength       = patWidthInBytes * patHeight;

    /* distance from one plane to the next */
    patPlaneOffset = patLength;

    M_PAUSE(dstBmap);

    dstWidth = dstBmap->pixBytes;

    /* determine number of bits per pixel */
    shfCnt = dstBmap->pixBits;
    if( shfCnt == 2 )
    {
        /* 2 bpp */
        flipMask    = 2;
        shiftMask   = 0xc0;
        firstOffset = 0x03;
    }
    else
    {
        /* 4 bpp */
        flipMask    = 1;
        shiftMask   = 0xf0;
        firstOffset = 0x01;
    }

    dstClass = fillRec->blitRop & 0x0f;

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* Memory non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M2B4F_MonoFillDestM2_4Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M2B4F_MonoFillOXADestM2_4Bit;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M2B4F_NotDestMonoFillM2_4Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M2B4F_InvertDestM2_4Bit;
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

    case 8: /* zCLEARz */
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
                    FillDrawer = &M2B4F_DrawSolidFillRectM2_4Bit;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M2B4F_DrawSolidFillRectM2_4Bit(fillRec);
        }
    }
    nuResume(dstBmap);
}

/***************************************************************************
* FUNCTION
*
*    M2B4F_MultiColorFillRectM2_4Bit
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
VOID M2B4F_MultiColorFillRectM2_4Bit(blitRcd *blitRec)
{
    INT32 lclByteCnt, lclLineCnt, lclShift;
    INT32 ptrnX, ptrnY;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;

    /* determine number of bits per pixel */
    shfCnt = dstBmap->pixBits;
    if( shfCnt == 2 )
    {
        /* 2 bpp */
        flipMask    = 2;
        shiftMask   = 0xc0;
        firstOffset = 0x03;
    }
    else
    {
        /* 4 bpp */
        flipMask    = 1;
        shiftMask   = 0xf0;
        firstOffset = 0x01;
    }

    lclLineCnt =  dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = (dRect.Xmin >> flipMask);
    pxShift    = (dRect.Xmin & firstOffset) * shfCnt;
    byteCntM1  =  dRect.Xmax - dRect.Xmin - 1;

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    lclShift = (1 << flipMask);
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

        ptrnX = dstBgnByte % patWidthInBytes;
        lclByteCnt = byteCntM1;

        /* check if first pixel aligned */
        if( pxShift != 0 )
        {
            /* no, align it */
            pxBit = (UINT8) (shiftMask >> pxShift);
            while( pxBit > 0 )
            {
                pxColor = (savePat->patData[ptrnX + ptrnY] & pxBit);

                *dstPtr = (*dstPtr & ~pxBit) | pxColor;

                pxBit = (pxBit >> shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
            dstPtr++;

            ptrnX++;
            if( ptrnX == patWidthInBytes)
            {
                ptrnX = 0;
            }
        }

        /* do the whole bytes */
        while( lclByteCnt >= lclShift )
        {
            /* for each UINT8 in the row */
            *dstPtr = savePat->patData[ptrnX + ptrnY];
            dstPtr++;

            ptrnX++;
            if( ptrnX == patWidthInBytes)
            {
                ptrnX = 0;
            }
            lclByteCnt -= lclShift;
        }

        /* do the last pixels if any */
        if( lclByteCnt >= 0 )
        {
            if( shfCnt == 2 )
            {
                pxBit = 0xc0;
            }
            else
            {
                pxBit = 0xf0;
            }
            while( lclByteCnt-- >= 0 )
            {
                pxColor = (savePat->patData[ptrnX + ptrnY] & pxBit);

                *dstPtr = (*dstPtr & ~pxBit) | pxColor;

                pxBit = (pxBit >> shfCnt);
            }
        }

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength)
        {
            ptrnY = 0;
        }
    }
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M2B4F_DrawSolidFillRectM2_4Bit
*
* DESCRIPTION
*
*    Fills the rectangle in memory.
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
VOID M2B4F_DrawSolidFillRectM2_4Bit(blitRcd *blitRec)
{
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = (dRect.Xmin >> flipMask);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    if( shfCnt == 2 )
    {
        /* 2 bpp */
        lclpnColr = lclpnColr & 0x03;
        lclpnColr = lclpnColr | (lclpnColr << 2);
        lclpnColr = lclpnColr | (lclpnColr << 4);

        lclbkColr = lclbkColr & 0x03;
        lclbkColr = lclbkColr | (lclbkColr << 2);
        lclbkColr = lclbkColr | (lclbkColr << 4);
    }
    else
    {
        /* 4 bpp */
        lclpnColr = lclpnColr & 0x0f;
        lclpnColr = lclpnColr | (lclpnColr << 4);

        lclbkColr = lclbkColr & 0x0f;
        lclbkColr = lclbkColr | (lclbkColr << 4);
    }

    optPtr();   /* blit the rectangle */

}

/***************************************************************************
* FUNCTION
*
*    M2B4F_InvertDestM2_4Bit
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
VOID M2B4F_InvertDestM2_4Bit(VOID)
{
    INT32 lclYmin, lclShift;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxShift;
    UINT8 pxBit;

    lclLineCnt = lineCntM1;
    lclYmin = dRect.Ymin;
    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    lclShift = (1 << flipMask);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + lclYmin)) + dstBgnByte;

        lclByteCnt = byteCntM1;

        /* check if first pixel aligned */
        if( pxShift != 0 )
        {
            /* no, align it */
            pxBit = (UINT8) (shiftMask >> pxShift);
            while(pxBit > 0 )
            {
                *dstPtr = *dstPtr ^ pxBit;

                pxBit = (pxBit >> shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
            dstPtr++;
        }

        /* do the whole bytes */
        while( lclByteCnt >= lclShift )
        {
            /* for each UINT8 in the row */
            *dstPtr = ~*dstPtr;

            dstPtr++;
            lclByteCnt -= lclShift;
        }

        /* do the last pixels if any */
        if(lclByteCnt >= 0 )
        {
            if( shfCnt == 2 )
            {
                pxBit = 0xc0;
            }
            else
            {
                pxBit = 0xf0;
            }
            while( lclByteCnt-- >= 0 )
            {
                *dstPtr = *dstPtr ^ pxBit;
                pxBit = (pxBit >> shfCnt);
            }
        }
        /* advance to next row */
        lclYmin++;
    }
}


/***************************************************************************
* FUNCTION
*
*    M2B4F_SetDestM2_4Bit
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
VOID M2B4F_SetDestM2_4Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxShift, lclShift;
    UINT8 pxBit;

    lclLineCnt = lineCntM1;
    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;

    lclShift = (1 << flipMask);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

        lclByteCnt = byteCntM1;

        /* check if first pixel aligned */
        if( pxShift != 0 )
        {
            /* no, align it */
            pxBit = (UINT8) (shiftMask >> pxShift);
            while( pxBit > 0 )
            {
                *dstPtr = (*dstPtr & ~pxBit) | (pxBit & lclpnColr);

                pxBit = (pxBit >> shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
            dstPtr++;
        }

        /* do the whole bytes */
        while( lclByteCnt >= lclShift )
        {
            /* for each UINT8 in the row */
            *dstPtr = lclpnColr;

            dstPtr++;
            lclByteCnt -= lclShift;
        }

        /* do the last pixels if any */
        if( lclByteCnt >= 0 )
        {
            if( shfCnt == 2 )
            {
                pxBit = 0xc0;
            }
            else
            {
                pxBit = 0xf0;
            }
            while( lclByteCnt-- >= 0 )
            {
                *dstPtr = (*dstPtr & ~pxBit) | (pxBit & lclpnColr);
                pxBit = (pxBit >> shfCnt);
            }
        }
        /* advance to next row */
        dRect.Ymin++;
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4F_NotDestFillM2_4Bit
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
VOID M2B4F_NotDestFillM2_4Bit(VOID)
{
    /* invert the destination */
    M2B4F_InvertDestM2_4Bit();

    /* fill this rectangle */
    M2B4F_FillOXADestM2_4Bit();
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*   M2B4F_NotDestMonoFillM2_4Bit
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
VOID M2B4F_NotDestMonoFillM2_4Bit(VOID)
{
    /* invert the destination */
    M2B4F_InvertDestM2_4Bit();

    /* fill this rectangle */
    M2B4F_MonoFillOXADestM2_4Bit();
}

/***************************************************************************
* FUNCTION
*
*    M2B4F_MonoFillDestM2_4Bit
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
VOID M2B4F_MonoFillDestM2_4Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;
    INT32 pxShift;
    UINT8 pxBit;

    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;

    ptrnY *= patWidthInBytes;
    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        ptrnBt = (128 >> (dRect.Xmin & 7));
        ptrnByte = savePat->patData[ptrnX + ptrnY];

        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

        lclByteCnt = byteCntM1;
        pxBit = (UINT8) (shiftMask >> pxShift);

        while( lclByteCnt-- >= 0 )
        {
            if( ptrnByte & ptrnBt )
            {
                ptrnColr = lclpnColr;
            }
            else
            {
                ptrnColr = lclbkColr;
            }

            *dstPtr = (*dstPtr & ~pxBit) | (pxBit & ptrnColr);

            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                if( shfCnt == 2 )
                {
                    pxBit = 0xc0;
                }
                else
                {
                    pxBit = 0xf0;
                }
            }

            ptrnBt = ptrnBt >> 1;
            if( ptrnBt == 0 )
            {
                ptrnBt = 128;

                ptrnX++;
                if(ptrnX == patWidthInBytes)
                {
                    ptrnX = 0;
                }
                ptrnByte = savePat->patData[ptrnX + ptrnY];
            }
        }

        /* advance to next row */
        dRect.Ymin++;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength)
        {
            ptrnY = 0;
        }
    }
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M2B4F_FillOXADestM2_4Bit
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
VOID M2B4F_FillOXADestM2_4Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxShift, lclShift;
    UINT8 pxBit;
    INT32 logFnc;

    lclLineCnt = lineCntM1;
    pxShift    = (dRect.Xmin & firstOffset) * shfCnt;
    lclShift   = (1 << flipMask);

    /* only two lower bits needed */
    logFnc     = (dstClass & 3);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

        lclByteCnt = byteCntM1;

        /* check if first pixel aligned */
        if( pxShift != 0 )
        {
            /* no, align it */
            pxBit = (UINT8) (shiftMask >> pxShift);
            while( pxBit > 0 )
            {
                switch (logFnc)
                {
                case 0: /* handled elsewhere */
                case 1: /* "OR" */
                    *dstPtr = *dstPtr | (pxBit & lclpnColr);
                    break;
                case 2: /* "XOR" */
                    *dstPtr = *dstPtr ^ (pxBit & lclpnColr);
                    break;
                case 3: /* "AND" */
                    *dstPtr = *dstPtr & (~pxBit | (pxBit & lclpnColr));
                }

                pxBit = (pxBit >> shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
            dstPtr++;
        }

        /* do the whole bytes */
        while( lclByteCnt >= lclShift )
        {
            /* for each UINT8 in the row */
            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
                *dstPtr = *dstPtr | lclpnColr;
                break;
            case 2: /* "XOR" */
                *dstPtr = *dstPtr ^ lclpnColr;
                break;
            case 3: /* "AND" */
                *dstPtr = *dstPtr & lclpnColr;
            }
            dstPtr++;
            lclByteCnt -= lclShift;
        }

        /* do the last pixels if any */
        if( lclByteCnt >= 0 )
        {
            if( shfCnt == 2 )
            {
                pxBit = 0xc0;
            }
            else
            {
                pxBit = 0xf0;
            }
            while(lclByteCnt-- >= 0 )
            {
                switch (logFnc)
                {
                case 0: /* handled elsewhere */
                case 1: /* "OR" */
                    *dstPtr = *dstPtr | (pxBit & lclpnColr);
                    break;
                case 2: /* "XOR" */
                    *dstPtr = *dstPtr ^ (pxBit & lclpnColr);
                    break;
                case 3: /* "AND" */
                    *dstPtr = *dstPtr & (~pxBit | (pxBit & lclpnColr));
                }
                pxBit = (pxBit >> shfCnt);
            }
        }
        /* advance to next row */
        dRect.Ymin++;
    }
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M2B4F_MonoFillOXADestM2_4Bit
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
VOID M2B4F_MonoFillOXADestM2_4Bit(VOID)
{
    INT32  lclLineCnt, lclByteCnt;
    INT32  ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;
    INT32  pxShift;
    UINT8  pxBit;
    INT32  logFnc;

    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY    = dRect.Ymin % patHeight;
    ptrnY   *= patWidthInBytes;
    pxShift  = (dRect.Xmin & firstOffset) * shfCnt;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX    = dRect.Xmin % patWidthInBytes;
        ptrnBt   = (128 >> (dRect.Xmin & 7));
        ptrnByte = savePat->patData[ptrnX + ptrnY];

        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

        lclByteCnt = byteCntM1;
        pxBit = (UINT8) (shiftMask >> pxShift);

        while( lclByteCnt-- >= 0 )
        {
            if( ptrnByte & ptrnBt )
            {
                ptrnColr = lclpnColr;
            }
            else
            {
                ptrnColr = lclbkColr;
            }

            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
                *dstPtr = *dstPtr | (pxBit & ptrnColr);
                break;
            case 2: /* "XOR" */
                *dstPtr = *dstPtr ^ (pxBit & ptrnColr);
                break;
            case 3: /* "AND" */
                *dstPtr = *dstPtr & (~pxBit | (pxBit & ptrnColr));
            }

            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                if( shfCnt == 2 )
                {
                    pxBit = 0xc0;
                }
                else
                {
                    pxBit = 0xf0;
                }
            }

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
        dRect.Ymin++;

        ptrnY += patWidthInBytes;
        if( ptrnY == patLength)
        {
            ptrnY = 0;
        }
    }
}

#endif /* FILL_PATTERNS_SUPPORT */

#endif /* INCLUDE_2_4_BIT */


