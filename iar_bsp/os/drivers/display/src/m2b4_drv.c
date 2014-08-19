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
*  m2b4_drv.c
*
* DESCRIPTION
*
*  This file contains 2/4-bit memory primitive driver routines.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M2B4D_BlitMonoToSelf2_4Bit
*  M2B4D_DrawRectEntryBlit1M2_4Bit
*  M2B4D_NotDestSolid1M2_4Bit
*  M2B4D_RepDest1M2_4Bit
*  M2B4D_SetTrans1M2_4Bit
*  M2B4D_InvertTrans1M2_4Bit
*  M2B4D_OXADest1M2_4Bit
*  M2B4D_BlitSelfToSelf2_4Bit
*  M2B4D_DrawRectEntryBlitM2_4MBit
*  M2B4D_NotDestSolidM2_4Bit
*  M2B4D_RepDestM2_4Bit
*  M2B4D_OXADestM2_4Bit
*  M2B4D_GetPixelFor2_4Bit
*  M2B4D_SetPixelFor2_4Bit
*  M2B4D_DrawRectEntry_SetPixel2_4Bit
*  M2B4D_WriteImage2_4Bit
*  M2B4D_DrawRectEntryImgM2_4Bit
*  M2B4D_WriteImage1M2_4Bit
*  M2B4D_DrawRectEntry1B2_4Bit
*  M2B4D_ReadImage2_4Bit
*  M2B4D_NotDestSolidImgM2_4Bit
*  M2B4D_RepDestImgM2_4Bit
*  M2B4D_OXADestImgM2_4Bit  
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

#ifdef INCLUDE_2_4_BIT

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

UINT32   lclpnColr, lclbkColr;

