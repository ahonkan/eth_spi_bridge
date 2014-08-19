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
*  m32b_fil.c
*
* DESCRIPTION
*
*  This file contains the 32-bit memory Fill functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M32BF_Fill32Bit
*  M32BF_FillOXADestMem32
*  M32BF_InvertDestM32Bit
*  M32BF_InvertDestMem32
*  M32BF_MonoFillDestMem32
*  M32BF_MonoFillOXADestMem32
*  M32BF_MonoPatternFillMem32
*  M32BF_MultiColorFillMem32
*  M32BF_MultiColorFillRectM32
*  M32BF_NotDestFillMem32
*  M32BF_NotDestMonoFillMem32
*  M32BF_SetDestM32Bit
*  M32BF_SetDestMem32
*  M32BF_SolidFillMem32
*  M32BF_SolidFillRectM32
*
* DEPENDENCIES
*
*  nucleus.h
*  nu_kernel.h
*  nu_ui.h
*  nu_drivers.h
*  display_inc.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/nu_ui.h"
#include "drivers/nu_drivers.h"
#include "drivers/display_inc.h"

#ifdef INCLUDE_32_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

extern UINT32 lclpnColr32, lclbkColr32;

/***************************************************************************
* FUNCTION
*
*    M32BF_Fill32Bit
*
* DESCRIPTION
*
*    A special case optimization for MultiFill, monochrome to 32 bit memory,
*    rectangular and/or region clipping with color translation.
*    M32BF_Fill32Bit forms the outer processing loop for this mode
*    which internally dispatches to one of several other optimizations
*    based upon the specific rasterOp transfer mode and LCD destination.
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
VOID M32BF_Fill32Bit(blitRcd *fillRec)
{
    /* Figure out whether this is a multicolor pattern, and, if so, whether the
       raster op is replace. All non-multicolor patterns are handled here, as are
       replace-op multicolor, but non-replace multicolor patterns are handled by
       the multiplane fill handler. */

    lclPortMask = fillRec->blitMask;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
    SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    HideCursor();
#endif

#ifdef  FILL_PATTERNS_SUPPORT

    /* save the pattern header address here temporarily */
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
                M32BF_MultiColorFillMem32(fillRec);
            }
        }
        else
        {
            /* we'll do a monocolor pattern fill locally */
            M32BF_MonoPatternFillMem32(fillRec);
        }
    }
    else

#endif /* FILL_PATTERNS_SUPPORT */
        
    {
        /* we'll do a solid fill locally */
        M32BF_SolidFillMem32(fillRec);
    }

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M32BF_MultiColorFillMem32
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
VOID M32BF_MultiColorFillMem32(blitRcd *fillRec)
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    /* retrieve the pattern header address  */
    savePat  = patStart;
    rectCnt  = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap  = fillRec->blitDmap;

    /* height in rows  */
    patHeight       = savePat->patHeight;
    patWidthInBytes = 4 * savePat->patWidth;
    patLength       = patWidthInBytes * patHeight;

    /* distance from one plane to the next */
    patPlaneOffset  = patLength;

    M_PAUSE(dstBmap);

    /* set up clipping */
    if( CLIP_Set_Up_Clip(fillRec, &clipR, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        while( --rectCnt >= 0 )
        {
            dRect = *rListPtr++;

            /* do we need to worry about clipping? */
            if( clipToRectFlag != 0 )
            {
                /* yes, do trivial reject */
                if( dRect.Xmin < clipR.Xmin)
                {
                    dRect.Xmin  = clipR.Xmin;
                }
                if( dRect.Ymin < clipR.Ymin)
                {
                    dRect.Ymin  = clipR.Ymin;
                }
                if( dRect.Xmax > clipR.Xmax)
                {
                    dRect.Xmax  = clipR.Xmax;
                }
                if( dRect.Ymax > clipR.Ymax)
                {
                    dRect.Ymax  = clipR.Ymax;
                }

                if( dRect.Ymin >= dRect.Ymax)
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
                    FillDrawer = &M32BF_MultiColorFillRectM32;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }

                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }
            /* blit the rectangle */
            M32BF_MultiColorFillRectM32(fillRec);
        }
    }
    nuResume(dstBmap);
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M32BF_SolidFillMem32
*
* DESCRIPTION
*
*    Solid fill for 32 bit per pixel.
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
VOID M32BF_SolidFillMem32(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  = fillRec->blitCnt;
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
            pnColr32 = fillRec->blitBack;
        }
    }
    else
    {
        pnColr32 = fillRec->blitFore;
    }

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr32, lclpnColr32);
    COLOR_CONVERT(bkColr32, lclbkColr32);

    if( !done )
    {
        M_PAUSE(dstBmap);

        dstClass = fillRec->blitRop & 0xf0;

        if (!dstClass)
        {
            dstClass = fillRec->blitRop & 0x0f;
        }

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M32BF_SetDestMem32;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M32BF_FillOXADestMem32;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M32BF_NotDestFillMem32;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M32BF_InvertDestMem32;
            break;
        case 32:

#ifdef  GLOBAL_ALPHA_SUPPORT
        
            optPtr = &M32_FillTransparency;
        
#else

            optPtr = &SCREENS_Nop;

#endif  /* GLOBAL_ALPHA_SUPPORT */

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
                        FillDrawer = &M32BF_SolidFillRectM32;

                        if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                        {
                            break;
                        }
                        continue;
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                /* blit the rectangle */
                M32BF_SolidFillRectM32(fillRec);
            }
        }
    }
    nuResume(dstBmap);
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M32BF_MonoPatternFillMem32
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
VOID M32BF_MonoPatternFillMem32(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap  = fillRec->blitDmap;
    pnColr32 = fillRec->blitFore;
    bkColr32 = fillRec->blitBack;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr32, lclpnColr32);
    COLOR_CONVERT(bkColr32, lclbkColr32);

    /* retrieve the pattern header address */
    savePat  = patStart;

    /* skip over the pattern header to the bit pattern  */
    ptrnData = savePat->patData;

    /* height in rows  */
    patHeight       = savePat->patHeight;
    patWidthInBytes = savePat->patWidth >> 3;
    patLength       = patWidthInBytes * patHeight;

    /* distance from one plane to the next */
    patPlaneOffset  = patLength;

    M_PAUSE(dstBmap);

    dstClass = fillRec->blitRop & 0xf0;

    if (!dstClass)
    {
        dstClass = fillRec->blitRop & 0x0f;
    }

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* Memory non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M32BF_MonoFillDestMem32;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M32BF_MonoFillOXADestMem32;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M32BF_NotDestMonoFillMem32;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M32BF_InvertDestMem32;
        break;

    case 32:

