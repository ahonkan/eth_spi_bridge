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
*  m16b_fil.c
*
* DESCRIPTION
*
*  This file contains the 16-bit memory Fill functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M16BF_Fill16Bit
*  M16BF_FillOXADestMem16
*  M16BF_InvertDestM16B
*  M16BF_InvertDestMem16
*  M16BF_MonoPatternFillMem16
*  M16BF_MonoFillDestMem16
*  M16BF_MonoFillOXADestMem16
*  M16BF_MultiColorFillMem16
*  M16BF_MultiColorFillRectM16
*  M16BF_NotDestFillMem16
*  M16BF_NotDestMonoFillMem16
*  M16BF_SetDestM16B
*  M16BF_SetDestMem16
*  M16BF_SolidFillMem16
*  M16BF_SolidFillRectM16
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

#ifdef INCLUDE_16_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)) || defined (SMART_LCD)

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) || defined (SMART_LCD) */

extern UINT32 lclpnColr16, lclbkColr16;

/***************************************************************************
* FUNCTION
*
*    M16BF_Fill16Bit
*
* DESCRIPTION
*
*    A special case optimization for MultiFill, monochrome to memory,
*    rectangular and/or region clipping with color translation.
*    M16BF_Fill16Bit forms the outer processing loop for this mode
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
VOID M16BF_Fill16Bit(blitRcd *fillRec)
{

    /* Figure out whether this is a multicolor pattern, and, if so, whether the
       raster op is replaced. All non-multicolor patterns are handled here, as are
       replace-op multicolor, but non-replace multicolor patterns are handled by
       the multiplane fill handler.  */

    lclPortMask = fillRec->blitMask;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
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
            if( (fillRec->blitRop & 0x0f) > 0)
            {
                /* non-replace multicolor, handle thru general plane filler */
                fillRec->blitDmap->mapWinPlane = -1;
            }
            else
            {
                /* we'll do a replace multicolor fill locally */
                M16BF_MultiColorFillMem16(fillRec);
            }
        }
        else
        {
            /* we'll do a monocolor pattern fill locally */
            M16BF_MonoPatternFillMem16(fillRec);
        }
    }
    else
        