/***************************************************************************
* FUNCTION
*
*    M2B4D_BlitMonoToSelf2_4Bit
*
* DESCRIPTION
*
*    A special case optimization for monochrome to memory,
*    rectangular clipping with color translation.
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
VOID M2B4D_BlitMonoToSelf2_4Bit(blitRcd *blitRec)
{
    rect clipR;
    rect *rListPtr;
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
        optPtr = &M2B4D_RepDest1M2_4Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M2B4D_OXADest1M2_4Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M2B4F_SetDestM2_4Bit;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M2B4D_NotDestSolid1M2_4Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M2B4F_InvertDestM2_4Bit;
        break;

    /* Memory transparent */
    case 16:    /* xREPx   :           !0src         */
    case 17:    /* xORx    :       !0src OR  dst     */
    case 25:    /* xORNx   :     !0src OR  (NOT dst) */
    case 28:    /* xSETx   :           1's           */
        optPtr = &M2B4D_SetTrans1M2_4Bit;
        break;

    case 18:    /* xXORx   :       !0src XOR dst     */
    case 27:    /* xANDNx  :     !0src AND (NOT dst) */
    case 30:    /* xINVERTx:        (NOT dst)        */
        optPtr = &M2B4D_InvertTrans1M2_4Bit;
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
        /* set all source bits to 0 */
        bkColr = 0;
        pnColr = 0;
        lclbkColr = 0;
        lclpnColr = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* sets all source bits to 1 */
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
    shfCnt = dstBmap->pixBits;
    if( shfCnt == 2 )
    {
        /* 2 bpp */
        flipMask    = 2;
        shiftMask   = 0xc0;
        firstOffset = 0x03;

        pnColr = pnColr & 0x03;
        pnColr = pnColr | (pnColr << 2);
        pnColr = pnColr | (pnColr << 4);

        bkColr = bkColr & 0x03;
        bkColr = bkColr | (bkColr << 2);
        bkColr = bkColr | (bkColr << 4);
    }
    else
    {
        /* 4 bpp */
        flipMask    = 1;
        shiftMask   = 0xf0;
        firstOffset = 0x01;

        pnColr = pnColr & 0x0f;
        pnColr = pnColr | (pnColr << 4);

        bkColr = bkColr & 0x0f;
        bkColr = bkColr | (bkColr << 4);
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
                    FillDrawer = &M2B4D_DrawRectEntryBlit1M2_4Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M2B4D_DrawRectEntryBlit1M2_4Bit(blitRec);
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
*    M2B4D_DrawRectEntryBlit1M2_4Bit
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
VOID M2B4D_DrawRectEntryBlit1M2_4Bit(blitRcd *blitRec)
{
    lineCntM1 = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte  = sRect.Xmin >> 3;
    dstBgnByte  = (dRect.Xmin >> flipMask);
    byteCntM1 = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - (sRect.Xmax >> 3) + srcBgnByte;

    srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                + srcBgnByte;

    /* blit the rectangle */
    optPtr();
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_NotDestSolid1M2_4Bit
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
VOID M2B4D_NotDestSolid1M2_4Bit(VOID)
{
    /* invert the destination */
    M2B4F_InvertDestM2_4Bit();

    /* fill this rectangle */
    M2B4D_OXADest1M2_4Bit();
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_RepDest1M2_4Bit
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
VOID   M2B4D_RepDest1M2_4Bit(VOID )
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;
    INT32 pxShift;
    UINT8  dstPxBit;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                + dstBgnByte ;
        pxBit = pxBitVal;
        dstPxBit = (UINT8 ) (shiftMask >> pxShift);

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8  in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclpnColr);
            }
            else
            {
                /* source is 0 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclbkColr);
            }

            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8  */
                dstPtr++;

                if( shfCnt == 2 )
                {
                    dstPxBit = 0xc0;
                }
                else
                {
                    dstPxBit = 0xf0;
                }

            }

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

    return;
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_SetTrans1M2_4Bit
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
VOID  M2B4D_SetTrans1M2_4Bit(VOID )
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;
    INT32 pxShift;
    UINT8 dstPxBit;

    pxShift = (dRect.Xmin & firstOffset) * shfCnt;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
            + dstBgnByte;

        pxBit = pxBitVal;
        dstPxBit = (UINT8 ) (shiftMask >> pxShift);

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8  in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr & ~dstPxBit) | (dstPxBit & lclpnColr);
            }

            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8  */
                dstPtr++;

                if( shfCnt == 2 )
                {
                    dstPxBit = 0xc0;
                }
                else
                {
                    dstPxBit = 0xf0;
                }
            }

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
*    M2B4D_InvertTrans1M2_4Bit
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
VOID  M2B4D_InvertTrans1M2_4Bit(VOID )
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;
    INT32 pxShift;
    UINT8  dstPxBit;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin))
            + dstBgnByte;
        pxBit = pxBitVal;
        dstPxBit = (UINT8 ) (shiftMask >> pxShift);

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8  in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *dstPtr = (*dstPtr ^ dstPxBit);
            }

            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8  */
                dstPtr++;

                if( shfCnt == 2 )
                {
                    dstPxBit = 0xc0;
                }
                else
                {
                    dstPxBit = 0xf0;
                }
            }

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
*    M2B4D_OXADest1M2_4Bit
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
VOID  M2B4D_OXADest1M2_4Bit(VOID )
{
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;
    INT32 pxShift;
    UINT8  dstPxBit;
    INT32 logFnc;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);
    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin))
            + dstBgnByte;
        pxBit = pxBitVal;
        dstPxBit = (UINT8 ) (shiftMask >> pxShift);

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8  in the row */
            if( lclColr & pxBit )
            {
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

            dstPxBit = (dstPxBit >> shfCnt);
            if( dstPxBit == 0 )
            {
                /* advance to next UINT8  */
                dstPtr++;

                if( shfCnt == 2 )
                {
                    dstPxBit = 0xc0;
                }
                else
                {
                    dstPxBit = 0xf0;
                }
            }

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
*    M2B4D_BlitSelfToSelf2_4Bit
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
VOID  M2B4D_BlitSelfToSelf2_4Bit(blitRcd *blitRec)
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

    /* look up the optimization routine */
    switch (dstClass)
    {
    /* LCD non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
        optPtr = &M2B4D_RepDestM2_4Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M2B4D_OXADestM2_4Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M2B4F_SetDestM2_4Bit;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M2B4D_NotDestSolidM2_4Bit;
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
        /* sets all source bits to 1 */
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
                    FillDrawer = &M2B4D_DrawRectEntryBlitM2_4MBit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0)
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M2B4D_DrawRectEntryBlitM2_4MBit(blitRec);
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
*    M2B4D_DrawRectEntryBlitM2_4MBit
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
VOID  M2B4D_DrawRectEntryBlitM2_4MBit(blitRcd *blitRec)
{
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte  = (sRect.Xmin >> flipMask);
    dstBgnByte  = (dRect.Xmin >> flipMask);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    dRect.Ymax--;
    sRect.Ymax--;

    /* blit the rectangle */
    optPtr();

}

#endif /* #ifndef NO_BLIT_SUPPORT */

#if ((!defined(NO_BLIT_SUPPORT)) || (!defined(NO_IMAGE_SUPPORT)))
/***************************************************************************
* FUNCTION
*
*    M2B4D_NotDestSolidM2_4Bit
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
VOID M2B4D_NotDestSolidM2_4Bit(VOID)
{
    /* invert the destination */
    M2B4F_InvertDestM2_4Bit();

     /* fill this rectangle */
    M2B4D_OXADestM2_4Bit();
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_RepDestM2_4Bit
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
VOID  M2B4D_RepDestM2_4Bit(VOID )
{
    INT32 lclByteCnt, lclShift;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8  dstPxBit, srcPxBit, srcColr;

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

        lclShift = (1 << flipMask);

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                    + (dRect.Xmax >> flipMask);
            srcPtr = (UINT8  *) (*(srcBmap->mapTable[0] + sRect.Ymax))
                                    + (sRect.Xmax >> flipMask);

            dstPxBit = (UINT8 ) (shiftMask >> dstPxShift);
            srcPxBit = (UINT8 ) (shiftMask >> srcPxShift);

            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8  in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;

                if( dstPxBit < srcPxBit)
                {
                    srcColr = (srcColr >> srcShf);
                }
                if( dstPxBit > srcPxBit)
                {
                    srcColr = (srcColr << srcShf);
                }

                *dstPtr = (*dstPtr & ~dstPxBit) | srcColr;

                dstPxBit = (dstPxBit << shfCnt);
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    dstPtr--;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= lclShift )
                        {
                            /* for each UINT8  in the row */
                            srcPtr--;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            *dstPtr = lclpnColr ^ lclColr;
                            dstPtr--;
                            lclByteCnt -= lclShift;
                        }
                    }

                    /* do rest of data */
                    if( shfCnt == 2 )
                    {
                        dstPxBit = 0x03;
                    }
                    else
                    {
                        dstPxBit = 0x0f;
                    }
                }

                srcPxBit = (srcPxBit << shfCnt);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    srcPtr--;
                    if( shfCnt == 2 )
                    {
                        srcPxBit = 0x03;
                    }
                    else
                    {
                        srcPxBit = 0x0f;
                    }
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
        dstPxShift = (dRect.Xmin & firstOffset) * shfCnt;
        srcPxShift = (sRect.Xmin & firstOffset) * shfCnt;
        srcShf = srcPxShift - dstPxShift;

        if( srcShf < 0 )
        {
            srcShf = -srcShf;
        }

        lclShift = (1 << flipMask);

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                    + dstBgnByte;
            srcPtr = (UINT8  *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                                    + srcBgnByte;

            dstPxBit = (UINT8 ) (shiftMask >> dstPxShift);
            srcPxBit = (UINT8 ) (shiftMask >> srcPxShift);

            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8  in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;

                if( dstPxBit < srcPxBit )
                {
                    srcColr = (srcColr >> srcShf);
                }
                if( dstPxBit > srcPxBit )
                {
                    srcColr = (srcColr << srcShf);
                }

                *dstPtr = (*dstPtr & ~dstPxBit) | srcColr;

                dstPxBit = (dstPxBit >> shfCnt);
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    dstPtr++;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= lclShift )
                        {
                            /* for each UINT8  in the row */
                            srcPtr++;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            *dstPtr = lclpnColr ^ lclColr;
                            dstPtr++;
                            lclByteCnt -= lclShift;
                        }
                    }

                    /* do rest of data */
                    if( shfCnt == 2 )
                    {
                        dstPxBit = 0xc0;
                    }
                    else
                    {
                        dstPxBit = 0xf0;
                    }
                }

                srcPxBit = (srcPxBit >> shfCnt);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    srcPtr++;
                    if( shfCnt == 2 )
                    {
                        srcPxBit = 0xc0;
                    }
                    else
                    {
                        srcPxBit = 0xf0;
                    }
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
*    M2B4D_OXADestM2_4Bit
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
VOID M2B4D_OXADestM2_4Bit(VOID)
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

        lclShift = (1 << flipMask);
        logFnc = (dstClass & 3);    /* only two lower bits needed */

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                    + (dRect.Xmax >> flipMask);
            srcPtr = (UINT8  *) (*(srcBmap->mapTable[0] + sRect.Ymax))
                                    + (sRect.Xmax >> flipMask);

            dstPxBit = (UINT8 ) (shiftMask >> dstPxShift);
            srcPxBit = (UINT8 ) (shiftMask >> srcPxShift);

            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8  in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;

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

                dstPxBit = (dstPxBit << shfCnt);
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    dstPtr--;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= lclShift )
                        {
                            /* for each UINT8  in the row */
                            srcPtr--;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            srcColr = lclpnColr ^ lclColr;
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
                    if( shfCnt == 2 )
                    {
                        dstPxBit = 0x03;
                    }
                    else
                    {
                        dstPxBit = 0x0f;
                    }
                }

                srcPxBit = (srcPxBit << shfCnt);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    srcPtr--;
                    if( shfCnt == 2 )
                    {
                        srcPxBit = 0x03;
                    }
                    else
                    {
                        srcPxBit = 0x0f;
                    }
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
        dstPxShift = (dRect.Xmin & firstOffset) * shfCnt;
        srcPxShift = (sRect.Xmin & firstOffset) * shfCnt;
        srcShf = srcPxShift - dstPxShift;

        if( srcShf < 0 )
        {
            srcShf = -srcShf;
        }

        lclShift = (1 << flipMask);
        logFnc = (dstClass & 3);    /* only two lower bits needed */

        while( lineCntM1-- >= 0 )
        {
            /* for each row */
            dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                    + dstBgnByte;
            srcPtr = (UINT8  *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                                    + srcBgnByte;

            dstPxBit = (UINT8 ) (shiftMask >> dstPxShift);
            srcPxBit = (UINT8 ) (shiftMask >> srcPxShift);

            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*srcPtr, lclColr);

                /* for each UINT8  in the row */
                srcColr = (lclColr ^ lclpnColr) & srcPxBit;

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

                dstPxBit = (dstPxBit >> shfCnt);
                if( dstPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    dstPtr++;

                    /* now check for high speed middle blit */
                    if( srcShf == 0 )
                    {
                        /* yes */
                        while( lclByteCnt >= lclShift )
                        {
                            /* for each UINT8  in the row */
                            srcPtr++;

                            /* Convert the color to target format. */
                            COLOR_CONVERT(*srcPtr, lclColr);

                            srcColr = lclpnColr ^ lclColr;
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
                    if( shfCnt == 2 )
                    {
                        dstPxBit = 0xc0;
                    }
                    else
                    {
                        dstPxBit = 0xf0;
                    }
                }

                srcPxBit = (srcPxBit >> shfCnt);
                if( srcPxBit == 0 )
                {
                    /* advance to next UINT8  */
                    srcPtr++;

                    if( shfCnt == 2 )
                    {
                        srcPxBit = 0xc0;
                    }
                    else
                    {
                        srcPxBit = 0xf0;
                    }
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
*    M2B4D_GetPixelFor2_4Bit
*
* DESCRIPTION
*
*     Special case optimization for GetPixel, 2/4-bit per pixel memory source.
*
* INPUTS
*
*    INT32   glbX
*    INT32   gblY
*    grafMap getpBmap
*
* OUTPUTS
*
*    INT32 - Returns status of 0 which means it is not on the bitmap returns the
*            the pixel value.
*
****************************************************************************/
INT32 M2B4D_GetPixelFor2_4Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap)
{
    UINT8  *pixelPtr;
    INT32  status = NU_SUCCESS;
    INT16  done   = NU_FALSE;

    /* check if off bitmap */
    if(   (gblX < 0)
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

        /* determine number of bits per pixel */
        shfCnt = getpBmap->pixBits;
        if( shfCnt == 2 )
        {
            /* 2 bpp */
            pixelPtr = (UINT8 *) (*(getpBmap->mapTable[0] + gblY)) + (gblX >> 2);
            switch (gblX & 0x03)
            {
            case 0:
                status = (*pixelPtr >> 6) & 0x03;
                break;
            case 1:
                status = (*pixelPtr >> 4) & 0x03;
                break;
            case 2:
                status = (*pixelPtr >> 2) & 0x03;
                break;
            case 3:
                status = *pixelPtr & 0x03;
                break;
            }
        }
        else
        {
            /* 4 bpp */
            pixelPtr = (UINT8 *) (*(getpBmap->mapTable[0] + gblY)) + (gblX >> 1);

            if( gblX & 0x01 )
            {
                status = *pixelPtr & 0x0f;
            }
            else
            {
                status = (*pixelPtr >> 4) & 0x0f;
            }
        }
        nuResume(getpBmap);
    }

    /* Convert the color back to source format. */
    COLOR_BACK_CONVERT(status, status);

    return(status);
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_SetPixelFor2_4Bit
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
VOID  M2B4D_SetPixelFor2_4Bit(blitRcd *setpRec )
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

    if( !done )
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
            M2B4D_DrawRectEntry_SetPixel2_4Bit(setpRec);
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
                    M2B4D_DrawRectEntry_SetPixel2_4Bit(setpRec);
                }
                
#ifndef NO_REGION_CLIP
                
                else
                {
                    /* yes, clip to the region and draw */
                    dRect.Xmax = dRect.Xmin + 1;
                    dRect.Ymax = dRect.Ymin + 1;
                    FillDrawer = &M2B4D_DrawRectEntry_SetPixel2_4Bit;
                    CLIP_Fill_Clip_Region(setpRec, &dRect);
                }
                
#endif  /* NO_REGION_CLIP */
                
            }
        }
        nuResume(setpRec->blitDmap);

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

        /* Call the function for post-processing. */
        SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */
    }

}

