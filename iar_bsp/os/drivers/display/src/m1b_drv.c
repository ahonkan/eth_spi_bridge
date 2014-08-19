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
*  m1b_drv.c
*
* DESCRIPTION
*
*  This file contains 1-bit memory primitive driver routines.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M1BD_BlitSelfToSelf1Bit
*  M1BD_BlitMonoToSelf1Bit
*  M1BD_DrawRectEntryBlit1M1Bit
*  M1BD_DrawRectEntryBlitM1Bit
*  M1BD_DrawRectEntry_SetPixel1Bit
*  M1BD_DrawRectEntry1_1Bit
*  M1BD_GetPixelFor1Bit
*  M1BD_InvertTrans1M1Bit
*  M1BD_OXADest1M1Bit
*  M1BD_OXADestM1Bit
*  M1BD_OXADestImgM1Bit
*  M1BD_NotDestSolidM1Bit
*  M1BD_NotDestSolid1M1Bit
*  M1BD_NotDestSolidImgM1Bit
*  M1BD_ReadImage1Bit
*  M1BD_RepDest1M1Bit
*  M1BD_RepDest1M1Bit
*  M1BD_RepDestImgM1Bit
*  M1BD_SetPixelFor1Bit
*  M1BD_SetTrans1M1Bit
*  M1BD_WriteImage1Bit
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

UINT32   lclpnColr, lclbkColr;

/***************************************************************************
* FUNCTION
*
*    M1BD_BlitMonoToSelf1Bit
*
* DESCRIPTION
*
*    A special case optimization for monochrome to 1-bit memory,
*    rectangular clipping with color translation.
*
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
VOID M1BD_BlitMonoToSelf1Bit(blitRcd *blitRec)
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt  =  blitRec->blitCnt;
    rListPtr =  (rect *) blitRec->blitList;
    srcBmap  =  blitRec->blitSmap;

    srcPixBytes = srcBmap->pixBytes;

    /* pause the source */
    M_PAUSE(srcBmap);

    /* background color */
    bkColr = blitRec->blitBack;

    /* foreground color */
    pnColr = blitRec->blitFore;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr, lclpnColr);
    COLOR_CONVERT(bkColr, lclbkColr);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x1f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing. */
    SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    HideCursor();
#endif

    /* pause the destination */
    M_PAUSE(dstBmap);

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* Memory non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
        optPtr = &M1BD_RepDest1M1Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M1BD_OXADest1M1Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M1BF_SetDestM1Bit;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M1BD_NotDestSolid1M1Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M1BF_InvertDestM1Bit;
        break;

    /* Memory transparent */
    case 16:    /* xREPx   :           !0src         */
    case 17:    /* xORx    :       !0src OR  dst     */
    case 25:    /* xORNx   :     !0src OR  (NOT dst) */
    case 28:    /* xSETx   :           1's           */
        optPtr = &M1BD_SetTrans1M1Bit;
        break;

    case 18:    /* xXORx   :       !0src XOR dst     */
    case 27:    /* xANDNx  :     !0src AND (NOT dst) */
    case 30:    /* xINVERTx:        (NOT dst)        */
        optPtr = &M1BD_InvertTrans1M1Bit;
        break;

    case 19:    /* xNANDx  :    (NOT !0src) AND dst  */
    case 20:    /* xNREPx  :        (NOT !0src)      */
    case 21:    /* xNORx   :    (NOT !0src) OR  dst  */
    case 22:    /* xNXORx  :    (NOT !0src) XOR dst  */
    case 23:    /* xANDx   :       !0src AND dst     */
    case 24:    /* xCLEARx :           0's           */
    case 26:    /* xNOPx   :           dst <NOP>     */
    case 29:    /* xNORNx  : (NOT !0src) OR  (NOT dst) */
    case 31:    /* xNANDNx : (NOT !0src) AND (NOT dst) */
        optPtr = &SCREENS_Nop;
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

    case 8: /* zCLEARz */
        /* set all source bits to 0 */
        bkColr = 0;
        pnColr = 0;
        lclbkColr = 0;
        lclpnColr = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* set all source bits to 1 */
        bkColr = ~0;
        pnColr = ~0;
        lclbkColr = ~0;
        lclpnColr = ~0;
        break;

    default:    /* all transparent cases */
        break;
    }

    dstWidth = dstBmap->pixBytes;

    /* determine number of bits per pixel */
    shfCnt      = 1;

    flipMask    = 3;
    shiftMask   = 0x80;
    firstOffset = 0x07;

    /* set up clipping */
    if( CLIP_Set_Up_Clip(blitRec, &clipR, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        while( --rectCnt >= 0 )
        {
            sRect = *rListPtr++;
            dRect = *rListPtr++;

            /* do we need to worry about clipping */
            if( clipToRectFlag != 0 )
            {
                /* yes, do trivial reject */
                if( dRect.Xmin < clipR.Xmin )
                {
                    /* Reset src & dst Xmin clip limit */
                    sRect.Xmin -= (dRect.Xmin - clipR.Xmin);
                    dRect.Xmin  = clipR.Xmin;
                }
                if( dRect.Ymin < clipR.Ymin )
                {
                    /* Reset src & dst Ymin clip limit */
                    sRect.Ymin -= (dRect.Ymin - clipR.Ymin);
                    dRect.Ymin  = clipR.Ymin;
                }
                if( dRect.Xmax > clipR.Xmax )
                {
                    /* Reset src & dst Xmax clip limit */
                    sRect.Xmax -= (dRect.Xmax - clipR.Xmax);
                    dRect.Xmax  = clipR.Xmax;
                }
                if( dRect.Ymax > clipR.Ymax )
                {
                    /* Reset src & dst Ymax clip limit */
                    sRect.Ymax -= (dRect.Ymax - clipR.Ymax);
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
                    if( CLIP_Check_YX_Band_Blit(&dRect, rListPtr, &clipR, &rectCnt) )
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
                    /* yes */
                    FillDrawer = &M1BD_DrawRectEntryBlit1M1Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0)
                    {
                        break;
                    }
                    continue;
                }

#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M1BD_DrawRectEntryBlit1M1Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M1BD_DrawRectEntryBlit1M1Bit
*
* DESCRIPTION
*
*    Blits the image to the memory
*
* INPUTS
*
*    blitRcd *  blitRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_DrawRectEntryBlit1M1Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1 = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = sRect.Xmin >> 3;
    dstBgnByte = (dRect.Xmin >> flipMask);
    byteCntM1 = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - (sRect.Xmax >> 3) + srcBgnByte;

    srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                + srcBgnByte;

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
*    M1BD_NotDestSolid1M1Bit
*
* DESCRIPTION
*
*    Handles M1Bit "not dst" by inverting the destination first
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
VOID M1BD_NotDestSolid1M1Bit(VOID)
{
    /* invert the destination */
    M1BF_InvertDestM1Bit();

    /* fill this rectangle */
    M1BD_OXADest1M1Bit();
}

/***************************************************************************
* FUNCTION
*
*    M1BD_RepDest1M1Bit
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
VOID M1BD_RepDest1M1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    UINT8 pxBit;
    UINT8 pxBitVal;
    UINT8 pxShift;
    UINT8 dstPxBit;

    pxShift    =  (dRect.Xmin & firstOffset);
    lclLineCnt = lineCntM1;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                            + dstBgnByte;
        pxBit = pxBitVal;
#ifdef BIG_ENDIAN_GRFX
        dstPxBit = (UINT8) (shiftMask >> pxShift);
#else
        dstPxBit = (UINT8) (0x01 << pxShift);
#endif

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
            /* for each UINT8 in the row */
            if( *srcPtr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclpnColr);
            }
#else
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8 in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclpnColr);
            }
