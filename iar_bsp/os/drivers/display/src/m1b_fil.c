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
*  m1b_fil.c
*
* DESCRIPTION
*
*  This file contains the 1-bit memory Fill functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M1BF_Fill1Bit
*  M1BF_SolidFillM1Bit
*  M1BF_MonoPatternFillM1Bit
*  M1BF_DrawSolidFillRectM1Bit
*  M1BF_SetDestM1Bit
*  M1BF_FillOXADestM1Bit
*  M1BF_NotDestFillM1Bit
*  M1BF_InvertDestM1Bit
*  M1BF_MonoFillDestM1Bit
*  M1BF_MonoFillOXADestM1Bit
*  M1BF_NotDestMonoFillM1Bit
*
* DEPENDENCIES
*
*  display_config.h
*  display_inc.h
*  nucleus.h
*  nu_kernel.h
*  nu_drivers.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"

#include "drivers/display_config.h"
#include "drivers/display_inc.h"

#ifdef INCLUDE_1_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

extern  UINT32  lclpnColr, lclbkColr;

/***************************************************************************
* FUNCTION
*
*    M1BF_Fill1Bit
*
* DESCRIPTION
*
*    A special case optimization for MultiFill, monochrome to 1 bit memory,
*    rectangular and/or region clipping with color translation. M1BF_Fill1Bit
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
VOID M1BF_Fill1Bit(blitRcd *fillRec)
{
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
        M1BF_MonoPatternFillM1Bit(fillRec);
    }
    else

#endif /* FILL_PATTERNS_SUPPORT */

    {
        M1BF_SolidFillM1Bit(fillRec);
    }
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

      /* Call the function post-processing task. */
      SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */
}