#endif /* FILL_PATTERNS_SUPPORT */
        
    {
        /* we'll do a solid fill locally */
        M16BF_SolidFillMem16(fillRec);
    }

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M16BF_MultiColorFillMem16
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
VOID M16BF_MultiColorFillMem16(blitRcd *fillRec)
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
    patWidthInBytes = savePat->patWidth << 1;
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

            /* do we need to worry about clipping */
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
                    FillDrawer = &M16BF_MultiColorFillRectM16;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0)
                    {
                        break;
                    }

                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M16BF_MultiColorFillRectM16(fillRec);
        }
    }
    nuResume(dstBmap);
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M16BF_SolidFillMem16
*
* DESCRIPTION
*
*    Solid fill for 16 bit per pixel.
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
VOID M16BF_SolidFillMem16(blitRcd *fillRec )
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
            pnColr16 = fillRec->blitBack;
        }
    }
    else
    {
        pnColr16 = fillRec->blitFore;
    }

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr16, lclpnColr16);
    COLOR_CONVERT(bkColr16, lclbkColr16);

    if( !done )
    {
        M_PAUSE(dstBmap);

        dstClass = fillRec->blitRop & 0xf0;

        if (!dstClass)
        {
            dstClass = fillRec->blitRop & 0x0f;
        }

        /* look up the optimization routine */
        switch ( dstClass )
        {

        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M16BF_SetDestMem16;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M16BF_FillOXADestMem16;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M16BF_NotDestFillMem16;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M16BF_InvertDestMem16;
            break;
        case 32:    //xAVGx       :

#ifdef  GLOBAL_ALPHA_SUPPORT
        
            if(text_trans)
            {
                optPtr = &M16BD_Text_Transparency;
            }
            else
            {
                optPtr = &M16BD_Transparency;
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
                dRect = *rListPtr;
                rListPtr++;

                /* do we need to worry about clipping */
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

                    if(dRect.Ymin >= dRect.Ymax)
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

                    if( dRect.Xmin >= dRect.Xmax)
                    {
                        continue;
                    }

#ifndef NO_REGION_CLIP
                
                    /* do we need to worry about region clipping? */
                    if(clipToRegionFlag != 0 )
                    {
                        /* yes, decide whether to copy top->bottom
                        or bottom->top */
                        FillDrawer = &M16BF_SolidFillRectM16;

                        if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                        {
                            break;
                        }
                        continue;
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                /* blit the rectangle */
                M16BF_SolidFillRectM16(fillRec);
            }
        }
    }

    nuResume(dstBmap);
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M16BF_MonoPatternFillMem16
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
VOID M16BF_MonoPatternFillMem16(blitRcd *fillRec )
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  = fillRec->blitCnt;
    rListPtr = (rect *) fillRec->blitList;
    dstBmap  = fillRec->blitDmap;
    pnColr16 = fillRec->blitFore;
    bkColr16 = fillRec->blitBack;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr16, lclpnColr16);
    COLOR_CONVERT(bkColr16, lclbkColr16);

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

    dstClass = fillRec->blitRop & 0x0f;

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* Memory non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M16BF_MonoFillDestMem16;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M16BF_MonoFillOXADestMem16;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M16BF_NotDestMonoFillMem16;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M16BF_InvertDestMem16;
        break;
    }

    /* modify the pen color as necessary */
    switch ( dstClass )
    {
    case 3:     /* zNANDz  */
    case 4:     /* zNREPz  */
    case 5:     /* zNORz   */
    case 6:     /* zNXORz  */
    case 13:    /* zNORNz  */
    case 15:    /* zNANDNz */
        /* this has the effect of notting the pen color
        for all operations during this call */
        bkColr16 = ~bkColr16;
        pnColr16 = ~pnColr16;
        lclbkColr16 = ~lclbkColr16;
        lclpnColr16 = ~lclpnColr16;

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
        bkColr16 = 0;
        pnColr16 = 0;
        lclbkColr16 = 0;
        lclpnColr16 = 0;
        break;

    case 12:    /* zSETz    */
    case 14:    /* zINVERTz */
        /* sets all source bits to 1 */
        bkColr16 = ~0;
        pnColr16 = ~0;
        lclbkColr16 = ~0;
        lclpnColr16 = ~0;
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
                    FillDrawer = &M16BF_SolidFillRectM16;

                    if( CLIP_Fill_Clip_Region(fillRec, &dRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M16BF_SolidFillRectM16(fillRec);
        }
    }
    nuResume(dstBmap);
}

/***************************************************************************
* FUNCTION
*
*    M16BF_MultiColorFillRectM16
*
* DESCRIPTION
*
*    Fills a multicolored rectangle in the memory.
*
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
VOID M16BF_MultiColorFillRectM16(blitRcd *blitRec)
{
    INT32  lclByteCnt, lclLineCnt;
    INT32  ptrnX, ptrnY;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */
    UINT16 *patWPtr;

#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lclLineCnt = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

#ifndef SMART_LCD
    dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;
#endif /* SMART_LCD */

    /* start of pattern alignment */
    ptrnY  = dRect.Ymin % patHeight;
    ptrnY  *= patWidthInBytes;

#ifndef SMART_LCD
    dstPtr16 = (UINT16 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;
#endif /* SMART_LCD */

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX = dRect.Xmin % patWidthInBytes;
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each byte in the row */
            *dstPtr = savePat->patData[ptrnX + ptrnY];
            dstPtr++;

            ptrnX++;

            *dstPtr = savePat->patData[ptrnX + ptrnY];
            dstPtr++;

            patWPtr = (UINT16 *) &savePat->patData[ptrnX + ptrnY];

#ifdef SMART_LCD
            /* Call the function to get the pixel on the rectangle. */
            SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
            pixelValue = *patWPtr;
            /* Call the function to set the pixel on the rectangle. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
            *dstPtr16++ = *patWPtr;
#endif /* SMART_LCD */ 
            ptrnX += 2;

            if( ptrnX >= patWidthInBytes )
            {
                ptrnX = 0;
            }
        }