/***************************************************************************
* FUNCTION
*
*    M2B4D_DrawRectEntry_SetPixel2_4Bit
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
VOID  M2B4D_DrawRectEntry_SetPixel2_4Bit(blitRcd *drwPRec)
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

    /* Convert the color. */
    COLOR_CONVERT(pColor, pColor);

    if( !done )
    {
        /* point to pixel */
        /* determine number of bits per pixel */
        shfCnt = drwGmap->pixBits;
        if( shfCnt == 2 )
        {
            /* 2 bpp */
            pColor = (pColor & 0x03);
            bytePtr = (UINT8 *) ((*rowTable) + (dRect.Xmin >> 2));

            switch( dRect.Xmin & 0x03 )
            {
            case 0:
                *bytePtr = (*bytePtr & 0x3f) | (pColor << 6);
                break;
            case 1:
                *bytePtr = (*bytePtr & 0xcf) | (pColor << 4);
                break;
            case 2:
                *bytePtr = (*bytePtr & 0xf3) | (pColor << 2);
                break;
            case 3:
                *bytePtr = (*bytePtr & 0xfc) | pColor;
                break;
            }
        }
        else
        {
            /* 4 bpp */
            pColor = (pColor & 0x0f);
            bytePtr = (UINT8  *) ((*rowTable) + (dRect.Xmin >> 1));

            if( dRect.Xmin & 0x01 )
            {
                *bytePtr = (*bytePtr & 0xf0) | pColor;
            }
            else
            {
                *bytePtr = (*bytePtr & 0x0f) | (pColor << 4);
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_WriteImage2_4Bit
*
* DESCRIPTION
*
*    A special case optimization for 2/4 Bit WriteImage, interleaved or linear.
*    Also passes through M2B4D_WriteImage1M2_4Bit.
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
VOID  M2B4D_WriteImage2_4Bit(blitRcd *blitRec)
{
    rect *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    srcImag = (image *) blitRec->blitSmap;

    /*is this a monochrome image? */
    if( srcImag->imBits == 1 )
    {
        /* Let the 1->2/4 bit handler handle */
        M2B4D_WriteImage1M2_4Bit(blitRec);
        done = 1;
    }

    if( !done )
    {
        if( (srcImag->imWidth <= 0) || (srcImag->imHeight <= 0) )
        {
            done = 1;
        }
        if( !done )
        {
            sRect.Ymin = 0;
            sRect.Xmin = srcImag->imAlign;
            srcPixBytes = srcImag->imBytes;
            rListPtr =  (rect *) blitRec->blitList;

            dstBmap =  blitRec->blitDmap;
            dstClass = blitRec->blitRop & 0x0f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

            /* Call the function for pre-processing. */
            SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

            /* pause the destination */
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

            /* look up the optimization routine */
            switch (dstClass)
            {
            /* Memory non-transparent */
            case 0:     /* zREPz   :           src           */
            case 4:     /* zNREPz  :        (NOT src)        */
                optPtr = &M2B4D_RepDestImgM2_4Bit;
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 2:     /* zXORz   :       src XOR dst       */
            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M2B4D_OXADestImgM2_4Bit;
                break;

            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                optPtr = &M2B4F_SetDestM2_4Bit;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M2B4D_NotDestSolidImgM2_4Bit;
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
            case 3:  /* zNANDz  */
            case 4:  /* zNREPz  */
            case 5:  /* zNORz   */
            case 6:  /* zNXORz  */
            case 13: /* zNORNz  */
            case 15: /* zNANDNz */
                /* this has the effect of notting the pen color for all operations during this call */
                pnColr = ~0;
                lclpnColr = ~0;
                break;

            case 0:  /* zREPz  */
            case 1:  /* zORz   */
            case 2:  /* zXORz  */
            case 7:  /* zANDz  */
            case 9:  /* zORNz  */
            case 10: /* zNOPz  */
            case 11: /* zANDNz */
                pnColr = 0;
                lclpnColr = 0;
                break;

            case 8:  /* zCLEARz */
                /* set all source bits to 0 */
                pnColr = 0;
                lclpnColr = 0;
                break;

            case 12: /* zSETz */
            case 14: /* zINVERTz */
                /* sets all source bits to 1 */
                pnColr = ~0;
                lclpnColr = ~0;
            }

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

                /* sXmax not adjusted because not used */

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
                        /* Reset src & dst Xmax clip limit */
                        dRect.Xmax  = cRect.Xmax;
                    }
                    if( dRect.Ymax > cRect.Ymax )
                    {
                        /* Reset src & dst Ymax clip limit */
                        dRect.Ymax  = cRect.Ymax;
                        /* sYmax not altered because not used */
                    }
                    if( (dRect.Xmin >= dRect.Xmax) || (dRect.Ymin >= dRect.Ymax) )
                    {
                        done = NU_TRUE;
                    }

#ifndef NO_REGION_CLIP
                
                    if( !done )
                    {
                        /* do we need to worry about region clipping?  */
                        if( clipToRegionFlag != 0 )
                        {
                            /* yes, go do it  */
                            FillDrawer = &M2B4D_DrawRectEntryImgM2_4Bit;
                            CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                            done = NU_TRUE;
                        }
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                if ( !done )
                {
                    /* Draw the image */
                    M2B4D_DrawRectEntryImgM2_4Bit(blitRec);
                }
            }
            nuResume(dstBmap);
        }
    }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M2B4D_DrawRectEntryImgM2_4Bit
*
* DESCRIPTION
*
*    Draw the Image
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
VOID  M2B4D_DrawRectEntryImgM2_4Bit(blitRcd *blitRec)
{
    lineCntM1  =  dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte  = (dRect.Xmin >> flipMask);
    byteCntM1  =  dRect.Xmax - dRect.Xmin - 1;
    srcBgnByte  = (sRect.Xmin >> flipMask);

    /* blit the rectangle */
    optPtr();
}


/***************************************************************************
* FUNCTION
*
*    M2B4D_WriteImage1M2_4Bit
*
* DESCRIPTION
*
*    a special case optimization for 1->2/4Bit WriteImage.
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
VOID  M2B4D_WriteImage1M2_4Bit(blitRcd *blitRec)
{
    rect  *rListPtr;
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
        rListPtr = (rect *) blitRec->blitList;

        dstBmap  = blitRec->blitDmap;
        dstClass = blitRec->blitRop & 0x1f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

        /* Call the function for pre-processing. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        HideCursor();
#endif

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
            optPtr = &M2B4D_RepDest1M2_4Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M2B4D_OXADest1M2_4Bit;
            break;

        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M2B4F_SetDestM2_4Bit;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M2B4D_NotDestSolid1M2_4Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M2B4F_InvertDestM2_4Bit;
            break;

        /* Memory transparent */
        case 16:    /* xREPx   :           !0src         */
        case 17:    /* xORx    :       !0src OR  dst     */
        case 25:    /* xORNx   :     !0src OR  (NOT dst) */
        case 28:    /* xSETx   :           1's           */
            optPtr = &M2B4D_SetTrans1M2_4Bit;
            break;

        case 18:    /* xXORx   :       !0src XOR dst     */
        case 27:    /* xANDNx  :     !0src AND (NOT dst) */
        case 30:    /* xINVERTx:        (NOT dst)        */
            optPtr = &M2B4D_InvertTrans1M2_4Bit;
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
            /* this has the effect of notting the pen color for all operations during this call */
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

        case 8:     /* zCLEARz */
            /* set all source bits to 0 */
            pnColr = 0;
            lclpnColr = 0;
            break;

        case 12:    /* zSETz */
        case 14:    /* zINVERTz */
            /* sets all source bits to 1 */
            pnColr = ~0;
            lclpnColr = ~0;
            break;

        default:    /* all transparent cases */
            break;
        }

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
            pnColr = pnColr | (pnColr << 2);
            pnColr = pnColr | (pnColr << 4);

            bkColr = bkColr & 0x03;
            bkColr = bkColr | (bkColr << 2);
            bkColr = bkColr | (bkColr << 4);
        }
        else
        {
            /* 4 bpp */
            flipMask    = 1;
            shiftMask   = 0xf0;
            firstOffset = 0x01;

            pnColr = pnColr & 0x0f;
            pnColr = pnColr | (pnColr << 4);

            bkColr = bkColr & 0x0f;
            bkColr = bkColr | (bkColr << 4);
        }

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

#ifndef NO_REGION_CLIP
                
                if( !done )
                {
                    /* do we need to worry about region clipping?  */
                    if( clipToRegionFlag != 0 )
                    {
                        /* yes, go do it  */
                        FillDrawer = &M2B4D_DrawRectEntry1B2_4Bit;
                        CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                        done = NU_TRUE;
                    }
                }
                
#endif  /* NO_REGION_CLIP */

                if( !done )
                {
                    /* Blit the image to the memory */
                    M2B4D_DrawRectEntry1B2_4Bit(blitRec);
                }
            }
            nuResume(dstBmap);
        }
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
*    M2B4D_DrawRectEntry1B2_4Bit
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
VOID M2B4D_DrawRectEntry1B2_4Bit(blitRcd *blitRec)
{
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte  = (dRect.Xmin >> flipMask);
    srcBgnByte  = (sRect.Xmin >> 3);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - ((sRect.Xmin + byteCntM1 + 1) >> 3)
                    + srcBgnByte;
    srcPtr = (UINT8  *) (srcImag->imData + (sRect.Ymin * srcPixBytes)
                    + srcBgnByte);

    /* blit the rectangle */
    optPtr();

}