#endif
            else
            {
                /* source is 0 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclbkColr);
            }

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x80;
            }
#else
            dstPxBit = (dstPxBit << shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x01;
            }
#endif

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                srcPtr++;
            }
        }

        /* advance to next row */
        dRect.Ymin++;
        srcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BD_SetTrans1M1Bit
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr if the source is 1 else it
*    does nothing.
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
VOID M1BD_SetTrans1M1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    UINT8 pxBit;
    UINT8 pxBitVal;
    UINT8 pxShift;
    UINT8 dstPxBit;

    pxShift =  (dRect.Xmin & firstOffset);
    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                        + dstBgnByte;
        pxBit = pxBitVal;
#ifdef BIG_ENDIAN_GRFX
        dstPxBit = (UINT8) (shiftMask >> pxShift);
#else
        dstPxBit = (UINT8) (0x01 << pxShift);
#endif

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
            /* for each UINT8 in the row */
            if( *srcPtr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclpnColr);
            }
#else
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8 in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclpnColr);
            }
#endif

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x80;
            }
#else
            dstPxBit = (dstPxBit << shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x01;
            }
#endif
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                srcPtr++;
            }
        }

        /* advance to next row */
        dRect.Ymin++;
        srcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BD_InvertTrans1M1Bit
*
* DESCRIPTION
*
*    Inverts the destination pixel data if the source is 1 else it does
*    nothing.
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
VOID M1BD_InvertTrans1M1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    UINT8 pxBit;
    UINT8 pxBitVal;
    UINT8 pxShift;
    UINT8 dstPxBit;

    pxShift =  (dRect.Xmin & firstOffset);
    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                            + dstBgnByte;
        pxBit = pxBitVal;
#ifdef BIG_ENDIAN_GRFX
        dstPxBit = (UINT8) (shiftMask >> pxShift);
#else
        dstPxBit = (UINT8) (0x01 << pxShift);
#endif

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
            /* for each UINT8 in the row */
            if( *srcPtr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr ^ dstPxBit);
            }
#else
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8 in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr ^ dstPxBit);
            }
#endif

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x80;
            }
#else
            dstPxBit = (dstPxBit << shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x01;
            }
#endif
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                srcPtr++;
            }
        }

        /* advance to next row */
        dRect.Ymin++;
        srcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BD_OXADest1M1Bit
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
VOID M1BD_OXADest1M1Bit(VOID)
{
    INT32 lclLineCnt, lclByteCnt;
    UINT8 pxBit;
    UINT8 pxBitVal;
    UINT8 pxShift;
    UINT8 dstPxBit;
    INT32 logFnc;

    pxShift =  (dRect.Xmin & firstOffset);

    /* only two lower bits needed */
    logFnc   = (dstClass & 3);
    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                            + dstBgnByte;
        pxBit = pxBitVal;
#ifdef BIG_ENDIAN_GRFX
        dstPxBit = (UINT8) (shiftMask >> pxShift);
#else
        dstPxBit = (UINT8) (0x01 << pxShift);
#endif

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
            if( *srcPtr & pxBit )
            {
#else
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            if( lclColr & pxBit )
            {
#endif
                /* source is 1 */
                switch (logFnc)
                {
                case 0: /* handled elsewhere */
                case 1: /* "OR" */
                    *dstPtr = *dstPtr | (dstPxBit & lclpnColr);
                    break;

                case 2: /* "XOR" */
                    *dstPtr = *dstPtr ^ (dstPxBit & lclpnColr);
                    break;

                case 3: /* "AND" */
                    *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & lclpnColr));
                }
            }
            else
            {
                /* source is 0 */
                switch (logFnc)
                {
                case 0: /* handled elsewhere */
                case 1: /* "OR" */
                    *dstPtr = *dstPtr | (dstPxBit & lclbkColr);
                    break;

                case 2: /* "XOR" */
                    *dstPtr = *dstPtr ^ (dstPxBit & lclbkColr);
                    break;

                case 3: /* "AND" */
                    *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & lclbkColr));
                }
            }

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x80;
            }
#else
            dstPxBit = (dstPxBit << shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;
                dstPxBit = 0x01;
            }
#endif
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                srcPtr++;
            }
        }

        /* advance to next row */
        dRect.Ymin++;
        srcPtr += srcNextRow;
    }
}


/***************************************************************************
* FUNCTION
*
*    M1BD_BlitSelfToSelf1Bit
*
* DESCRIPTION
*
*    A special case optimization for memory to memory,
*    rectangular clipping with no color translation.
*
*
* INPUTS
*
*    blitRcd *  blitRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
#ifndef NO_BLIT_SUPPORT
VOID M1BD_BlitSelfToSelf1Bit(blitRcd *blitRec)
{
    rect   clipR;
    rect   *rListPtr;
    INT32  blitMayOverlap = NU_FALSE;
    INT32  isLine         = NU_FALSE;
    INT16  done           = NU_FALSE;

    rectCnt  =  blitRec->blitCnt;
    rListPtr =  (rect *) blitRec->blitList;
    srcBmap  =  blitRec->blitSmap;
    srcPixBytes = srcBmap->pixBytes;

    /* pause the source */
    M_PAUSE(srcBmap);

    /* background color */
    bkColr = blitRec->blitBack;

    /* foreground color */
    pnColr = blitRec->blitFore;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr, lclpnColr);
    COLOR_CONVERT(bkColr, lclbkColr);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x0f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing. */
    SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    HideCursor();