#ifdef  GLOBAL_ALPHA_SUPPORT
        
        optPtr = &M32_FillTransparency;
        
#else

        optPtr = &SCREENS_Nop;

#endif  /* GLOBAL_ALPHA_SUPPORT */

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
        bkColr32 = ~bkColr32;
        pnColr32 = ~pnColr32;
        lclbkColr32 = ~lclbkColr32;
        lclpnColr32 = ~lclpnColr32;
    case 0:     /* zREPz  */
    case 1:     /* zORz   */
    case 2:     /* zXORz  */
    case 7:     /* zANDz  */
    case 9:     /* zORNz  */
    case 10:    /* zNOPz  */
    case 11:    /* zANDNz */
        break;

    case 8: /* zCLEARz */
        /* set all source bits to 0 */
        bkColr32 = 0;
        pnColr32 = 0;
        lclbkColr32 = 0;
        lclpnColr32 = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* set all source bits to 1 */
        bkColr32 = ~0;
        pnColr32 = ~0;
        lclbkColr32 = ~0;
        lclpnColr32 = ~0;
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
            if(clipToRectFlag != 0 )
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
                    FillDrawer = &M32BF_SolidFillRectM32;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M32BF_SolidFillRectM32(fillRec);
        }
    }
    nuResume(dstBmap);
}