#ifndef SMART_LCD
        /* advance to next row */
        dstPtr16 += dstNextRow;
#endif /* SMART_LCD */
        ptrnY += patWidthInBytes;

        if( ptrnY >= patLength )
        {
            ptrnY = 0;
        }
    }
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef USING_DIRECT_X
    /* Blit back buffer to screen */
    BlitBacktoFront(&scrnRect);
#endif
}

#endif /* FILL_PATTERNS_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M16BF_DrawSolidFillRectM16
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
VOID M16BF_SolidFillRectM16(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;

    dstPtr16 = (UINT16 *) (*(dstBmap->mapTable[0] + dRect.Ymin)) + dstBgnByte;

    /* blit the rectangle */
    optPtr();
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef USING_DIRECT_X
    /* Blit back buffer to screen */
    BlitBacktoFront(&scrnRect);
#endif
}

/***************************************************************************
* FUNCTION
*
*    M16BF_InvertDestMem16
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
VOID M16BF_InvertDestMem16(VOID)
{
#ifdef SMART_LCD
    UINT32 pixelValue;
#else
    UINT16 *lclDstPtr;
#endif /* SMART_LCD */
    INT32 lclLineCnt, lclByteCnt;

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr  = dstPtr16;
#endif /* !SMART_LCD */
    lclLineCnt = lineCntM1;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each byte in the row */
#ifdef SMART_LCD
            /* Call the function to get the pixel on the rectangle. */
            SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
            pixelValue = ~pixelValue;
            /* Call the function to set the pixel on the rectangle. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
            *lclDstPtr = ~(*lclDstPtr);
            lclDstPtr++;
#endif /* SMART_LCD */
        }