#endif

    /* pause the destination */
    M_PAUSE(dstBmap);

    dstWidth = dstBmap->pixBytes;

    /* determine number of bits per pixel */
    shfCnt      = 1;
    flipMask    = 3;
    shiftMask   = 0x80;
    firstOffset = 0x07;

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* LCD non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
        optPtr = &M1BD_RepDestM1Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M1BD_OXADestM1Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M1BF_SetDestM1Bit;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M1BD_NotDestSolidM1Bit;
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
        pnColr = ~0;
        lclpnColr = ~0;
        break;

    case 0:     /* zREPz  */
    case 1:     /* zORz   */
    case 2:     /* zXORz  */
    case 7:     /* zANDz  */
    case 9:     /* zORNz  */
    case 10:    /* zNOPz  */
    case 11:    /* zANDNz */
        pnColr = 0;
        lclpnColr = 0;
        break;

    case 8: /* zCLEARz */
        /* set all source bits to 0 */
        pnColr = 0;
        lclpnColr = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* set all source bits to 1 */
        pnColr = ~0;
        lclpnColr = ~0;
    }

    /* set up clipping */
    if( CLIP_Set_Up_Clip(blitRec, &clipR, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        while( --rectCnt >= 0 )
        {
            sRect = *rListPtr++;
            dRect = *rListPtr++;

            /* do we need to worry about clipping */
            if( clipToRectFlag != 0 )
            {
                /* yes, do trivial reject */
                if( dRect.Xmin < clipR.Xmin )
                {
                    /* Reset src & dst Xmin clip limit */
                    sRect.Xmin -= (dRect.Xmin - clipR.Xmin);
                    dRect.Xmin  = clipR.Xmin;
                }
                if( dRect.Ymin < clipR.Ymin )
                {
                    /* Reset src & dst Ymin clip limit */
                    sRect.Ymin -= (dRect.Ymin - clipR.Ymin);
                    dRect.Ymin  = clipR.Ymin;
                }
                if( dRect.Xmax > clipR.Xmax )
                {
                    /* Reset src & dst Xmax clip limit */
                    sRect.Xmax -= (dRect.Xmax - clipR.Xmax);
                    dRect.Xmax  = clipR.Xmax;
                }
                if( dRect.Ymax > clipR.Ymax )
                {
                    /* Reset src & dst Ymax clip limit */
                    sRect.Ymax -= (dRect.Ymax - clipR.Ymax);
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
                    if( CLIP_Check_YX_Band_Blit(&dRect, rListPtr, &clipR, &rectCnt) )
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
                    /* yes */
                    FillDrawer = &M1BD_DrawRectEntryBlitM1Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0)
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M1BD_DrawRectEntryBlitM1Bit(blitRec);
        }
    }

    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M1BD_DrawRectEntryBlitM1Bit
*
* DESCRIPTION
*
*    Blits the rectangle to the memory
*
*
* INPUTS
*
*    blitRcd *   blitRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_DrawRectEntryBlitM1Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = (sRect.Xmin >> flipMask);
    dstBgnByte = (dRect.Xmin >> flipMask);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    dRect.Ymax--;
    sRect.Ymax--;

    /* blit the rectangle */
    optPtr();

#ifdef USING_DIRECT_X
    /* Blit back buffer to screen */
    BlitBacktoFront(&scrnRect);
#endif

}
#endif /* #ifndef NO_BLIT_SUPPORT */

#if ((!defined(NO_BLIT_SUPPORT)) || (!defined(NO_IMAGE_SUPPORT)))
/***************************************************************************
* FUNCTION
*
*    M1BD_NotDestSolidM1Bit
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
VOID M1BD_NotDestSolidM1Bit(VOID)
{
    /* invert the destination */
    M1BF_InvertDestM1Bit();

    /* fill this rectangle */
    M1BD_OXADestM1Bit();
}

/***************************************************************************
* FUNCTION
*
*    M1BD_RepDestM1Bit
*
* DESCRIPTION
*
*    Sets the destination pixel data with the source data.
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
VOID M1BD_RepDestM1Bit(VOID)
{
    INT32 lclByteCnt;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8 dstPxBit, srcPxBit, srcColr;

    if( (dstBmap == srcBmap)
        &&
          (
            (sRect.Ymin < dRect.Ymin)
           ||
            ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin) )
          )
      )
    {
        /* blit bottom to top, right to left */
        srcPxShift = (sRect.Xmax & 0x07);
        dstPxShift = (dRect.Xmax & 0x07);

        srcShf = srcPxShift - dstPxShift;

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymax))
                                + (sRect.Xmax >> 3);
            dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                + (dRect.Xmax >> 3);

            srcPxBit = (UINT8) (0x80 >> srcPxShift);
#ifndef BIG_ENDIAN_GRFX
            dstPxBit = (UINT8) (0x80 >> dstPxShift);
#else
            dstPxBit = (UINT8) (0x01 << dstPxShift);
#endif
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                srcColr = (*srcPtr ^ lclpnColr) & srcPxBit;
#else
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8 in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;
#endif

                srcColr = srcColr? 0xFF : 0x00;

                *dstPtr = (*dstPtr & ~dstPxBit) | (srcColr & dstPxBit);

#ifndef BIG_ENDIAN_GRFX
                dstPxBit = (dstPxBit << 1);
#else
                dstPxBit = (dstPxBit >> 1);
#endif
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    dstPtr--;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= 8 )
                        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                            /* for each UINT8 in the row */
                            srcPtr--;
                            *dstPtr = lclpnColr ^ *srcPtr;
#else
                            UINT8 lclColr;

                            /* for each UINT8 in the row */
                            srcPtr--;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            *dstPtr = lclpnColr ^ lclColr;
#endif

                            dstPtr--;
                            lclByteCnt -= 8;
                        }
                    }

                    /* do rest of data */