/***************************************************************************
* FUNCTION
*
*    M32BF_MultiColorFillRectM32
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
VOID  M32BF_MultiColorFillRectM32(blitRcd *blitRec)
{
    INT32 lclByteCnt, lclLineCnt;
    INT32 ptrnX, ptrnY, ptrnCntr;

#ifdef USING_DIRECT_X
    rect scrnRect;
    scrnRect = dRect;
#endif

    lclLineCnt = dRect.Ymax - dRect.Ymin - 1;

    dstBgnByte  = 4 * dRect.Xmin;           /* 4 bytes per pixel. */

    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    dstNextRow = dstBmap->pixBytes - (4 * byteCntM1) - 4;


    /* start of pattern alignment */
    ptrnY  = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte ;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = (4 * dRect.Xmin) % patWidthInBytes;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
            ptrnCntr = ptrnX + ptrnY;

            *dstPtr++ = savePat->patData[ptrnCntr++];
            *dstPtr++ = savePat->patData[ptrnCntr++];
            *dstPtr++ = savePat->patData[ptrnCntr];
            dstPtr++;
            ptrnCntr++;
            ptrnX += 4;

            if( ptrnX == patWidthInBytes)
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
*    M32BF_DrawSolidFillRectM32
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
VOID  M32BF_SolidFillRectM32(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;
    scrnRect = dRect;
#endif

    lineCntM1 = dRect.Ymax - dRect.Ymin - 1;

    dstBgnByte  = 4 * dRect.Xmin;

    byteCntM1 = dRect.Xmax - dRect.Xmin - 1;

    dstNextRow = dstBmap->pixBytes - (4 * byteCntM1) - 4;

    dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

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
*    M32BF_InvertDestMem32
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
VOID  M32BF_InvertDestMem32(VOID)
{
    UINT8  *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    UINT32 i;

    /* set up local pointers */
    lclDstPtr  = dstPtr;
    lclLineCnt = lineCntM1;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = (4 * byteCntM1) + 3;

        i=1;
        while( lclByteCnt-- >= 0 )
        {
            /* Do not invert the alpha value.     */
            /* This condition will make sure that */
            if(i%4 != 0)
            {
                /* for each UINT8  in the row */
                *lclDstPtr = ~(*lclDstPtr);
            }
            i++;
            lclDstPtr++;
        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M32BF_SetDestMem32
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
VOID  M32BF_SetDestMem32(VOID)
{
    register UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    register INT32 lclLineCnt, lclByteCnt;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    register UINT32 pnColr;
    register UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* set up local colors */
    pnColrR = (UINT8)((lclpnColr32 >> 16) & 0xFF);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xFF);
    pnColrB = (UINT8)(lclpnColr32  & 0xFF);

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    pnColr = (UINT32)pnColrA << 24 | (UINT32)pnColrR << 16 |
                (UINT32)pnColrG << 8 | (UINT32)pnColrB; /* ARGB */
#endif

    /* In this case, there is no source alpha available. */
    /* If destination has alpha, then use it */
    /* If alpha is absent then it will not affect any writes */
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            double  alpha_value;

#ifdef  GLOBAL_ALPHA_SUPPORT
            
            /* Get the alpha value to blend the pixel */
            if (alpha_level != 0.0)
            {
                alpha_value = alpha_level;
            }
            else
                
#endif  /* GLOBAL_ALPHA_SUPPORT */

            {
                alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);
            }

            /* for each UINT8  in the row */
            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
            lclDstPtr++;
            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
            lclDstPtr++;
            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
            lclDstPtr++;

            /* Skip Alpha Byte */
            lclDstPtr++;
#else
            /* for each UINT8  in the row */
            
            *((UINT32 *)lclDstPtr) = pnColr;
            lclDstPtr += 4;
#endif
        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M32BF_NotDestFillMem32
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
VOID  M32BF_NotDestFillMem32(VOID)
{
    /* invert the destination */
    M32BF_InvertDestMem32();

    /* fill this rectangle */
    M32BF_FillOXADestMem32();
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*   M32BF_NotDestMonoFillMem32
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
VOID  M32BF_NotDestMonoFillMem32(VOID)
{
    /* invert the destination */
    M32BF_InvertDestMem32();

    /* fill this rectangle */
    M32BF_MonoFillOXADestMem32();
}

/***************************************************************************
* FUNCTION
*
*    M32BF_MonoFillDestMem32
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr if the source is 1 else
*    bkColr24.
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
VOID  M32BF_MonoFillDestMem32(VOID)
{
    UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8  bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnUINT8 , ptrnBt;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
    UINT8 bkColrA = (UINT8)(lclbkColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* set up local colors */
    pnColrB = (UINT8)((lclpnColr32 >> 16) & 0xFF);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xFF);
    pnColrR = (UINT8)(lclpnColr32  & 0xFF);

    bkColrB = (UINT8)((lclbkColr32 >> 16) & 0xFF);
    bkColrG = (UINT8)((lclbkColr32 >> 8) & 0xFF);
    bkColrR = (UINT8)(lclbkColr32  & 0xFF);

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    /* In this case, there is no source alpha available.        */
    /* If destination has alpha, then use it.                    */
    /* If alpha is absent then it will not affect any writes.   */
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        ptrnBt = (128 >> (dRect.Xmin & 7));
        ptrnUINT8  = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;

        while ( lclByteCnt-- >= 0 )
        {
            
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            /* Get the alpha value to blend the pixel */
            double alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);

            /* for each UINT8  in the row */
            if( ptrnUINT8  & ptrnBt )
            {
                /* source is 1 */
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;
            }
            else
            {
                /* source is 0 */
                *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;
            }
#else
            /* for each UINT8  in the row */
            if( ptrnUINT8  & ptrnBt )
            {
                /* source is 1 */
                *lclDstPtr++ = pnColrB;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrA;
            }
            else
            {
                *lclDstPtr++ = bkColrB;
                *lclDstPtr++ = bkColrG;
                *lclDstPtr++ = bkColrR;
                *lclDstPtr++ = bkColrA;
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
                ptrnUINT8  = savePat->patData[ptrnX + ptrnY];
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
*    M32BF_FillOXADestMem32
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
VOID  M32BF_FillOXADestMem32(VOID)
{
    UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    /* set up local colors */
    pnColrB = (UINT8 ) ((lclpnColr32 >> 16) & 0xFF);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xFF);
    pnColrR = (UINT8)(lclpnColr32  & 0xFF);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            /* Get the alpha value to blend the pixel */
            double alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);

            /* for each UINT8  in the row */
            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr | pnColrB)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr | pnColrG)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr | pnColrR)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;

                break;

            case 2: /* "XOR" */

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr ^ pnColrB)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr ^ pnColrG)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr ^ pnColrR)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;

                break;

            case 3: /* "AND" */

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr & pnColrB)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr & pnColrG)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr & pnColrR)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;
            }