#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BF_SetDestMem16
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
VOID M16BF_SetDestMem16(VOID)
{
#ifdef LCD_OPTIMIZE_FILL
    R1 UINT8 *lclDstPtr;
    R1 INT32 lclLineCnt;
#else /* LCD_OPTIMIZE_FILL */
#ifndef SMART_LCD
    UINT16 *lclDstPtr;
    INT32 lclLineCnt;
#endif /* !SMART_LCD */
#endif /* LCD_OPTIMIZE_FILL */

#ifndef SMART_LCD
    register INT32 lclByteCnt;

#ifdef LCD_OPTIMIZE_FILL
    R1 UINT32  pnColr32    = (UINT32)( (((UINT32)lclpnColr16 & 0xFFFF) << 16) |
                             ((UINT32)lclpnColr16 & 0xFFFF));
    R1 UINT8   *lclpnColr8 = (UINT8*)&lclpnColr16;
#endif /* LCD_OPTIMIZE_FILL */

#endif /* SMART_LCD */

    /* set up local pointers */
#ifdef LCD_OPTIMIZE_FILL
    lclDstPtr  = (UINT8 *)dstPtr16;
#else
#ifndef SMART_LCD
    lclDstPtr  = dstPtr16;
#endif /* !SMART_LCD */
#endif

#ifndef SMART_LCD
    lclLineCnt = lineCntM1;
#endif /* !SMART_LCD */

#ifdef SMART_LCD
    /* Call the function for filling the rectangle. */
    SCREENI_Display_Device.display_fill_rect_hook(dRect.Xmin, dRect.Xmax, dRect.Ymin, dRect.Ymax, lclpnColr16);
#else /* SMART_LCD */
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

#ifdef LCD_OPTIMIZE_FILL

        lclByteCnt += 1;

        /* Total bytes will be double the number of pixels in 16BPP */
        lclByteCnt = lclByteCnt << 1;

        if(lclByteCnt > 2)
        {
            /* Make destination pointers 4 byte align
               to perform optimization. */
            switch((UINT32)lclDstPtr & 0x03)
            {
            case 1:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
                *((UINT8*)lclDstPtr++) = lclpnColr8[3];
                *((UINT8*)lclDstPtr++) = lclpnColr8[2];
                *((UINT8*)lclDstPtr++) = lclpnColr8[3];

                pnColr32 = ((pnColr32 << 8) | lclpnColr8[3] );
#else
                *((UINT8*)lclDstPtr++) = lclpnColr8[0];
                *((UINT8*)lclDstPtr++) = lclpnColr8[1];
                *((UINT8*)lclDstPtr++) = lclpnColr8[0];

                pnColr32 = ((pnColr32 << 8) | lclpnColr8[0] );
#endif
                lclByteCnt -= 3;

                break;

            case 2:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
                *((UINT8*)lclDstPtr++) = lclpnColr8[2];
                *((UINT8*)lclDstPtr++) = lclpnColr8[3];
#else
                *((UINT8*)lclDstPtr++) = lclpnColr8[0];
                *((UINT8*)lclDstPtr++) = lclpnColr8[1];
#endif
                lclByteCnt -= 2;
                break;

            case 3:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
                *((UINT8*)lclDstPtr++) = lclpnColr8[3];

                pnColr32 = ((pnColr32 << 8) | lclpnColr8[3] );
#else
                *((UINT8*)lclDstPtr++) = lclpnColr8[0];

                pnColr32 = ((pnColr32 << 8) | lclpnColr8[0] );
#endif
                lclByteCnt -= 1;
                break;

            default:
                break;
            }

            while( lclByteCnt >= 4 )
            {
                *((UINT32 *)lclDstPtr) = pnColr32;
                lclDstPtr += 4;
                lclByteCnt -= 4;
            }
        }

        switch(lclByteCnt)
        {
        case 1:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
            *((UINT8*)lclDstPtr++) = lclpnColr8[3];
#else
            *((UINT8*)lclDstPtr++) = lclpnColr8[1];
#endif
            break;

        case 2:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
            *((UINT8*)lclDstPtr++) = lclpnColr8[2];
            *((UINT8*)lclDstPtr++) = lclpnColr8[3];
#else
            *((UINT8*)lclDstPtr++) = lclpnColr8[0];
            *((UINT8*)lclDstPtr++) = lclpnColr8[1];
#endif
            break;

        case 3:
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
            *((UINT8*)lclDstPtr++) = lclpnColr8[3];
            *((UINT8*)lclDstPtr++) = lclpnColr8[2];
            *((UINT8*)lclDstPtr++) = lclpnColr8[3];
#else
            *((UINT8*)lclDstPtr++) = lclpnColr8[1];
            *((UINT8*)lclDstPtr++) = lclpnColr8[0];
            *((UINT8*)lclDstPtr++) = lclpnColr8[1];
#endif
            break;
        }

#else /* LCD_OPTIMIZE_FILL */

        while( lclByteCnt-- >= 0 )
        {
            /* for each byte in the row */
            *lclDstPtr = (UINT16)lclpnColr16;
            lclDstPtr++;
        }

#endif /* LCD_OPTIMIZE_FILL */

        /* advance to next row */
#ifdef      LCD_OPTIMIZE_FILL
        lclDstPtr += (dstNextRow<<1);
#else
        lclDstPtr += dstNextRow;
#endif
    }
#endif /* SMART_LCD */
}

/***************************************************************************
* FUNCTION
*
*    M16BF_NotDestFillMem16
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
VOID M16BF_NotDestFillMem16(VOID)
{
    /* invert the destination */
    M16BF_InvertDestMem16();

    /* fill this rectangle */
    M16BF_FillOXADestMem16();
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*   M16BF_NotDestMonoFillMem16
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
VOID M16BF_NotDestMonoFillMem16(VOID)
{
    /* invert the destination */
    M16BF_InvertDestMem16();

    /* fill this rectangle */
    M16BF_MonoFillOXADestMem16();
}

/***************************************************************************
* FUNCTION
*
*    M16BF_MonoFillDestMem16
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr if the source is 1 else bkColr16.
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
VOID M16BF_MonoFillDestMem16(VOID)
{
    UINT16 *lclDstPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt;

    /* set up local pointers */
    lclDstPtr  = dstPtr16;
    lclLineCnt = lineCntM1;

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX     = dRect.Xmin % patWidthInBytes;
        ptrnBt    = (128 >> (dRect.Xmin & 7));
        ptrnByte  = savePat->patData[ptrnX + ptrnY];
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each byte in the row */
            if( ptrnByte & ptrnBt )
            {
                /* source is 1 */
#ifdef SMART_LCD
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                *lclDstPtr = (UINT16)lclpnColr16;
#endif /* SMART_LCD */
            }