#ifndef BIG_ENDIAN_GRFX
                    dstPxBit = 0x01;
#else
                    dstPxBit = 0x80;
#endif
                }

                srcPxBit = (srcPxBit << 1);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    srcPtr--;
                    srcPxBit = 0x01;
                }
            }

            /* advance to next row */
            dRect.Ymax--;
            sRect.Ymax--;
        }
    }
    else
    {
        /* blit top to bottom, left to right */
        srcPxShift = (sRect.Xmin & 0x07);
        dstPxShift = (dRect.Xmin & 0x07);

        srcShf = srcPxShift - dstPxShift;

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                                + srcBgnByte;
            dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                + dstBgnByte;

            srcPxBit = (UINT8) (0x80 >> srcPxShift);
#ifndef BIG_ENDIAN_GRFX
            dstPxBit = (UINT8) (0x80 >> dstPxShift);
#else
            dstPxBit = (UINT8) (0x01 << dstPxShift);
#endif
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                srcColr = (*srcPtr ^ lclpnColr) & srcPxBit;
#else
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8 in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;
#endif

                srcColr = srcColr? 0xFF : 0x00;

                *dstPtr = (*dstPtr & ~dstPxBit) | (srcColr & dstPxBit);

#ifndef BIG_ENDIAN_GRFX
                dstPxBit = (dstPxBit >> 1);
#else
                dstPxBit = (dstPxBit << 1);
#endif
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    dstPtr++;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= 8 )
                        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                            /* for each UINT8 in the row */
                            srcPtr++;
                            *dstPtr = lclpnColr ^ *srcPtr;
#else
                            UINT8 lclColr;

                            /* for each UINT8 in the row */
                            srcPtr++;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            *dstPtr = lclpnColr ^ lclColr;
#endif

                            dstPtr++;
                            lclByteCnt -= 8;
                        }
                    }

                    /* do rest of data */
#ifndef BIG_ENDIAN_GRFX
                    dstPxBit = 0x80;
#else
                    dstPxBit = 0x01;
#endif
                }

                srcPxBit = (srcPxBit >> 0x01);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    srcPtr++;
                    srcPxBit = 0x80;
                }
            }

            /* advance to next row */
            dRect.Ymin++;
            sRect.Ymin++;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BD_OXADestM1Bit
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
VOID M1BD_OXADestM1Bit(VOID)
{
    INT32 lclByteCnt, lclShift;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8 dstPxBit, srcPxBit, srcColr;
    INT32 logFnc;

    if( (dstBmap == srcBmap)
        &&
          (
             (sRect.Ymin < dRect.Ymin)
           ||
             ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin) )
          )
      )
    {
        /* blit bottom to top, right to left */
        dstPxShift = (dRect.Xmax & firstOffset) * shfCnt;
        srcPxShift = (sRect.Xmax & firstOffset) * shfCnt;

        srcShf = srcPxShift - dstPxShift;
        if( srcShf < 0 )
        {
            srcShf = -srcShf;
        }

        lclShift = 8;
        logFnc = (dstClass & 3);    /* only two lower bits needed */

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                    + (dRect.Xmax >> 3);
            srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymax))
                                    + (sRect.Xmax >> 3);

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (UINT8) (shiftMask >> dstPxShift);
#else
            dstPxBit = (UINT8) (0x01 << dstPxShift);
#endif
            srcPxBit = (UINT8) (shiftMask >> srcPxShift);
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                srcColr = (*srcPtr ^ lclpnColr) & srcPxBit;
#else
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8 in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;
#endif

                if( dstPxBit < srcPxBit )
                {
                    srcColr = (srcColr >> srcShf);
                }
                if( dstPxBit > srcPxBit )
                {
                    srcColr = (srcColr << srcShf);
                }

                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
                    *dstPtr = *dstPtr | srcColr;
                    break;

                case 2: /* "XOR" */
                    *dstPtr = *dstPtr ^ srcColr;
                    break;

                case 3: /* "AND" */
                    *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & srcColr));
                    break;
                }

#ifdef BIG_ENDIAN_GRFX
                dstPxBit = (dstPxBit << shfCnt);
#else
                dstPxBit = (dstPxBit >> shfCnt);
#endif
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    dstPtr--;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= lclShift )
                        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                            /* for each UINT8 in the row */
                            srcPtr--;
                            srcColr = lclpnColr ^ *srcPtr;
#else
                            UINT8 lclColr;

                            /* for each UINT8 in the row */
                            srcPtr--;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            srcColr = lclpnColr ^ lclColr;
#endif
                            switch (logFnc)
                            {
                            case 0: /* can't happen */
                            case 1: /* "OR" */
                                *dstPtr = *dstPtr | srcColr;
                                break;

                            case 2: /* "XOR" */
                                *dstPtr = *dstPtr ^ srcColr;
                                break;

                            case 3: /* "AND" */
                                *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & srcColr));
                                break;
                            }

                            dstPtr--;
                            lclByteCnt -= lclShift;
                        }
                    }
                    /* do rest of data */
#ifdef BIG_ENDIAN_GRFX
                    dstPxBit = 0x01;
#else
                    dstPxBit = 0x80;
#endif
                }

                srcPxBit = (srcPxBit << shfCnt);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    srcPtr--;
                    srcPxBit = 0x01;
                }
            }
            /* advance to next row */
            dRect.Ymax--;
            sRect.Ymax--;
        }
    }
    else
    {
        /* blit top to bottom, left to right */
        dstPxShift = (dRect.Xmin & firstOffset);
        srcPxShift = (sRect.Xmin & firstOffset);

        srcShf = srcPxShift - dstPxShift;
        if( srcShf < 0 )
        {
            srcShf = -srcShf;
        }

        lclShift = 8;

        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                    + dstBgnByte;
            srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                                    + srcBgnByte;

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (UINT8) (shiftMask >> dstPxShift);
#else
            dstPxBit = (UINT8) (0x01 << dstPxShift);
#endif
            srcPxBit = (UINT8) (shiftMask >> srcPxShift);

            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                srcColr = (*srcPtr ^ lclpnColr) & srcPxBit;
#else
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr,lclColr);

                /* for each UINT8 in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;
#endif

                if( dstPxBit < srcPxBit )
                {
                    srcColr = (srcColr >> srcShf);
                }
                if( dstPxBit > srcPxBit )
                {
                    srcColr = (srcColr << srcShf);
                }

                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
                    *dstPtr = *dstPtr | srcColr;
                    break;

                case 2: /* "XOR" */
                    *dstPtr = *dstPtr ^ srcColr;
                    break;

                case 3: /* "AND" */
                    *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & srcColr));
                    break;
                }