/***************************************************************************
* FUNCTION
*
*    M2B4D_ReadImage2_4Bit
*
* DESCRIPTION
*
*    A special case optimization for ReadImage, 2/4 bit per pixel,
*    linear memory.
*
* INPUTS
*
*    grafMap * rdiBmap
*    image   * dstImage
*    INT32   gblYmax
*    INT32   gblXmax
*    INT32   gblYmin
*    INT32   gblXmin
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M2B4D_ReadImage2_4Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                           INT32 gblYmin, INT32 gblXmin)
{
    /* # of UINT8 s across an dstImage row  */
    INT32  dstRowBytes;

    /* bitmap limits  */
    INT32  cXmax,cYmax;
    INT16  grafErrValue, tmpYmin;
    INT32 *srcMapTable;
    UINT8  *dstPtrSav;
    INT32  count;
    INT16  done = NU_FALSE;

    gblXmax--;
    gblYmax--;

    M_PAUSE(rdiBmap);

    /* determine number of bits per pixel */
    shfCnt = rdiBmap->pixBits;
    if( shfCnt == 2)
    {
        /* 2 bpp */
        flipMask    = 2;
        firstOffset = 0x03;
    }
    else
    {
        /* 4 bpp */
        flipMask    = 1;
        firstOffset = 0x01;
    }

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

    dstPtr = (UINT8 *)((INT32 ) &dstImage->imData[0]);

    if( (dstImage->imWidth < 0) || (dstImage->imHeight < 0) )
    {
        grafErrValue = c_ReadImag + c_NullRect;
        rdiBmap->cbPostErr(grafErrValue);

        dstImage->imWidth  = 0;
        dstImage->imHeight = 0;
        dstImage->imBytes  = 0;
        done = NU_TRUE;
    }
    if( !done )
    {
        if( (dstImage->imWidth == 0) || (dstImage->imHeight == 0) )
        {
            dstImage->imWidth  = 0;
            dstImage->imHeight = 0;
            dstImage->imBytes  = 0;
            done = NU_TRUE;
        }

        if( !done )
        {
            /* Now clip to the source bitmap */
            /* check that gblXmin is not off bitmap  */
            if( gblXmin < 0)
            {
                dstPtr -= (gblXmin + firstOffset) >> flipMask;
                gblXmin = 0;
            }

            /* Check that gblYmin is not off bitmap  */
            if( gblYmin < 0 )
            {
                dstPtr = (UINT8 *) ( ((INT32 ) dstPtr) + (dstRowBytes * (-gblYmin)) );
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

            if( !done )
            {
                lineCntM1 = (gblYmax - gblYmin);

                if( lineCntM1 < 0 )
                {
                    done = NU_TRUE;
                }

                if( !done )
                {
                    tmpYmin = gblYmin;

                    while( lineCntM1-- >= 0 )
                    {
                        /* point to row table entry for the source row */
                        srcPtr = (UINT8 *) *(srcMapTable + tmpYmin) + (gblXmin >> flipMask);

                        dstPtrSav = dstPtr; /* save for later */

                        for( count = 0; count <= byteCntM1; count++)
                        {
                            UINT8 lclColr;

                            /* Convert the color to target format. */
                            COLOR_BACK_CONVERT(*srcPtr, lclColr);

                            *dstPtr++ = lclColr;
                            srcPtr++;
                        }

                        dstPtr = dstPtrSav + dstRowBytes;
                        tmpYmin++;
                    }
                }
            }
        }
    }
    nuResume(rdiBmap);
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_NotDestSolidImgM2_4Bit
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
VOID M2B4D_NotDestSolidImgM2_4Bit(VOID)
{
    /* invert the destination */
    M2B4F_InvertDestM2_4Bit();

    /* fill this rectangle */
    M2B4D_OXADestImgM2_4Bit();
}

/***************************************************************************
* FUNCTION
*
*    M2B4D_RepDestImgM2_4Bit
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
VOID  M2B4D_RepDestImgM2_4Bit(VOID )
{
    INT32 lclByteCnt, lclShift;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8 dstPxBit, srcPxBit, srcColr, highBitColr;

    dstPxShift = (dRect.Xmin & firstOffset) * shfCnt;
    srcPxShift = (sRect.Xmin & firstOffset) * shfCnt;

    srcShf = srcPxShift - dstPxShift;
    if( srcShf < 0 )
    {
        srcShf = -srcShf;
    }

    lclShift = (1 << flipMask);

    while( lineCntM1-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                        + dstBgnByte;
        srcPtr = (UINT8 *) ((srcImag->imData + (sRect.Ymin * srcPixBytes))
                        + srcBgnByte);

        lclByteCnt = byteCntM1;

        dstPxBit = (UINT8) (shiftMask >> dstPxShift);
        srcPxBit = (UINT8) (shiftMask >> srcPxShift);

        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8  in the row */
            srcColr = (lclColr ^ lclpnColr) & srcPxBit;

            if( dstPxBit < srcPxBit )
            {
                srcColr = (srcColr >> srcShf);
            }
            if( dstPxBit > srcPxBit )
            {
                srcColr = (srcColr << srcShf);
            }

            /* Check here to see if we only need to write 4 bits of data */
            /* Also check if we are going off of the screen */
            if ((lclByteCnt < 0) && 
			   (dRect.Xmax < MAX_SCREEN_WIDTH_X - 1))
            {
                /* Get the high bit colors so we don't overwrite them */
                highBitColr = (*dstPtr & 0xF0);

                /* Draw only the bottom 4 bits */
                *dstPtr = ((((*dstPtr & ~dstPxBit) | srcColr) & ~0xF0) | highBitColr);
            }
            else
            {
                /* Draw like normal */
                *dstPtr = ((*dstPtr & ~dstPxBit) | srcColr);
            }
            dstPxBit = (dstPxBit >> shfCnt);

            if( dstPxBit == 0 )
            {
                /* advance to next UINT8  */
                dstPtr++;

                /* now check for high speed middle blit */
                if( srcShf == 0 )
                {
                    /* yes */
                    while( lclByteCnt >= lclShift )
                    {
                        /* for each UINT8  in the row */
                        srcPtr++;

                        /* Convert the color to target format. */
                        COLOR_CONVERT(*srcPtr, lclColr);

                        *dstPtr = lclpnColr ^ lclColr;
                        dstPtr++;
                        lclByteCnt -= lclShift;
                    }
                }

                /* do rest of data */
                if( shfCnt == 2 )
                {
                    dstPxBit = 0xc0;
                }
                else
                {
                    dstPxBit = 0xf0;
                }
            }

            srcPxBit = (srcPxBit >> shfCnt);
            if( srcPxBit == 0 )
            {
                /* advance to next UINT8  */
                srcPtr++;
                if( shfCnt == 2 )
                {
                    srcPxBit = 0xc0;
                }
                else
                {
                    srcPxBit = 0xf0;
                }
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
*    M2B4D_OXADestImgM2_4Bit
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
VOID  M2B4D_OXADestImgM2_4Bit(VOID )
{
    INT32 lclByteCnt, lclShift;
    INT32 dstPxShift, srcPxShift, srcShf;
    UINT8 dstPxBit, srcPxBit, srcColr;
    INT32 logFnc;

    dstPxShift = (dRect.Xmin & firstOffset) * shfCnt;
    srcPxShift = (sRect.Xmin & firstOffset) * shfCnt;

    srcShf = srcPxShift - dstPxShift;
    if( srcShf < 0 )
    {
        srcShf = -srcShf;
    }

    lclShift = (1 << flipMask);

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    while( lineCntM1-- >= 0 )
    {
        /* for each row */
        dstPtr = (UINT8  *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                                + dstBgnByte;
        srcPtr = (UINT8  *) (srcImag->imData + (sRect.Ymin * srcPixBytes))
                                + srcBgnByte;
        lclByteCnt = byteCntM1;

        dstPxBit = (UINT8) (shiftMask >> dstPxShift);
        srcPxBit = (UINT8) (shiftMask >> srcPxShift);

        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*srcPtr, lclColr);

            /* for each UINT8  in the row */
            srcColr = (lclColr ^ lclpnColr) & srcPxBit;

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

            dstPxBit = (dstPxBit >> shfCnt);

            if( dstPxBit == 0 )
            {
                /* advance to next UINT8  */
                dstPtr++;

                /* now check for high speed middle blit */
                if( srcShf == 0 )
                {
                    /* yes */
                    while( lclByteCnt >= lclShift )
                    {
                        /* for each UINT8  in the row */
                        srcPtr++;

                        /* Convert the color to target format. */
                        COLOR_CONVERT(*srcPtr, lclColr);

                        srcColr = lclpnColr ^ lclColr;
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
                if( shfCnt == 2 )
                {
                    dstPxBit = 0xc0;
                }
                else
                {
                    dstPxBit = 0xf0;
                }
            }

            srcPxBit = (srcPxBit >> shfCnt);

            if( srcPxBit == 0 )
            {
                /* advance to next UINT8  */
                srcPtr++;

                if( shfCnt == 2 )
                {
                    srcPxBit = 0xc0;
                }
                else
                {
                    srcPxBit = 0xf0;
                }
            }
        }

        /* advance to next row */
        dRect.Ymin++;
        sRect.Ymin++;
    }

}

#endif /* #ifndef NO_IMAGE_SUPPORT */

#endif /* #ifdef INCLUDE_2_4_BIT */


