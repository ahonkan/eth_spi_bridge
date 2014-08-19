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
*  m24b_fil.c
*
* DESCRIPTION
*
*  This file contains the 24-bit memory Fill functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M24BF_Fill24Bit
*  M24BF_FillOXADestMem24
*  M24BF_InvertDestM24Bit
*  M24BF_InvertDestMem24
*  M24BF_MonoFillDestMem24
*  M24BF_MonoFillOXADestMem24
*  M24BF_MonoPatternFillMem24
*  M24BF_MultiColorFillMem24
*  M24BF_MultiColorFillRectM24
*  M24BF_NotDestFillMem24
*  M24BF_NotDestMonoFillMem24
*  M24BF_SetDestM24Bit
*  M24BF_SetDestMem24
*  M24BF_SolidFillMem24
*  M24BF_SolidFillRectM24
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

#ifdef INCLUDE_24_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

extern UINT32 lclpnColr24, lclbkColr24;

/***************************************************************************
* FUNCTION
*
*    M24BF_Fill24Bit
*
* DESCRIPTION
*
*    A special case optimization for MultiFill, monochrome to 24 bit memory,
*    rectangular and/or region clipping with color translation.
*    M24BF_Fill24Bit forms the outer processing loop for this mode
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
VOID M24BF_Fill24Bit(blitRcd *fillRec)
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
                M24BF_MultiColorFillMem24(fillRec);
            }
        }
        else
        {
            /* we'll do a monocolor pattern fill locally */
            M24BF_MonoPatternFillMem24(fillRec);
        }
    }
    else
        
#endif /* FILL_PATTERNS_SUPPORT */
        
    {
        /* we'll do a solid fill locally */
        M24BF_SolidFillMem24(fillRec);
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
*    M24BF_MultiColorFillMem24
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
VOID M24BF_MultiColorFillMem24(blitRcd *fillRec)
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
    patWidthInBytes = 3 * savePat->patWidth;
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
                    FillDrawer = &M24BF_MultiColorFillRectM24;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }

                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M24BF_MultiColorFillRectM24(fillRec);
        }
    }
    nuResume(dstBmap);
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M24BF_SolidFillMem24
*
* DESCRIPTION
*
*    Solid fill for 24 bit per pixel.
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
VOID M24BF_SolidFillMem24(blitRcd *fillRec )
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
            pnColr24 = fillRec->blitBack;
        }
    }
    else
    {
        pnColr24 = fillRec->blitFore;
    }

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr24, lclpnColr24);
    COLOR_CONVERT(bkColr24, lclbkColr24);

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
            optPtr = &M24BF_SetDestMem24;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M24BF_FillOXADestMem24;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M24BF_NotDestFillMem24;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M24BF_InvertDestMem24;
            break;
        case 32:    /* xAVGx */
            
#ifdef  GLOBAL_ALPHA_SUPPORT
        
            if(text_trans)
            {
                optPtr = &M24BD_Text_Transparency;
            }
            else
            {
                optPtr = &M24BD_Transparency;
            }
        
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
                        FillDrawer = &M24BF_SolidFillRectM24;

                        if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                        {
                            break;
                        }
                        continue;
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                /* blit the rectangle */
                M24BF_SolidFillRectM24(fillRec);
            }
        }
    }
    nuResume(dstBmap);
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M24BF_MonoPatternFillMem24
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
VOID M24BF_MonoPatternFillMem24(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap  = fillRec->blitDmap;
    pnColr24 = fillRec->blitFore;
    bkColr24 = fillRec->blitBack;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr24, lclpnColr24);
    COLOR_CONVERT(bkColr24, lclbkColr24);

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
        optPtr = &M24BF_MonoFillDestMem24;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M24BF_MonoFillOXADestMem24;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M24BF_NotDestMonoFillMem24;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M24BF_InvertDestMem24;
        break;

    case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
        if(text_trans)
        {
            optPtr = &M24BD_Text_Transparency;
        }
        else
        {
            optPtr = &M24BD_Transparency;
        }
        
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
        bkColr24 = ~bkColr24;
        pnColr24 = ~pnColr24;
        lclbkColr24 = ~lclbkColr24;
        lclpnColr24 = ~lclpnColr24;
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
        bkColr24 = 0;
        pnColr24 = 0;
        lclbkColr24 = 0;
        lclpnColr24 = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* set all source bits to 1 */
        bkColr24 = ~0;
        pnColr24 = ~0;
        lclbkColr24 = ~0;
        lclpnColr24 = ~0;
        break;

    case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
        /* set all source bits to 1 */
        bkColr24 = 0;
        pnColr24 = 0;
        lclbkColr24 = 0;
        lclpnColr24 = 0;

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
                    FillDrawer = &M24BF_SolidFillRectM24;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M24BF_SolidFillRectM24(fillRec);
        }
    }
    nuResume(dstBmap);
}