#ifdef BIG_ENDIAN_GRFX
                dstPxBit = (dstPxBit >> shfCnt);
#else
                dstPxBit = (dstPxBit << shfCnt);
#endif
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    dstPtr++;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= lclShift )
                        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                            /* for each UINT8 in the row */
                            srcPtr++;
                            srcColr = lclpnColr ^ *srcPtr;
#else
                            UINT8 lclColr;

                            /* for each UINT8 in the row */
                            srcPtr++;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr,lclColr);

                            srcColr = lclpnColr ^ lclColr;
#endif
                            switch (logFnc)
                            {
                            case 0: /* can't happen */
                            case 1: /* "OR" */
                                *dstPtr = *dstPtr | srcColr;
                                break;

                            case 2: /* "XOR" */
                                *dstPtr = *dstPtr ^ srcColr;
                                break;

                            case 3: /* "AND" */
                                *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & srcColr));
                                break;
                            }

                            dstPtr++;
                            lclByteCnt -= lclShift;
                        }
                    }

                    /* do rest of data */
#ifdef BIG_ENDIAN_GRFX
                    dstPxBit = 0x80;
#else
                    dstPxBit = 0x01;
#endif
                }

                srcPxBit = (srcPxBit >> shfCnt);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8 */
                    srcPtr++;
                    srcPxBit = 0x80;
                }
            }

            /* advance to next row */
            dRect.Ymin++;
            sRect.Ymin++;
        }
    }
}

#endif  /* ((!defined(NO_BLIT_SUPPORT)) || (!defined(NO_IMAGE_SUPPORT))) */

/***************************************************************************
* FUNCTION
*
*    M1BD_GetPixelFor1Bit
*
* DESCRIPTION
*
*     Special case optimization for GetPixel, 1-bit per pixel memory source.
*
* INPUTS
*
*    glbX
*    gblY
*    getpBmap
*
* OUTPUTS
*
*    Returns status of 0 which means it is not on the bitmap returns the
*    the pixel value.
*
****************************************************************************/
INT32 M1BD_GetPixelFor1Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap)
{
    UINT8 *pixelPtr;
    INT32 status = NU_SUCCESS;
    INT16 done   = NU_FALSE;

    /* check if off bitmap */
    if(    (gblX < 0)
        || (gblY < 0)
        || (gblX >= getpBmap->pixWidth)
        || (gblY >= getpBmap->pixHeight)
      )
    {
        done = NU_TRUE;
    }

    if(!done)
    {
        /* lock grafMap */
        M_PAUSE(getpBmap);

        shfCnt = (gblX & 0x07);
        pixelPtr = (UINT8 *) (*(getpBmap->mapTable[0] + gblY)) + (gblX >> 3);
        status = (*pixelPtr >> shfCnt) & 0x01;

        /* Convert the pixel color back to source format. */
        COLOR_BACK_CONVERT(status, status);

        nuResume(getpBmap);
    }
    return(status);
}