#else

            /* for each UINT8  in the row */
            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */

                *lclDstPtr = *lclDstPtr | pnColrB;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrA;
                lclDstPtr++;

                break;

            case 2: /* "XOR" */

                *lclDstPtr = *lclDstPtr ^ pnColrB;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrA;
                lclDstPtr++;

                break;

            case 3: /* "AND" */

                *lclDstPtr = *lclDstPtr & pnColrB;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrA;
                lclDstPtr++;
            }
#endif /* (HARDWARE_ALPHA_SUPPORTED == NU_FALSE) */

        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M32BF_MonoFillOXADestMem32
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
VOID  M32BF_MonoFillOXADestMem32(VOID)
{
    UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 ptrnX, ptrnY, ptrnByte , ptrnBt;

#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)
    double  alpha_value;
#else
    UINT8  pnColrA;
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        ptrnBt = (128 >> (dRect.Xmin & 7));
        ptrnByte  = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
            if( ptrnByte  & ptrnBt )
            {
                /* set up local colors */
#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
                pnColrA = (UINT8)(lclpnColr32 >> 24);
#endif
                pnColrB = (UINT8)((lclpnColr32 >> 16) & 0xFF);
                pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xFF);
                pnColrR = (UINT8)(lclpnColr32  & 0xFF);
            }
            else
            {
                /* set up local colors */
#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
                pnColrA = (UINT8)(lclbkColr32 >> 24);
#endif
                pnColrB = (UINT8)((lclbkColr32 >> 16) & 0xFF);
                pnColrG = (UINT8)((lclbkColr32 >> 8) & 0xFF);
                pnColrR = (UINT8)(lclbkColr32  & 0xFF);
            }

#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            /* Get the alpha value to blend the pixel */
            alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);

            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr | pnColrB)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr | pnColrG)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr | pnColrR)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;

                break;

            case 2: /* "XOR" */

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr ^ pnColrB)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr ^ pnColrG)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr ^ pnColrR)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;

                break;

            case 3: /* "AND" */

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr & pnColrB)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr & pnColrG)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr & pnColrR)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the Alpha Byte */
                lclDstPtr++;

            }

#else

            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */

                *lclDstPtr = *lclDstPtr | pnColrB;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrA;
                lclDstPtr++;

                break;

            case 2: /* "XOR" */

                *lclDstPtr = *lclDstPtr ^ pnColrB;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrA;
                lclDstPtr++;

                break;

            case 3: /* "AND" */

                *lclDstPtr = *lclDstPtr & pnColrB;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrA;
                lclDstPtr++;

            }

#endif

            ptrnBt = ptrnBt >> 1;
            if( ptrnBt == 0 )
            {
                ptrnBt = 128;

                ptrnX++;
                if( ptrnX == patWidthInBytes )
                {
                    ptrnX = 0;
                }
                ptrnByte  = savePat->patData[ptrnX + ptrnY];
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

#ifdef  GLOBAL_ALPHA_SUPPORT

VOID  M32_FillTransparency(VOID)
{
    UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    INT32 lclLineCnt, lclByteCnt;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* set up local colors */
    pnColrB = (UINT8)((lclpnColr32 >> 16) & 0xFF);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xFF);
    pnColrR = (UINT8)(lclpnColr32  & 0xFF);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            /* for each UINT8 in the row */
            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         pnColrR * (1.0-alpha_level));
            *lclDstPtr++;


            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         pnColrG*(1.0-alpha_level));
            *lclDstPtr++;


            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         pnColrB*(1.0-alpha_level));
            *lclDstPtr++;
            *lclDstPtr++;
#else
            /* for each UINT8 in the row */
            *lclDstPtr++ = pnColrB;
            *lclDstPtr++ = pnColrG;
            *lclDstPtr++ = pnColrR;
            *lclDstPtr++ = pnColrA;
#endif
        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

#endif  /* GLOBAL_ALPHA_SUPPORT */

#endif /* #ifdef INCLUDE_32_BIT */