/***************************************************************************
* FUNCTION
*
*    M24BF_MultiColorFillRectM24
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
VOID  M24BF_MultiColorFillRectM24(blitRcd *blitRec)
{
    INT32 lclByteCnt, lclLineCnt;
    INT32 ptrnX, ptrnY, ptrnCntr;

#ifdef USING_DIRECT_X
    rect scrnRect;
    scrnRect = dRect;
#endif

    lclLineCnt = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte  = 3 * dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = dstBmap->pixBytes - (3 * byteCntM1) - 3;

    /* start of pattern alignment */
    ptrnY  = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte ;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = (3 * dRect.Xmin) % patWidthInBytes;
        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
            ptrnCntr = ptrnX + ptrnY;

            *dstPtr++ = savePat->patData[ptrnCntr++];
            *dstPtr++ = savePat->patData[ptrnCntr++];
            *dstPtr++ = savePat->patData[ptrnCntr];

            ptrnX += 3;
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
*    M24BF_DrawSolidFillRectM24
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
VOID  M24BF_SolidFillRectM24(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;
    scrnRect = dRect;
#endif

    lineCntM1 = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte  = 3 * dRect.Xmin;
    byteCntM1 = dRect.Xmax - dRect.Xmin - 1;

    dstNextRow = dstBmap->pixBytes - (3 * byteCntM1) - 3;

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
*    M24BF_InvertDestMem24
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
VOID  M24BF_InvertDestMem24(VOID )
{
    UINT8  *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;

    /* set up local pointers */
    lclDstPtr  = dstPtr;
    lclLineCnt = lineCntM1;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = (3 * byteCntM1) + 2;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
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
*    M24BF_SetDestMem24
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
VOID  M24BF_SetDestMem24(VOID )
{
    register UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    register INT32 lclLineCnt, lclByteCnt;

#ifdef      LCD_OPTIMIZE_FILL
     UINT32  pnColr[3];
#endif      /* LCD_OPTIMIZE_FILL */

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
    /* set up local colors */
    pnColrR = (UINT8)((lclpnColr24 >> 8) & 0xFF);
    pnColrG = (UINT8)((lclpnColr24 >> 16) & 0xFF);
    pnColrB = (UINT8)((lclpnColr24 >> 24) & 0xFF);
#else
    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);
#endif

#ifdef      LCD_OPTIMIZE_FILL

#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
    pnColr[0] = (UINT32)pnColrB << 24 | (UINT32)pnColrG << 16 |
                (UINT32)pnColrR << 8 | (UINT32)pnColrB; /* BGRB */

    pnColr[1] = (UINT32)pnColrG << 24 | (UINT32)pnColrR << 16 |
                (UINT32)pnColrB << 8 | (UINT32)pnColrG; /* GRBG */

    pnColr[2] = (UINT32)pnColrR << 24 | (UINT32)pnColrB << 16 |
                (UINT32)pnColrG << 8 | (UINT32)pnColrR; /* RBGR */
#else
    pnColr[0] = (UINT32)pnColrR << 24 | (UINT32)pnColrB << 16 |
                (UINT32)pnColrG << 8 | (UINT32)pnColrR; /* RBGR */

    pnColr[1] = (UINT32)pnColrG << 24 | (UINT32)pnColrR << 16 |
                (UINT32)pnColrB << 8 | (UINT32)pnColrG; /* GRBG */

    pnColr[2] = (UINT32)pnColrB << 24 | (UINT32)pnColrG << 16 |
                (UINT32)pnColrR << 8 | (UINT32)pnColrB; /* BGRB */
#endif

#endif      /* LCD_OPTIMIZE_FILL */

    while( lclLineCnt-- >= 0 )
    {
#ifdef      LCD_OPTIMIZE_FILL
        register UINT32  i = 0;
#endif      /* LCD_OPTIMIZE_FILL */

        /* for each row */
        lclByteCnt = byteCntM1;

#ifdef      LCD_OPTIMIZE_FILL

        lclByteCnt = (lclByteCnt << 1) + lclByteCnt + 3;

        /* Make destination pointers 4 byte align
           to perform optimization. */
        switch((UINT32)lclDstPtr & 0x03)
        {
        case 1:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
            *lclDstPtr++ = pnColrB;
            *lclDstPtr++ = pnColrG;
            *lclDstPtr++ = pnColrR;
#else
            *lclDstPtr++ = pnColrR;
            *lclDstPtr++ = pnColrG;
            *lclDstPtr++ = pnColrB;
#endif
            lclByteCnt -= 3;
            break;

        case 2:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
        	*lclDstPtr++ = pnColrB;
            *lclDstPtr++ = pnColrG;
#else
        	*lclDstPtr++ = pnColrR;
            *lclDstPtr++ = pnColrG;
#endif
            lclByteCnt -= 2;
            i = 2;
            break;

        case 3:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
        	*lclDstPtr++ = pnColrB;
#else
        	*lclDstPtr++ = pnColrR;
#endif
            lclByteCnt -= 1;
            i = 1;
            break;

        default:
            break;
        }

        while( lclByteCnt > 7 )
        {
            *((UINT32 *)lclDstPtr) = pnColr[i];
            lclDstPtr += 4;
            lclByteCnt -= 4;
            i++;

            if (i == 3) i = 0;

            /* for each UINT8 in the row */
            *((UINT32 *)lclDstPtr) = pnColr[i];
            lclDstPtr += 4;
            lclByteCnt -= 4;
            i++;

            if (i == 3) i = 0;
        }

        if ( lclByteCnt > 3 )
        {
            *((UINT32 *)lclDstPtr) = pnColr[i];
            lclDstPtr += 4;
            lclByteCnt -= 4;
            i++;

            if (i == 3) i = 0;
        }

        switch(lclByteCnt)
        {
        case 1:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
        	*lclDstPtr++ = (UINT8)(pnColr[i] & 0xFF);               /* R */
#else
        	*lclDstPtr++ = (UINT8)(pnColr[i] & 0xFF);
#endif
            break;

        case 2:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
            *lclDstPtr++ = (UINT8)(pnColr[i] & 0xFF);               /* G */
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0x00FF0000) >> 16); /* R */
#else
            *lclDstPtr++ = (UINT8)(pnColr[i] & 0xFF);
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0xFF00) >> 8);
#endif
            break;

        case 3:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0xFF000000) >> 24); /* B */
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0x00FF0000) >> 16); /* G */
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0x0000FF00) >> 8);  /* R */
#else
            *lclDstPtr++ = (UINT8)(pnColr[i] & 0xFF);
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0xFF00) >> 8);
            *lclDstPtr++ = (UINT8)((pnColr[i] & 0xFF0000) >> 16);