/***************************************************************************
* FUNCTION
*
*    M1BD_SetPixelFor1Bit
*
* DESCRIPTION
*
*    A special case optimization for SetPixel,1-bit-per-pixel memory
*    destination, rectangular and/or region clipping with color translation.
*    Only patterns 0 and 1 are handled here, and only raster ops 0 and 16!
*    blitCnt is ignored; it's assumed to be 1.
*
* INPUTS
*
*    blitRcd *  setpRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_SetPixelFor1Bit(blitRcd *setpRec )
{
    point *drawPt;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    /* set up the rectangular/region clip info */
    if( CLIP_Set_Up_Clip(setpRec, &cRect, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if( !done)
    {
        /* set up pointer */
        drawPt = (point *) setpRec->blitList;

        dRect.Xmin = drawPt->X;
        dRect.Ymin = drawPt->Y;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

        /* Call the function for pre-processing. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

        /* lock grafMap */
        M_PAUSE(setpRec->blitDmap);

        /* do we need to worry about clipping at all*/
        if( clipToRectFlag == 0 )
        {
            /* no--valid coordinates are guaranteed at a higher level */
            /* draw the point */
            M1BD_DrawRectEntry_SetPixel1Bit(setpRec);
        }
        else
        {
            /* yes first check trivial reject */
            if( (dRect.Xmin >= cRect.Xmin) && (dRect.Ymin >= cRect.Ymin)
                &&
                (dRect.Xmin < cRect.Xmax) && (dRect.Ymin < cRect.Ymax)
              )
            {
                
#ifndef NO_REGION_CLIP
                
                /* it passes so far - do we need to worry about region clipping? */
                if( clipToRegionFlag == 0 )
                
#endif  /* NO_REGION_CLIP */
                
                {
                    /* no, draw the point */
                    M1BD_DrawRectEntry_SetPixel1Bit(setpRec);
                }
                
#ifndef NO_REGION_CLIP
                
                else
                {
                    /* yes, clip to the region and draw */
                    dRect.Xmax = dRect.Xmin + 1;
                    dRect.Ymax = dRect.Ymin + 1;
                    FillDrawer = &M1BD_DrawRectEntry_SetPixel1Bit;
                    CLIP_Fill_Clip_Region(setpRec, &dRect);
                }
                
#endif  /* NO_REGION_CLIP */
                
            }
        }
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        ShowCursor();
#endif
    }
    nuResume(setpRec->blitDmap);

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M1BD_DrawRectEntry_SetPixel1Bit
*
* DESCRIPTION
*
*    Blit the set pixel.
*
* INPUTS
*
*    blitRcd *  drwPRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_DrawRectEntry_SetPixel1Bit(blitRcd *drwPRec)
{
    grafMap *drwGmap;
    INT32  *rowTable;
    UINT8   *bytePtr;
    INT16   pColor;
    INT16   done = NU_FALSE;

    /* get grafMap */
    drwGmap = drwPRec->blitDmap;

    /* look up the starting row */
    rowTable = (INT32 *) drwGmap->mapTable[0] + dRect.Ymin;

    /* pattern 0? */
    if( drwPRec->blitPat == 0 )
    {
        /* yes, use background color */
        if( drwPRec->blitRop & 0x10 )
        {
            /* done if transparent */
            done = NU_TRUE;
        }

        if( !done )
        {
            pColor = (INT16) drwPRec->blitBack;
        }
    }
    else
    {
        /* no, use foreground color */
        pColor = (INT16) drwPRec->blitFore;
    }

    if( !done )
    {
        /* point to pixel */
        shfCnt = (dRect.Xmin & 0x07);
        flipMask = ~(0x80 >> shfCnt);

        pColor = (pColor << (7 - shfCnt));

        /* Convert the color to target format. */
        COLOR_CONVERT(pColor, pColor);

         bytePtr = (UINT8 *) ((*rowTable) + (dRect.Xmin >> 3));
        *bytePtr = (*bytePtr & flipMask) | pColor;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BD_WriteImage1Bit
*
* DESCRIPTION
*
*    a special case optimization for 1->1Bit WriteImage.
*
* INPUTS
*
*    blitRcd *  blitRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
#ifndef NO_IMAGE_SUPPORT
VOID M1BD_WriteImage1Bit(blitRcd *blitRec)
{
    rect *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    srcImag = (image *) blitRec->blitSmap;

    if( (srcImag->imWidth <= 0) || (srcImag->imHeight <= 0) )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        sRect.Ymin = 0;
        sRect.Xmin = srcImag->imAlign;
        srcPixBytes = srcImag->imBytes;
        rListPtr =  (rect *) blitRec->blitList;

        dstBmap =  blitRec->blitDmap;
        dstClass = blitRec->blitRop & 0x1f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

        /* Call the function for pre-processing. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

        M_PAUSE(dstBmap);

        /* background color */
        bkColr = blitRec->blitBack;

        /* foreground color */
        pnColr = blitRec->blitFore;

        /* Convert the color to target format. */
        COLOR_CONVERT(pnColr, lclpnColr);
        COLOR_CONVERT(bkColr, lclbkColr);

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
            optPtr = &M1BD_RepDest1M1Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M1BD_OXADest1M1Bit;
            break;

        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M1BF_SetDestM1Bit;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M1BD_NotDestSolid1M1Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M1BF_InvertDestM1Bit;
            break;

        /* Memory transparent */
        case 16:    /* xREPx   :           !0src         */
        case 17:    /* xORx    :       !0src OR  dst     */
        case 25:    /* xORNx   :     !0src OR  (NOT dst) */
        case 28:    /* xSETx   :           1's           */
            optPtr = &M1BD_SetTrans1M1Bit;
            break;

        case 18:    /* xXORx   :       !0src XOR dst     */
        case 27:    /* xANDNx  :     !0src AND (NOT dst) */
        case 30:    /* xINVERTx:        (NOT dst)        */
            optPtr = &M1BD_InvertTrans1M1Bit;
            break;

        case 19:    /* xNANDx  :    (NOT !0src) AND dst  */
        case 20:    /* xNREPx  :        (NOT !0src)      */
        case 21:    /* xNORx   :    (NOT !0src) OR  dst  */
        case 22:    /* xNXORx  :    (NOT !0src) XOR dst  */
        case 23:    /* xANDx   :       !0src AND dst     */
        case 24:    /* xCLEARx :           0's           */
        case 26:    /* xNOPx   :           dst <NOP>     */
        case 29:    /* xNORNx  : (NOT !0src) OR  (NOT dst) */
        case 31:    /* xNANDNx : (NOT !0src) AND (NOT dst) */
            optPtr = &SCREENS_Nop;
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
            /* this has the effect of noting the pen color for all operations during this call */
            pnColr = ~pnColr;
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
            /* set all source bits to 0 */
            pnColr = 0;
            lclpnColr = 0;
            break;

        case 12:    /* zSETz    */
        case 14:    /* zINVERTz */
            /* sets all source bits to 1 */
            pnColr = ~0;
            lclpnColr = ~0;
            break;

        default:    /* all transparent cases */
            break;
        }

        shfCnt      = 1;
        flipMask    = 3;
        shiftMask   = 0x80;
        firstOffset = 0x07;

        /* set up clipping */
        if( CLIP_Set_Up_Clip(blitRec, &cRect, blitMayOverlap, isLine) )
        {
            done = NU_TRUE;
        }

        if( !done )
        {
            dRect = *rListPtr;

            /* See if the destination is too wide  */
            if( (dRect.Xmax - dRect.Xmin) > srcImag->imWidth )
            {
                dRect.Xmax = dRect.Xmin + srcImag->imWidth;
            }

            /* sXmax not adjusted because not used  */

            /* See if the destination is too high  */
            if( (dRect.Ymax - dRect.Ymin) > srcImag->imHeight )
            {
                dRect.Ymax = dRect.Ymin + srcImag->imHeight;
            }

            /* sYmax not adjusted because not used  */

            /* do we need to worry about clipping */
            if( clipToRectFlag != 0 )
            {
                if( dRect.Xmin < cRect.Xmin )
                {
                    /* Reset src & dst Xmin clip limit */
                    sRect.Xmin -= (dRect.Xmin - cRect.Xmin);
                    dRect.Xmin  = cRect.Xmin;
                }
                if( dRect.Ymin < cRect.Ymin )
                {
                    /* Reset src & dst Ymin clip limit */
                    sRect.Ymin -= (dRect.Ymin - cRect.Ymin);
                    dRect.Ymin  = cRect.Ymin;
                }
                if( dRect.Xmax > cRect.Xmax )
                {
                    /* Reset dst Xmax clip limit */
                    dRect.Xmax  = cRect.Xmax;
                }
                if( dRect.Ymax > cRect.Ymax )
                {
                    /* Reset dst Ymax clip limit */
                    dRect.Ymax  = cRect.Ymax;
                }
                if( (dRect.Xmin >= dRect.Xmax) || (dRect.Ymin >= dRect.Ymax) )
                {
                    done = NU_TRUE;
                }
            }
            
#ifndef NO_REGION_CLIP
                
            if( !done )
            {
                /* do we need to worry about region clipping?  */
                if( clipToRegionFlag != 0 )
                {
                    /* yes, go do it  */
                    FillDrawer = &M1BD_DrawRectEntry1_1Bit;
                    CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                    done = NU_TRUE;
                }
            }
                
#endif  /* NO_REGION_CLIP */
                
        }

        if( !done )
        {
            M1BD_DrawRectEntry1_1Bit(blitRec);
        }

        nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        ShowCursor();
#endif
    }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M1BD_DrawRectEntry1_1Bit
*
* DESCRIPTION
*
*    Blits the image to the memory
*
* INPUTS
*
*    blitRcd *  blitRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_DrawRectEntry1_1Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = (dRect.Xmin >> flipMask);
    srcBgnByte = (sRect.Xmin >> 3);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - ((sRect.Xmin + byteCntM1 + 1) >> 3)
                        + srcBgnByte;
    srcPtr = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes)
                        + srcBgnByte);

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
*    M1BD_ReadImage1Bit
*
* DESCRIPTION
*
*    A special case optimization for ReadImage, 1 bit per pixel,
*    linear memory.
*
* INPUTS
*
*    grafMap * rdiBmap
*    image   * dstImage
*    INT32       gblYmax
*    INT32       gblXmax
*    INT32       gblYmin
*    INT32       gblXmin
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_ReadImage1Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                        INT32 gblYmin, INT32 gblXmin)
{
    /* # of bytes across an dstImage row  */
    INT32  dstRowBytes;

    /* bitmap limits  */
    INT32  cXmax,cYmax;
    INT16  grafErrValue, tmpYmin;
    INT32 *srcMapTable;
    UINT8  *dstPtrSav;
    INT32   count;
    INT32   done = NU_FALSE;

    gblXmax--;
    gblYmax--;

    M_PAUSE(rdiBmap);

    shfCnt      = 1;
    flipMask    = 3;
    firstOffset = 0x07;

    /* store mapTable[0] pointer locally  */
    srcMapTable = (INT32 *)rdiBmap->mapTable[0];

    /* get the source bitmap's limits  */
    cXmax = rdiBmap->pixWidth - 1;
    cYmax = rdiBmap->pixHeight - 1;

    /* Initialize dstImage header and destination copy parameters  */
    /* assume no span segment  */
    dstImage->imFlags  = 0;

    /* always plane # 1  */
    dstImage->imPlanes = 1;

    /* # bits per pixel  */
    dstImage->imBits   = rdiBmap->pixBits;
    dstImage->imHeight = gblYmax - gblYmin + 1;
    dstImage->imAlign  = gblXmin & firstOffset;
    dstImage->imWidth  = gblXmax - gblXmin + 1;
    dstImage->imBytes  = (dstImage->imWidth + dstImage->imAlign) >> flipMask;

    dstRowBytes = dstImage->imBytes;

    dstPtr = (UINT8 *)((INT32) &dstImage->imData[0]);

    if( (dstImage->imWidth < 0) || (dstImage->imHeight < 0) )
    {
        grafErrValue = c_ReadImag + c_NullRect;
        rdiBmap->cbPostErr(grafErrValue);

        dstImage->imWidth  = 0;
        dstImage->imHeight = 0;
        dstImage->imBytes  = 0;
        done = NU_TRUE;
    }
    if( !done &&  (dstImage->imWidth == 0) || (dstImage->imHeight == 0) )
    {
        dstImage->imWidth  = 0;
        dstImage->imHeight = 0;
        dstImage->imBytes  = 0;
        done = NU_TRUE;
    }

    if(!done)
    {
        /* Now clip to the source bitmap */
        /* check that gblXmin is not off bitmap  */
        if( gblXmin < 0 )
        {
            dstPtr -= (gblXmin + firstOffset) >> flipMask;
            gblXmin = 0;
        }

        /* Check that gblYmin is not off bitmap  */
        if( gblYmin < 0 )
        {
            dstPtr = (UINT8 *) ( ( (SIGNED) dstPtr) + (dstRowBytes * (-gblYmin) ) );
            gblYmin = 0;
        }

        /* Check that gblYmax is not off bitmap  */
        if( cYmax < gblYmax )
        {
            gblYmax = cYmax;
        }

        /* Check that gblXmax is not off bitmap */
        if( cXmax < gblXmax )
        {
            gblXmax = cXmax;
        }

        /* Now set the horizontal and vertical */
        byteCntM1 = (gblXmax - gblXmin + dstImage->imAlign) >> flipMask;
        if( byteCntM1 < 0 )
        {
            done = NU_TRUE;
        }

        if(!done)
        {
            lineCntM1 = (gblYmax - gblYmin);

            if( lineCntM1 < 0 )
            {
                done = NU_TRUE;
            }

            if(!done)
            {
                tmpYmin = gblYmin;

                while( lineCntM1-- >= 0 )
                {
                    /* point to row table entry for the source row  */
                    srcPtr = (UINT8 *) *(srcMapTable + tmpYmin) + (gblXmin >> flipMask);

                    /* save for later */
                    dstPtrSav = dstPtr;

                    for( count = 0; count <= byteCntM1; count++)
                    {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                        *dstPtr++ = *srcPtr++;
#else
                        UINT8 lclColr;

                        /* Convert the color to target format. */
                        COLOR_BACK_CONVERT(*srcPtr,lclColr);

                        *dstPtr++ = lclColr;
                        srcPtr++;
#endif
                    }

                    dstPtr = dstPtrSav + dstRowBytes;
                    tmpYmin++;
                }
            }
        }
    }
    nuResume(rdiBmap);
}