/***************************************************************************
* FUNCTION
*
*    M1BF_SolidFillM1Bit
*
* DESCRIPTION
*
*    Solid fill for 1 bit per pixel.
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
VOID M1BF_SolidFillM1Bit(blitRcd *fillRec )
{
    rect   clipR;
    rect   *rListPtr;
    INT32  blitMayOverlap = NU_FALSE;
    INT32  isLine         = NU_FALSE;
    INT16  done           = NU_FALSE;

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

    /* Convert the color to required format. */
    COLOR_CONVERT(pnColr, lclpnColr);
    COLOR_CONVERT(bkColr, lclbkColr);

    if( !done )
    {
        M_PAUSE(dstBmap);

        dstWidth = dstBmap->pixBytes;

        /* determine number of bits per pixel */
        shfCnt      = 1;
        flipMask    = 3;
        shiftMask   = 0x80;
        firstOffset = 0x07;

        dstClass = fillRec->blitRop & 0x0f;

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M1BF_SetDestM1Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M1BF_FillOXADestM1Bit;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M1BF_NotDestFillM1Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M1BF_InvertDestM1Bit;
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
                        if( CLIP_Check_YX_Band( &dRect, rListPtr, &clipR, &rectCnt) )
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
                    if(clipToRegionFlag != 0 )
                    {
                        /* yes, decide whether to copy
                        top->bottom or bottom->top */
                        FillDrawer = &M1BF_DrawSolidFillRectM1Bit;

                        if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                        {
                            break;
                        }
                        continue;
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                /* blit the rectangle */
                M1BF_DrawSolidFillRectM1Bit(fillRec);
            }
        }
    }
    nuResume(dstBmap);
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M1BF_MonoPatternFillM1Bit
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
VOID M1BF_MonoPatternFillM1Bit(blitRcd *fillRec )
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

    /* Convert the color to required format. */
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
    shfCnt      = 1;
    flipMask    = 3;
    shiftMask   = 0x80;
    firstOffset = 0x07;

    dstClass = fillRec->blitRop & 0x0f;

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* Memory non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M1BF_MonoFillDestM1Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M1BF_MonoFillOXADestM1Bit;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M1BF_NotDestMonoFillM1Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M1BF_InvertDestM1Bit;
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
        /* this has the effect of noting the pen color for all operations during this call */
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
        bkColr = 0; /* sets all source bits to 0 */
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

            /* do we need to worry about clipping ? */
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
                    FillDrawer = &M1BF_DrawSolidFillRectM1Bit;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M1BF_DrawSolidFillRectM1Bit(fillRec);
        }
    }
    nuResume(dstBmap);
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M1BF_DrawSolidFillRectM1Bit
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
VOID M1BF_DrawSolidFillRectM1Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = (dRect.Xmin >> flipMask);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

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
*    M1BF_InvertDestM1Bit
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
VOID M1BF_InvertDestM1Bit(VOID)
{
    INT32 lclYmin, lclShift;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxShift;
    UINT8 pxBit;

    lclLineCnt = lineCntM1;
    lclYmin    = dRect.Ymin;
    pxShift    = (dRect.Xmin & firstOffset);
    lclShift   = 8;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + lclYmin)) + dstBgnByte;
        lclByteCnt = byteCntM1;

        /* check if first pixel aligned */
        if( pxShift != 0 )
        {
            /* no, align it */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (UINT8) (shiftMask >> pxShift);
            while( pxBit > 0 )
            {
                *dstPtr = *dstPtr ^ pxBit;

                pxBit = (pxBit >> shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
#else
            pxBit = 0x01 << pxShift;
            while( pxBit > 0 )
            {
                *dstPtr = *dstPtr ^ pxBit;

                pxBit = (pxBit << shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
#endif
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
#ifdef BIG_ENDIAN_GRFX
        if( lclByteCnt >= 0 )
        {
            pxBit = 0x80;

            while( lclByteCnt-- >= 0 )
            {
                *dstPtr = *dstPtr ^ pxBit;

                pxBit = (pxBit >> shfCnt);
            }
        }
#else
        if( lclByteCnt >= 0 )
        {
            pxBit = 0x01;

            while( lclByteCnt-- >= 0 )
            {
                *dstPtr = *dstPtr ^ pxBit;

                pxBit = (pxBit << shfCnt);
            }
        }
#endif
        /* advance to next row */
        lclYmin++;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BF_SetDestM1Bit
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
VOID M1BF_SetDestM1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxShift, lclShift;
    UINT8 pxBit;

    lclLineCnt = lineCntM1;
    pxShift    = (dRect.Xmin & firstOffset);
    lclShift   = 8;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;
        lclByteCnt = byteCntM1;

        /* check if first pixel aligned */
        if( pxShift != 0 )
        {
            /* no, align it */
#ifdef BIG_ENDIAN_GRFX
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
#else
            pxBit = 0x01 << pxShift;
            while( pxBit > 0 )
            {
                *dstPtr = (*dstPtr & ~pxBit) | (pxBit & lclpnColr);

                pxBit = (pxBit << shfCnt);

                lclByteCnt--;
                if( lclByteCnt < 0 )
                {
                    break;
                }
            }
#endif
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
#ifdef BIG_ENDIAN_GRFX
        if( lclByteCnt >= 0 )
        {
            pxBit = 0x80;

            while( lclByteCnt-- >= 0 )
            {
                *dstPtr = (*dstPtr & ~pxBit) | (pxBit & lclpnColr);
                pxBit = (pxBit >> shfCnt);
            }
        }
#else
        if( lclByteCnt >= 0 )
        {
            pxBit = 0x01;

            while( lclByteCnt-- >= 0 )
            {
                *dstPtr = (*dstPtr & ~pxBit) | (pxBit & lclpnColr);

                pxBit = (pxBit << shfCnt);
            }
        }
#endif
        /* advance to next row */
        dRect.Ymin++;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BF_NotDestFillM1Bit
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
VOID M1BF_NotDestFillM1Bit(VOID)
{
    /* invert the destination */
    M1BF_InvertDestM1Bit();

    /* fill this rectangle */
    M1BF_FillOXADestM1Bit();
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M1BF_NotDestMonoFillM1Bit
*
* DESCRIPTION
*
*    Handles memory "not dst" by inverting the destination first.
*
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
VOID M1BF_NotDestMonoFillM1Bit(VOID)
{
    /* invert the destination */
    M1BF_InvertDestM1Bit();

    /* fill this rectangle */
    M1BF_MonoFillOXADestM1Bit();
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M1BF_MonoFillDestM1Bit
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
VOID M1BF_MonoFillDestM1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;
    INT32 pxShift;
    UINT8 pxBit;

    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY   = dRect.Ymin % patHeight;
    ptrnY   *= patWidthInBytes;
    pxShift =  (dRect.Xmin & firstOffset);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX      = dRect.Xmin % patWidthInBytes;
        ptrnBt     = (128 >> (dRect.Xmin & 7));
        ptrnByte   = savePat->patData[ptrnX + ptrnY];

        dstPtr     = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

        lclByteCnt = byteCntM1;

#ifdef BIG_ENDIAN_GRFX
        pxBit      = (UINT8) (shiftMask >> pxShift);
#else
        pxBit      = (UINT8) 0x01 << pxShift;
#endif
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

#ifdef BIG_ENDIAN_GRFX
            pxBit   = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                pxBit = 0x80;
            }
#else
            pxBit   = (pxBit << shfCnt);
            if( pxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                pxBit = 0x01;
            }
#endif

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
        if( ptrnY == patLength )
        {
            ptrnY = 0;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BF_FillOXADestM1Bit
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
VOID M1BF_FillOXADestM1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxShift, lclShift;
    UINT8 pxBit;
    INT32 logFnc;

    lclLineCnt = lineCntM1;
    pxShift    = (dRect.Xmin & firstOffset);
    lclShift   = 8;

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
#ifdef BIG_ENDIAN_GRFX
            pxBit = (UINT8) (shiftMask >> pxShift);
#else
            pxBit = (UINT8) (0x01 << pxShift);
#endif
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

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> shfCnt);
#else
                pxBit = (pxBit << shfCnt);
#endif

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
#ifdef BIG_ENDIAN_GRFX
           pxBit = 0x80;
#else
           pxBit = 0x01;
#endif
            while( lclByteCnt-- >= 0 )
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
#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> shfCnt);
#else
                pxBit = (pxBit << shfCnt);
#endif
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
*    M1BF_MonoFillOXADestM1Bit
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
VOID M1BF_MonoFillOXADestM1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;
    INT32 pxShift;
    UINT8 pxBit;
    INT32 logFnc;

    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY   = dRect.Ymin % patHeight;
    ptrnY   *= patWidthInBytes;
    pxShift =  (dRect.Xmin & firstOffset);

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX       = dRect.Xmin % patWidthInBytes;
        ptrnBt      = (128 >> (dRect.Xmin & 7));
        ptrnByte    = savePat->patData[ptrnX + ptrnY];
        dstPtr      = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;
        lclByteCnt  = byteCntM1;

#ifdef BIG_ENDIAN_GRFX
        pxBit       = (UINT8) (shiftMask >> pxShift);
#else
        pxBit       = (UINT8) (0x01 << pxShift);
#endif

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

#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                pxBit = 0x80;
            }
#else
            pxBit = (pxBit << shfCnt);
            if( pxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                pxBit = 0x01;
            }
#endif

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

#endif /* INCLUDE_1_BIT */