#endif
            break;
        }

#else

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
            *lclDstPtr++ = pnColrR;
            *lclDstPtr++ = pnColrG;
            *lclDstPtr++ = pnColrB;
        }

#endif      /* LCD_OPTIMIZE_FILL */

        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M24BF_NotDestFillMem24
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
VOID  M24BF_NotDestFillMem24(VOID )
{
    /* invert the destination */
    M24BF_InvertDestMem24();

    /* fill this rectangle */
    M24BF_FillOXADestMem24();
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*   M24BF_NotDestMonoFillMem24
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
VOID  M24BF_NotDestMonoFillMem24(VOID )
{
    /* invert the destination */
    M24BF_InvertDestMem24();

    /* fill this rectangle */
    M24BF_MonoFillOXADestMem24();
}

/***************************************************************************
* FUNCTION
*
*    M24BF_MonoFillDestMem24
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
VOID  M24BF_MonoFillDestMem24(VOID )
{
    UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8  bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnUINT8 , ptrnBt;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    bkColrR = (UINT8)(lclbkColr24 >> 16);
    bkColrG = (UINT8)((lclbkColr24 >> 8) & 0xff);
    bkColrB = (UINT8)(lclbkColr24  & 0xff);

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        ptrnBt = (128 >> (dRect.Xmin & 7));
        ptrnUINT8  = savePat->patData[ptrnX + ptrnY];

        lclByteCnt = byteCntM1;

        while ( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
            if( ptrnUINT8  & ptrnBt )
            {
                /* source is 1 */
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrB;
            }
            else
            {   /* source is 0 */
                *lclDstPtr++ = bkColrR;
                *lclDstPtr++ = bkColrG;
                *lclDstPtr++ = bkColrB;
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
*    M24BF_FillOXADestMem24
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
VOID  M24BF_FillOXADestMem24(VOID )
{
    UINT8  *lclDstPtr, pnColrR, pnColrG, pnColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    /* set up local colors */
    pnColrR = (UINT8 ) (lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8  in the row */
            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
                *lclDstPtr = *lclDstPtr | pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | pnColrB;
                lclDstPtr++;
                break;
            case 2: /* "XOR" */
                *lclDstPtr = *lclDstPtr ^ pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ pnColrB;
                lclDstPtr++;
                break;
            case 3: /* "AND" */
                *lclDstPtr = *lclDstPtr & pnColrR;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrG;
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & pnColrB;
                lclDstPtr++;
            }

        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
    }
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M24BF_MonoFillOXADestMem24
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
VOID  M24BF_MonoFillOXADestMem24(VOID )
{
    UINT8  *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 ptrnX, ptrnY, ptrnByte , ptrnBt, ptrnColr;

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
                ptrnColr = lclpnColr24;
            }
            else
            {
                ptrnColr = lclbkColr24;
            }

            switch (logFnc)
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
                *lclDstPtr = *lclDstPtr | (ptrnColr >> 16);
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | ((ptrnColr >> 8) & 0xff);
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr | (ptrnColr & 0xff);
                lclDstPtr++;
                break;
            case 2: /* "XOR" */
                *lclDstPtr = *lclDstPtr ^ (ptrnColr >> 16);
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ ((ptrnColr >> 8) & 0xff);
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr ^ (ptrnColr & 0xff);
                lclDstPtr++;
                break;
            case 3: /* "AND" */
                *lclDstPtr = *lclDstPtr & (ptrnColr >> 16);
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & ((ptrnColr >> 8) & 0xff);
                lclDstPtr++;
                *lclDstPtr = *lclDstPtr & (ptrnColr & 0xff);
                lclDstPtr++;
            }

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

#endif /* #ifdef INCLUDE_24_BIT */