/***************************************************************************
* FUNCTION
*
*    M1BD_NotDestSolidImgM1Bit
*
* DESCRIPTION
*
*    Handles memory "not dst" by inverting the destination first.
*
* INPUTS
*
*   None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M1BD_NotDestSolidImgM1Bit(VOID)
{
    /* invert the destination */
    M1BF_InvertDestM1Bit();

    /* fill this rectangle */
    M1BD_OXADestImgM1Bit();
}

/***************************************************************************
* FUNCTION
*
*    M1BD_RepDestImgM1Bit
*
* DESCRIPTION
*
*    Sets the destination Image pixel data with the source data.
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
VOID M1BD_RepDestImgM1Bit(VOID)
{
    INT32 lclByteCnt, lclShift;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8 dstPxBit, srcPxBit, srcColr;

    dstPxShift = (dRect.Xmin & firstOffset);
    srcPxShift = (sRect.Xmin & firstOffset);

    srcShf = srcPxShift - dstPxShift;
    if( srcShf < 0 )
    {
        srcShf = -srcShf;
    }

    lclShift = 8;

    while( lineCntM1-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                + dstBgnByte;
        srcPtr = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes))
                                + srcBgnByte;
        lclByteCnt = byteCntM1;

#ifdef BIG_ENDIAN_GRFX
        dstPxBit = (UINT8) (shiftMask >> dstPxShift);
#else
        dstPxBit = (UINT8) (0x01 << dstPxShift);
#endif
        srcPxBit = (UINT8) (shiftMask >> srcPxShift);

        while( lclByteCnt-- >= 0 )
        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
            /* for each UINT8 in the row */
            srcColr = (*srcPtr ^ lclpnColr) & srcPxBit;