#ifndef SMART_LCD
            else
            {
                /* source is 0 */
                *lclDstPtr = (UINT16)lclbkColr16;
            }

            lclDstPtr++;
#endif /* SMART_LCD */

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

#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
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
*    M16BF_FillOXADestMem16
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
VOID M16BF_FillOXADestMem16(VOID)
{
#ifdef SMART_LCD
    UINT32 pixelValue;
#else
    UINT16 *lclDstPtr;
#endif /* SMART_LCD */
    INT32  lclLineCnt, lclByteCnt;
    INT32  logFnc;

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr  = dstPtr16;
#endif /* !SMART_LCD */
    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each byte in the row */
            switch ( logFnc )
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
#ifdef SMART_LCD
                /* Call the function to get the pixel on the rectangle. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue | lclpnColr16;
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr | lclpnColr16;
#endif /* SMART_LCD */
                break;

            case 2: /* "XOR" */
#ifdef SMART_LCD
                /* Call the function to get the pixel on the rectangle. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue ^ lclpnColr16;
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr ^ lclpnColr16;
#endif /* SMART_LCD */
                break;

            case 3: /* "AND" */
#ifdef SMART_LCD
                /* Call the function to get the pixel on the rectangle. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue & lclpnColr16;
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr & lclpnColr16;
#endif /* SMART_LCD */
            }
#ifndef SMART_LCD
            lclDstPtr++;
#endif /* SMART_LCD */
        }
#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
    }
}

#ifdef  FILL_PATTERNS_SUPPORT
/***************************************************************************
* FUNCTION
*
*    M16BF_MonoFillOXADestMem16
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
VOID M16BF_MonoFillOXADestMem16(VOID)
{
    UINT16 *lclDstPtr;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 ptrnX, ptrnY, ptrnByte, ptrnBt, ptrnColr;

    /* set up local pointers */
    lclDstPtr  = dstPtr16;
    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    /* start of pattern alignment */
    ptrnY = dRect.Ymin % patHeight;
    ptrnY *= patWidthInBytes;

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        ptrnX      = dRect.Xmin % patWidthInBytes;
        ptrnBt     = (128 >> (dRect.Xmin & 7));
        ptrnByte   = savePat->patData[ptrnX + ptrnY];
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each byte in the row */
            if( ptrnByte & ptrnBt)
            {
                ptrnColr = (UINT16)lclpnColr16;
            }
            else
            {
                ptrnColr = (UINT16)lclbkColr16;
            }

            switch ( logFnc )
            {
            case 0: /* handled elsewhere */
            case 1: /* "OR" */
#ifdef SMART_LCD
                /* Call the function to get the pixel on the rectangle. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue | ptrnColr;
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr | ptrnColr;
#endif /* SMART_LCD */
                break;

            case 2: /* "XOR" */
#ifdef SMART_LCD
                /* Call the function to get the pixel on the rectangle. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue ^ ptrnColr;
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr ^ ptrnColr;
#endif /* SMART_LCD */
                break;

            case 3: /* "AND" */
#ifdef SMART_LCD
                /* Call the function to get the pixel on the rectangle. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue & ptrnColr;
                /* Call the function to set the pixel on the rectangle. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr & ptrnColr;
#endif /* SMART_LCD */
            }

#ifndef SMART_LCD
            lclDstPtr++;
#endif /* SMART_LCD */
            ptrnBt = ptrnBt >> 1;

            if( ptrnBt == 0)
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
#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
        ptrnY += patWidthInBytes;

        if( ptrnY == patLength)
        {
            ptrnY = 0;
        }
    }
}

#endif /* FILL_PATTERNS_SUPPORT */

#endif /* #ifdef INCLUDE_16_BIT */