#else
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr,lclColr);

            /* for each UINT8 in the row */
            srcColr = (lclColr ^ lclpnColr) & srcPxBit;
#endif

            if( dstPxBit < srcPxBit )
            {
                srcColr = (srcColr >> srcShf);
            }
            if( dstPxBit > srcPxBit )
            {
                srcColr = (srcColr << srcShf);
            }

            *dstPtr = (*dstPtr & ~dstPxBit) | srcColr;

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (dstPxBit >> shfCnt);
#else
            dstPxBit = (dstPxBit << shfCnt);
#endif
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;

                /* now check for high speed middle blit */
                if( srcShf == 0 )
                {
                    /* yes */
                    while( lclByteCnt >= lclShift )
                    {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                        /* for each UINT8 in the row */
                        srcPtr++;
                        *dstPtr = lclpnColr ^ *srcPtr;
#else

                        UINT8 lclColr;

                        /* for each UINT8 in the row */
                        srcPtr++;

                        /* Convert the color to target format. */
                        COLOR_CONVERT(*srcPtr, lclColr);

                        *dstPtr = lclpnColr ^ lclColr;

#endif
                        dstPtr++;
                        lclByteCnt -= lclShift;
                    }
                }

                /* do rest of data */
#ifdef BIG_ENDIAN_GRFX
                dstPxBit = 0x80;
#else
                dstPxBit = 0x01;
#endif
            }

            srcPxBit = (srcPxBit >> shfCnt);
            if( srcPxBit == 0 )
            {
                /* advance to next UINT8 */
                srcPtr++;
                srcPxBit = 0x80;
            }
        }
        /* advance to next row */
        dRect.Ymin++;
        sRect.Ymin++;
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BD_OXADestImgM1Bit
*
* DESCRIPTION
*
*    Sets the destination Image pixel data based on the logical function "OR",
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
VOID M1BD_OXADestImgM1Bit(VOID)
{
    INT32 lclByteCnt, lclShift;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8 dstPxBit, srcPxBit, srcColr;
    INT32 logFnc;

    dstPxShift = (dRect.Xmin & firstOffset);
    srcPxShift = (sRect.Xmin & firstOffset);

    srcShf = srcPxShift - dstPxShift;
    if( srcShf < 0 )
    {
        srcShf = -srcShf;
    }

    lclShift = 8;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    while( lineCntM1-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                + dstBgnByte;
        srcPtr = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes))
                                + srcBgnByte;
        lclByteCnt = byteCntM1;

#ifdef BIG_ENDIAN_GRFX
        dstPxBit = (UINT8) (shiftMask >> dstPxShift);
#else
        dstPxBit = (UINT8) (0x01 << dstPxShift);
#endif
        srcPxBit = (UINT8) (shiftMask >> srcPxShift);
        while( lclByteCnt-- >= 0 )
        {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
            /* for each UINT8 in the row */
            srcColr = (*srcPtr ^ lclpnColr) & srcPxBit;
#else
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8 in the row */
            srcColr = (lclColr ^ lclpnColr) & srcPxBit;
#endif

            if( dstPxBit < srcPxBit )
            {
                srcColr = (srcColr >> srcShf);
            }
            if( dstPxBit > srcPxBit )
            {
                srcColr = (srcColr << srcShf);
            }

            switch (logFnc)
            {
            case 0: /* can't happen */
            case 1: /* "OR" */
                *dstPtr = *dstPtr | srcColr;
                break;

            case 2: /* "XOR" */
                *dstPtr = *dstPtr ^ srcColr;
                break;

            case 3: /* "AND" */
                *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & srcColr));
                break;
            }

#ifdef BIG_ENDIAN_GRFX
            dstPxBit = (dstPxBit >> shfCnt);
#else
            dstPxBit = (dstPxBit << shfCnt);
#endif
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8 */
                dstPtr++;

                /* now check for high speed middle blit */
                if( srcShf == 0 )
                {
                    /* yes */
                    while( lclByteCnt >= lclShift )
                    {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                        /* for each UINT8 in the row */
                        srcPtr++;

                        srcColr = lclpnColr ^ *srcPtr;
#else

                        UINT8 lclColr;

                        /* for each UINT8 in the row */
                        srcPtr++;

                        /* Convert the color to target format. */
                        COLOR_CONVERT(*srcPtr, lclColr);

                        srcColr = lclpnColr ^ lclColr;

#endif
                        switch (logFnc)
                        {
                        case 0: /* can't happen */
                        case 1: /* "OR" */
                            *dstPtr = *dstPtr | srcColr;
                            break;

                        case 2: /* "XOR" */
                            *dstPtr = *dstPtr ^ srcColr;
                            break;

                        case 3: /* "AND" */
                            *dstPtr = *dstPtr & (~dstPxBit | (dstPxBit & srcColr));
                            break;
                        }

                        dstPtr++;
                        lclByteCnt -= lclShift;
                    }
                }

                /* do rest of data */
#ifdef BIG_ENDIAN_GRFX
                dstPxBit = 0x80;
#else
                dstPxBit = 0x01;
#endif
            }

            srcPxBit = (srcPxBit >> shfCnt);

            if( srcPxBit == 0 )
            {
                /* advance to next UINT8 */
                srcPtr++;
                srcPxBit = 0x80;
            }
        }
        /* advance to next row */
        dRect.Ymin++;
        sRect.Ymin++;
    }
}

#endif /* #ifndef NO_IMAGE_SUPPORT */

#endif /* #ifdef INCLUDE_1_BIT */

