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
*  m24b_drv.c
*
* DESCRIPTION
*
*  This file contains 24-bit memory primitive driver routines.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*   M24BD_BlitMonoToSelf24Bit
*   M24BD_BlitSelfToSelf24Bit
*   M24BD_DrawRectEntryImg24Bit
*   M24BD_DrawRectEntryBlitMM24Bit
*   M24BD_DrawRectEntryImg1B24
*   M24BD_DrawRectEntryBlit1M24Bit
*   M24BD_DrawRectEntry_SetPixel24
*   M24BD_GetPixelFor24Bit
*   M24BD_InvertTrans1M24Bit
*   M24BD_NotDestSolid1M24Bit
*   M24BD_NotDestSolidM24Bit
*   M24BD_OXADest1M24Bit
*   M24BD_OXADestM24Bit
*   M24BD_ReadImage24Bit
*   M24BD_RepDestM24Bit
*   M24BD_RepDest1M24Bit
*   M24BD_SetPixelFor24Bit
*   M24BD_SetTrans1M24Bit
*   M24BD_WriteImage24Bit
*   M24BD_WriteImage1M24Bit
*
* DEPENDENCIES
*
*  nucleus.h
*  nu_kernel.h
*  nu_ui.h
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

UINT32 lclpnColr24, lclbkColr24;

/***************************************************************************
* FUNCTION
*
*    M24BD_BlitMonoToSelf24Bit
*
* DESCRIPTION
*
*    A special case optimization for monochrome to 24-bit memory,
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
VOID M24BD_BlitMonoToSelf24Bit(blitRcd *blitRec)
{
    rect   clipR;
    rect   *rListPtr;
    INT32  blitMayOverlap = NU_FALSE;
    INT32  isLine         = NU_FALSE;
    INT16  done           = NU_FALSE;

    rectCnt     =  blitRec->blitCnt;
    rListPtr    =  (rect *) blitRec->blitList;
    srcBmap     =  blitRec->blitSmap;
    srcPixBytes = srcBmap->pixBytes;

    /* pause the source */
    M_PAUSE(srcBmap);

    /* background color */
    bkColr24 = blitRec->blitBack;

    /* foreground color */
    pnColr24 = blitRec->blitFore;

    /* Convert the color to target format. */
    COLOR_CONVERT(bkColr24, lclbkColr24);
    COLOR_CONVERT(pnColr24, lclpnColr24);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x20;

    if (!dstClass)
    {
        dstClass = blitRec->blitRop & 0x1f;
    }

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the pre-process function. */
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
        optPtr = &M24BD_RepDest1M24Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M24BD_OXADest1M24Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M24BF_SetDestMem24;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M24BD_NotDestSolid1M24Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;
    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M24BF_InvertDestMem24;
        break;

    /* Memory transparent */
    case 16:    /* xREPx   :           !0src         */
    case 17:    /* xORx    :       !0src OR  dst     */
    case 25:    /* xORNx   :     !0src OR  (NOT dst) */
    case 28:    /* xSETx   :           1's           */
        optPtr = &M24BD_SetTrans1M24Bit;
        break;

    case 18:    /* xXORx   :       !0src XOR dst     */
    case 27:    /* xANDNx  :     !0src AND (NOT dst) */
    case 30:    /* xINVERTx:        (NOT dst)        */
        optPtr = &M24BD_InvertTrans1M24Bit;
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

    default:    /* all transparent cases */
        break;
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
                    FillDrawer = &M24BD_DrawRectEntryBlit1M24Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M24BD_DrawRectEntryBlit1M24Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the post-processing function. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */
}

/***************************************************************************
* FUNCTION
*
*    M24BD_DrawRectEntryBlit1M24Bit
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
VOID M24BD_DrawRectEntryBlit1M24Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = sRect.Xmin >> 3;
    dstBgnByte = 3 * dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - (sRect.Xmax >> 3) + srcBgnByte;
    dstNextRow = dstBmap->pixBytes - (3 * byteCntM1) - 3;

    dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                + dstBgnByte;
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
*    M24BD_NotDestSolid1M24Bit
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
VOID M24BD_NotDestSolid1M24Bit(VOID)
{
    /* invert the destination */
    M24BF_InvertDestMem24();

    /* fill this rectangle */
    M24BD_OXADest1M24Bit();
}

/***************************************************************************
* FUNCTION
*
*    M24BD_RepDest1M24Bit
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
VOID M24BD_RepDest1M24Bit(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr, bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    bkColrR = (UINT8)(lclbkColr24 >> 16);
    bkColrG = (UINT8)((lclbkColr24 >> 8) & 0xff);
    bkColrB = (UINT8)(lclbkColr24  & 0xff);

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrB;
            }
            else
            {
                if(set_back_color == NU_TRUE)
                {
                    /* source is 0 */
                    *lclDstPtr++ = bkColrR;
                    *lclDstPtr++ = bkColrG;
                    *lclDstPtr++ = bkColrB;
                }
                else
                {
                    lclDstPtr+=3;
                }
            }

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_SetTrans1M24Bit
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr24 if the source is 1 else it
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
VOID M24BD_SetTrans1M24Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    UINT8 pnColrR, pnColrG, pnColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;


    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrB;
            }
            else
            {
                lclDstPtr += 3;
            }

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_InvertTrans1M24Bit
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
VOID M24BD_InvertTrans1M24Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;
    UINT8 pixlVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                pixlVal = *lclDstPtr;
                *lclDstPtr++ = ~pixlVal;

                pixlVal = *lclDstPtr;
                *lclDstPtr++ = ~pixlVal;

                pixlVal = *lclDstPtr;
                *lclDstPtr++ = ~pixlVal;
            }
            else
            {
                lclDstPtr += 3;
            }

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_OXADest1M24Bit
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
VOID M24BD_OXADest1M24Bit(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr, bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    bkColrR = (UINT8)(lclbkColr24 >> 16);
    bkColrG = (UINT8)((lclbkColr24 >> 8) & 0xff);
    bkColrB = (UINT8)(lclbkColr24  & 0xff);

    /* only two lower bits needed */
    logFnc   = (dstClass & 3);
    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0)
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( !(*lclSrcPtr & pxBit) )
            {
                /* source is 0 */
                if( (logFnc == 3) || (*lclDstPtr == 0) )
                {
                    /* "AND" or 0,0 */
                    *lclDstPtr++ = bkColrR;
                    *lclDstPtr++ = bkColrG;
                    *lclDstPtr++ = bkColrB;
                }
                else
                {
                    /* "OR","XOR" with 0,1 */
                    *lclDstPtr++ = pnColrR;
                    *lclDstPtr++ = pnColrG;
                    *lclDstPtr++ = pnColrB;
                }
            }
            else
            {
                /* source is 1 */
                if( logFnc == 1 )
                {
                    /* "OR" with 1,X */
                    *lclDstPtr++ = pnColrR;
                    *lclDstPtr++ = pnColrG;
                    *lclDstPtr++ = pnColrB;
                }
                else
                {
                    if( logFnc == 3 )
                    {
                        /* "AND" */
                        if( *lclDstPtr )
                        {
                            /* "AND", with 1,1 */
                            *lclDstPtr++ = pnColrR;
                            *lclDstPtr++ = pnColrG;
                            *lclDstPtr++ = pnColrB;
                        }
                        else
                        {
                            /* "AND", with 1,0 */
                            *lclDstPtr++ = bkColrR;
                            *lclDstPtr++ = bkColrG;
                            *lclDstPtr++ = bkColrB;
                        }
                    }
                    else
                    {
                        /* "XOR" */
                        if( *lclDstPtr )
                        {
                            /* "XOR", with 1,1 */
                            *lclDstPtr++ = bkColrR;
                            *lclDstPtr++ = bkColrG;
                            *lclDstPtr++ = bkColrB;
                        }
                        else
                        {
                            /* "XOR", with 1,0 */
                            *lclDstPtr++ = pnColrR;
                            *lclDstPtr++ = pnColrG;
                            *lclDstPtr++ = pnColrB;
                        }
                    }
                }
            }

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_BlitSelfToSelf24Bit
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
VOID M24BD_BlitSelfToSelf24Bit(blitRcd *blitRec)
{
    rect  clipR;
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    rectCnt     =  blitRec->blitCnt;
    rListPtr    =  (rect *) blitRec->blitList;
    srcBmap     =  blitRec->blitSmap;
    srcPixBytes = srcBmap->pixBytes;

    /* pause the source */
    M_PAUSE(srcBmap);

    /* background color */
    bkColr24 = blitRec->blitBack;

    /* foreground color */
    pnColr24 = blitRec->blitFore;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr24, lclpnColr24);
    COLOR_CONVERT(bkColr24, lclbkColr24);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0xf0;

    if (!dstClass)
    {
        dstClass = blitRec->blitRop & 0x0f;
    }

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing the LCD buffer. */
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
    /* LCD non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
        optPtr = &M24BD_RepDestM24Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M24BD_OXADestM24Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M24BF_SetDestMem24;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M24BD_NotDestSolidM24Bit;
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
        pnColr24 = ~0;
        lclpnColr24 = ~0;
        break;

    case 0:     /* zREPz  */
    case 1:     /* zORz   */
    case 2:     /* zXORz  */
    case 7:     /* zANDz  */
    case 9:     /* zORNz  */
    case 10:    /* zNOPz  */
    case 11:    /* zANDNz */
        pnColr24 = 0;
        lclpnColr24 = 0;
        break;

    case 8:     /* zCLEARz */
        /* set all source bits to 0 */
        pnColr24 = 0;
        lclpnColr24 = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* sets all source bits to 1 */
        pnColr24 = ~0;
        lclpnColr24 = ~0;
        break;

    case 32:    /* xAVGx */
        
#ifdef  GLOBAL_ALPHA_SUPPORT
        
        /* set all source bits to 1 */
        pnColr24 = 0;
        lclpnColr24 = 0;

#endif  /* GLOBAL_ALPHA_SUPPORT */

        break;
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

                if( dRect.Xmin >= dRect.Xmax)
                {
                    continue;
                }

#ifndef NO_REGION_CLIP
                
                /* do we need to worry about region clipping? */
                if( clipToRegionFlag != 0 )
                {
                    /* yes */
                    FillDrawer = &M24BD_DrawRectEntryBlitMM24Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0)
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M24BD_DrawRectEntryBlitMM24Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the post-processing function. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */
}

/***************************************************************************
* FUNCTION
*
*    M24BD_DrawRectEntryBlitMM24Bit
*
* DESCRIPTION
*
*    Blits the rectangle to the memory
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
VOID M24BD_DrawRectEntryBlitMM24Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* Check if the destination bitmap points to user memory. */
    if (dstBmap->devMode == cMEMORY || dstBmap->devMode == cUSER)
    {
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = (BPP / 8) * sRect.Xmin;
        dstBgnByte = (BPP / 8) * dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = srcPixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8);
        dstNextRow = dstBmap->pixBytes - 
		            ((BPP / 8) * byteCntM1) - 
					(BPP / 8);
    }

    else
    {
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = (BPP / 8) * sRect.Xmin;
        dstBgnByte = 3 * dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = srcPixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8);
        dstNextRow = dstBmap->pixBytes - (3 * byteCntM1) - 3;
    }

    dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                + dstBgnByte;
    srcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                + srcBgnByte;

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
*    M24BD_NotDestSolidM24Bit
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
VOID M24BD_NotDestSolidM24Bit(VOID)
{
    /* invert the destination */
    M24BF_InvertDestMem24();

    /* fill this rectangle */
    M24BD_OXADestM24Bit();
}

/***************************************************************************
* FUNCTION
*
*    M16BD_RepDestM16Bit
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
VOID M24BD_RepDestM24Bit(VOID)
{
	/* Use register variables for improved performance */
    R1 UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    R1 UINT8 *lclSrcPtr;
    R1 INT32 lclLineCnt, lclByteCnt;

    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    if( (dstBmap == srcBmap)
        &&
        ( (sRect.Ymin < dRect.Ymin)
          ||
        ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin)))
      )
    {
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                + (3 * dRect.Xmax) + 2;
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                                + (3 * sRect.Xmax) + 2;

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                *lclDstPtr-- = pnColrB ^ *lclSrcPtr--;
                *lclDstPtr-- = pnColrG ^ *lclSrcPtr--;
                *lclDstPtr-- = pnColrR ^ *lclSrcPtr--;
#else
                UINT32 lclColor, lclSrcColr;

                lclSrcColr = (*lclSrcPtr) |
                             (*(lclSrcPtr - 1) << 8) |
                             (*(lclSrcPtr - 2) << 16);

                /* Convert the color. */
                COLOR_CONVERT(lclSrcColr, lclColor);

                /* for each UINT8 in the row */
                *lclDstPtr-- = pnColrB ^ ((lclColor) & 0xFF);
                *lclDstPtr-- = pnColrG ^ ((lclColor >> 8) & 0xFF);
                *lclDstPtr-- = pnColrR ^ ((lclColor >> 16) & 0xFF);
                lclSrcPtr-=3;
#endif
            }

            /* advance to next row */
            lclDstPtr -= dstNextRow;
            lclSrcPtr -= srcNextRow;
        }
    }
    else
    {
        /* blit top to bottom, left to right */
        /* set up local pointers */
        lclDstPtr = dstPtr;
        lclSrcPtr = srcPtr;
        lclLineCnt = lineCntM1;

        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;

#ifdef LCD_OPTIMIZE_BLIT

#if (COLOR_CONVERSION_NEEDED == NU_TRUE)

#error "Blit optimization does not work with color conversion."

#endif
            lclByteCnt = (lclByteCnt << 1) + lclByteCnt + 3;

			/* Check if optimization can be applied. */
            if(((UINT32)lclDstPtr & 0x03) == ((UINT32)lclSrcPtr & 0x03))
            {
                /* Make source and destination pointers 4 byte align 
                   to perform optimization. */
                switch((UINT32)lclDstPtr & 0x03)
                {
                case 1:
                    *lclDstPtr++ = *lclSrcPtr++;
                    *lclDstPtr++ = *lclSrcPtr++;
                    *lclDstPtr++ = *lclSrcPtr++;              
                    lclByteCnt -= 3;
              
                    break;

                case 2:
                    *lclDstPtr++ = *lclSrcPtr++;
                    *lclDstPtr++ = *lclSrcPtr++;
                    lclByteCnt -= 2;
                    break;

                case 3:
                    *lclDstPtr++ = *lclSrcPtr++;
                    lclByteCnt -= 1;
                    break;   

                default:
                    break;
                }

    			/* Copy 32 bytes from source to destination to speed things up */
                while( lclByteCnt > 3 )
                {
                  *((UINT32 *)lclDstPtr) = *((UINT32 *)lclSrcPtr);
                  lclDstPtr += 4;
                  lclSrcPtr += 4;
                  lclByteCnt -= 4;
                }

    			/* Copy remaining bytes from source to destination (these will be less than 4 bytes) */
    			while( lclByteCnt-- > 0 )
                {
                    /* for each UINT8 in the row */
                    *lclDstPtr++ = *lclSrcPtr++;
                }
            }
            else
            {
            	while (lclByteCnt > 0)
            	{
                    *lclDstPtr++ = *lclSrcPtr++;
                    *lclDstPtr++ = *lclSrcPtr++;
                    *lclDstPtr++ = *lclSrcPtr++;
                    lclByteCnt -= 3;
            	}
            }
#else /* LCD_OPTIMIZE_BLIT */

            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                *lclDstPtr++ = pnColrR ^ *lclSrcPtr++;
                *lclDstPtr++ = pnColrG ^ *lclSrcPtr++;
                *lclDstPtr++ = pnColrB ^ *lclSrcPtr++;
#else
                /* Check if the destination bitmap points to user memory. */
                if (dstBmap->devMode == cMEMORY || dstBmap->devMode == cUSER)
                {
                    /* for each UINT8 in the row */
                    *lclDstPtr++ = pnColrR ^ *lclSrcPtr++;
#if (BPP > 8)
                    *lclDstPtr++ = pnColrG ^ *lclSrcPtr++;
#endif
#if (BPP > 16)
                    *lclDstPtr++ = pnColrB ^ *lclSrcPtr++;
#endif
#if (BPP > 24)
                    *lclDstPtr++ = *lclSrcPtr++;
#endif
                }

                else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr)
#if (BPP > 8)
                    | (*(lclSrcPtr + 1) << 8)
#endif
#if (BPP > 16)
                    | (*(lclSrcPtr + 2) << 16)
#endif
#if (BPP > 24)
                    | (*(lclSrcPtr + 3) << 24)
#endif
                                 ;

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    /* for each UINT8 in the row */
                    *lclDstPtr++ = pnColrR ^ (lclColor & 0xFF);
                    *lclDstPtr++ = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    *lclDstPtr++ = pnColrB ^ ((lclColor >> 16) & 0xFF);

                    lclSrcPtr += (BPP/8);
                }
#endif
            }

#endif /* LCD_OPTIMIZE_BLIT */

            /* advance to next row */
            lclDstPtr += dstNextRow;
            lclSrcPtr += srcNextRow;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_OXADestM24Bit
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
VOID M24BD_OXADestM24Bit(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc, locColr;

    /* set up local colors */
    pnColrR = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrB = (UINT8)(lclpnColr24  & 0xff);

    if( (dstBmap == srcBmap)
        &&
        ( (sRect.Ymin < dRect.Ymin)
          ||
        ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin) ) )
      )
    {
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                + (3 * dRect.Xmax) + 2;
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                                + (3 * sRect.Xmax) + 2;

        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                /* for each UINT8 in the row */
                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    locColr = pnColrB ^ *lclSrcPtr--;
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrG ^ *lclSrcPtr--;
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrR ^ *lclSrcPtr--;
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr-- = locColr;
#else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr) |
                                 (*(lclSrcPtr - 1) << 8) |
                                 (*(lclSrcPtr - 2) << 16);

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    locColr = pnColrB ^ (lclColor & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrR ^ ((lclColor >> 16) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    lclSrcPtr -= 3;
                }
#endif

                    break;

                case 2: /* "XOR" */
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    locColr = pnColrB ^ *lclSrcPtr--;
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrG ^ *lclSrcPtr--;
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrR ^ *lclSrcPtr--;
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr-- = locColr;

#else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr) |
                                 (*(lclSrcPtr - 1) << 8) |
                                 (*(lclSrcPtr - 2) << 16);

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    locColr = pnColrB ^ (lclColor & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrR ^ ((lclColor >> 16) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    lclSrcPtr -= 3;
                }
#endif

                    break;

                case 3: /* "AND" */
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    locColr = pnColrB ^ *lclSrcPtr--;
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrG ^ *lclSrcPtr--;
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrR ^ *lclSrcPtr--;
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr-- = locColr;

#else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr) |
                                 (*(lclSrcPtr - 1) << 8) |
                                 (*(lclSrcPtr - 2) << 16);

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    locColr = pnColrB ^ (lclColor & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    locColr = pnColrR ^ ((lclColor >> 16) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr-- = locColr;

                    lclSrcPtr -= 3;
                }
#endif

                    break;
                }
            }
            /* advance to next row */
            lclDstPtr -= dstNextRow;
            lclSrcPtr -= srcNextRow;
        }
    }
    else
    {
        /* blit top to bottom, left to right */
        /* set up local pointers */
        lclDstPtr = dstPtr;
        lclSrcPtr = srcPtr;

        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                /* for each UINT8 in the row */
                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    locColr = pnColrR ^ *lclSrcPtr++;
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrG ^ *lclSrcPtr++;
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrB ^ *lclSrcPtr++;
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr++ = locColr;

#else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr) |
                                 (*(lclSrcPtr + 1) << 8) |
                                 (*(lclSrcPtr + 2) << 16);

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    locColr = pnColrR ^ (lclColor & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrB ^ ((lclColor >> 16) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    lclSrcPtr += 3;
                }
#endif
                    break;

                case 2: /* "XOR" */
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    locColr = pnColrR ^ *lclSrcPtr++;
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrG ^ *lclSrcPtr++;
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrB ^ *lclSrcPtr++;
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr++ = locColr;
#else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr) |
                                 (*(lclSrcPtr + 1) << 8) |
                                 (*(lclSrcPtr + 2) << 16);

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    locColr = pnColrR ^ (lclColor & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrB ^ ((lclColor >> 16) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    lclSrcPtr += 3;
                }
#endif

                    break;

                case 3: /* "AND" */
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    locColr = pnColrR ^ *lclSrcPtr++;
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrG ^ *lclSrcPtr++;
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrB ^ *lclSrcPtr++;
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr++ = locColr;


#else
                {
                    UINT32 lclColor, lclSrcColr;

                    lclSrcColr = (*lclSrcPtr) |
                                 (*(lclSrcPtr + 1) << 8) |
                                 (*(lclSrcPtr + 2) << 16);

                    /* Convert the color. */
                    COLOR_CONVERT(lclSrcColr, lclColor);

                    locColr = pnColrR ^ (lclColor & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrG ^ ((lclColor >> 8) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    locColr = pnColrB ^ ((lclColor >> 16) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr++ = locColr;

                    lclSrcPtr += 3;
                }
#endif

                    break;
                }
            }
            /* advance to next row */
            lclDstPtr += dstNextRow;
            lclSrcPtr += srcNextRow;
        }
    }
}

#endif  /* ((!defined(NO_BLIT_SUPPORT)) || (!defined(NO_IMAGE_SUPPORT))) */

/***************************************************************************
* FUNCTION
*
*    M24BD_GetPixelFor24Bit
*
* DESCRIPTION
*
*     Special case optimization for GetPixel, 24-bit per pixel memory source.
*
* INPUTS
*
*    INT32   glbX
*    INT32   gblY
*    grafMap *getpBmap
*
* OUTPUTS
*
*    INT32 - Returns status of 0 which means it is not on the bitmap returns the
*            the pixel value.
*
****************************************************************************/
INT32 M24BD_GetPixelFor24Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap)
{
    UINT8 *pixelPtr;
    INT32 pixelValue = -1;

    /* check if off bitmap */
    if(    (gblX < 0)
        || (gblY < 0)
        || (gblX >= getpBmap->pixWidth)
        || (gblY >= getpBmap->pixHeight)
      )
    {
        pixelValue = 0;
    }

    if(pixelValue != 0)
    {
        /* lock grafMap */
        M_PAUSE(getpBmap);

        if( (gblY < getpBmap->mapWinYmin[0]) || (gblY > getpBmap->mapWinYmax[0]) )
        {
            getpBmap->mapBankMgr(getpBmap, gblY, -1);
        }

        pixelPtr = (UINT8 *) (*(getpBmap->mapTable[0] + gblY)) + (3 * gblX);

        pixelValue  = (*pixelPtr++ << 16);
        pixelValue |= (*pixelPtr++ << 8);
        pixelValue |=  *pixelPtr;

        /* Convert the pixel color back to source format. */
        COLOR_BACK_CONVERT(pixelValue, pixelValue);

        nuResume(getpBmap);
    }

    return(pixelValue);
}

/***************************************************************************
* FUNCTION
*
*    M24BD_SetPixelFor24Bit
*
* DESCRIPTION
*
*    A special case optimization for SetPixel,24-bit-per-pixel memory
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
VOID M24BD_SetPixelFor24Bit(blitRcd *setpRec )
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

        /* Call the function for pre-processing task. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

        /* lock grafMap */
        M_PAUSE(setpRec->blitDmap);

        /* do we need to worry about clipping at all*/
        if( clipToRectFlag == 0 )
        {
            /* no--valid coordinates are guaranteed at a higher level */
            /* draw the point */
            M24BD_DrawRectEntry_SetPixel24(setpRec);
        }
        else
        {
            /* yes first check trivial reject */
            if( (dRect.Xmin >= cRect.Xmin)
                &&
                (dRect.Ymin >= cRect.Ymin)
                &&
                (dRect.Xmin < cRect.Xmax)
                &&
                (dRect.Ymin < cRect.Ymax)
              )
            {
                
#ifndef NO_REGION_CLIP
                
                /* it passes so far - do we need to worry about region clipping? */
                if( clipToRegionFlag == 0 )
                
#endif  /* NO_REGION_CLIP */
                
                {
                    /* no, draw the point */
                    M24BD_DrawRectEntry_SetPixel24(setpRec);
                }
                
#ifndef NO_REGION_CLIP
                
                else
                {
                    /* yes, clip to the region and draw */
                    dRect.Xmax = dRect.Xmin + 1;
                    dRect.Ymax = dRect.Ymin + 1;
                    FillDrawer = &M24BD_DrawRectEntry_SetPixel24;
                    CLIP_Fill_Clip_Region(setpRec, &dRect);
                }
                
#endif  /* NO_REGION_CLIP */
                
            }
        }
        nuResume(setpRec->blitDmap);

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

        /* Call the function for post-processing task. */
        SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_DrawRectEntry_SetPixel24
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
VOID M24BD_DrawRectEntry_SetPixel24(blitRcd *drwPRec)
{
    grafMap *drwGmap;
    long    *rowTable;
    UINT8   *bytePtr;
    UINT8   pColorR, pColorG, pColorB;
    INT16   done = NU_FALSE;

#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* get grafMap */
    drwGmap = drwPRec->blitDmap;

    if( (dRect.Ymin < drwGmap->mapWinYmin[0])    /* below window #0? */
        ||
        (dRect.Ymin > drwGmap->mapWinYmax[0]) )  /* above window #0? */
    {
        /* yes, map in bank */
        drwGmap->mapBankMgr(drwGmap, dRect.Ymin, -1, 0);
    }

    /* point to row table */
    rowTable = (long *) drwGmap->mapTable[0];

    /* look up the starting row */
    rowTable = rowTable + dRect.Ymin;

    /* point to pixel */
    bytePtr = (UINT8 *) ((*rowTable) + (3 * dRect.Xmin));

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
            UINT32 lclblitBack;

            /* Convert the color to target format. */
            COLOR_CONVERT(drwPRec->blitBack, lclblitBack);

            pColorR = (UINT8) (  lclblitBack >> 16 );
            pColorG = (UINT8) ( (lclblitBack >> 8) & 0xff );
            pColorB = (UINT8) (  lclblitBack & 0xff );
        }
    }
    else
    {
        UINT32 lclblitFore;

        /* Convert the color to target format. */
        COLOR_CONVERT(drwPRec->blitFore, lclblitFore);

        /* no, use foreground color */
        pColorR = (UINT8) (  lclblitFore >> 16 );
        pColorG = (UINT8) ( (lclblitFore >> 8) & 0xff );
        pColorB = (UINT8) (  lclblitFore & 0xff );
    }

    if( !done )
    {
        /* draw the pixel */
        *bytePtr++ = pColorR;
        *bytePtr++ = pColorG;
        *bytePtr   = pColorB;
    }

#ifdef USING_DIRECT_X
    BlitBacktoFront(&scrnRect);
#endif
}

/***************************************************************************
* FUNCTION
*
*    M24BD_WriteImage24Bit
*
* DESCRIPTION
*
*    A special case optimization for 24 Bit WriteImage, interleaved or linear.
*    Also passes through M24BD_WriteImage1M24Bit.
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
VOID M24BD_WriteImage24Bit(blitRcd *blitRec)
{
    rect  *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    srcImag = (image *) blitRec->blitSmap;
    srcBmap = 0;

    /*is this a monochrome image? */
    if( srcImag->imBits == 1 )
    {
        /* Let the 1->16 bit handler handle */
        M24BD_WriteImage1M24Bit(blitRec);
        done = NU_TRUE;
    }

    if( !done )
    {
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

            dstClass = blitRec->blitRop & 0xf0;

            if (!dstClass)
            {
                dstClass = blitRec->blitRop & 0x0f;
            }

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

            /* Call the function for pre-processing the LCD buffer. */
            SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

            M_PAUSE(dstBmap);

            /* look up the optimization routine */
            switch (dstClass)
            {
            /* Memory non-transparent */
            case 0:     /* zREPz   :           src           */
            case 4:     /* zNREPz  :        (NOT src)        */
                optPtr = &M24BD_RepDestM24Bit;
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 2:     /* zXORz   :       src XOR dst       */
            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M24BD_OXADestM24Bit;
                break;

            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                optPtr = &M24BF_SetDestMem24;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M24BD_NotDestSolidM24Bit;
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
                pnColr24 = ~0;
                lclpnColr24 = ~0;
                break;

            case 0:     /* zREPz  */
            case 1:     /* zORz   */
            case 2:     /* zXORz  */
            case 7:     /* zANDz  */
            case 9:     /* zORNz  */
            case 10:    /* zNOPz  */
            case 11:    /* zANDNz */
                pnColr24 = 0;
                lclpnColr24 = 0;
                break;

            case 8: /* zCLEARz */
                /* set all source bits to 0 */
                pnColr24 = 0;
                lclpnColr24 = 0;
                break;

            case 12:    /* zSETz */
            case 14:    /* zINVERTz */
                /* sets all source bits to 1 */
                pnColr24 = ~0;
                lclpnColr24 = ~0;
                break;

            case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
                /* set all source bits to 1 */
                pnColr24 = 0;
                lclpnColr24 = 0;

#endif  /* GLOBAL_ALPHA_SUPPORT */
                
                break;
            }

            /* set up clipping */
            if( CLIP_Set_Up_Clip(blitRec, &cRect, blitMayOverlap, isLine) )
            {
                done = NU_TRUE;
            }

            if( !done )
            {
                dRect = *rListPtr;

                /* See if the destination is too wide */
                if( (dRect.Xmax - dRect.Xmin) > srcImag->imWidth )
                {
                    dRect.Xmax = dRect.Xmin + srcImag->imWidth;
                }

                /* See if the destination is too high */
                if( (dRect.Ymax - dRect.Ymin) > srcImag->imHeight )
                {
                    dRect.Ymax = dRect.Ymin + srcImag->imHeight;
                }

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
                    }
                    if( (dRect.Xmin >= dRect.Xmax) || (dRect.Ymin >= dRect.Ymax) )
                    {
                        done = NU_TRUE;
                    }

#ifndef NO_REGION_CLIP
                
                    /* do we need to worry about region clipping?  */
                    if( !done )
                    {
                        if( clipToRegionFlag != 0 )
                        {
                            /* yes, go do it  */
                            FillDrawer = &M24BD_DrawRectEntryImg24Bit;
                            CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                            done = NU_TRUE;
                        }
                    }
                
#endif  /* NO_REGION_CLIP */
                
                    if ( !done )
                    {
                        /* Draw the image */
                        M24BD_DrawRectEntryImg24Bit(blitRec);
                    }
                }
                nuResume(dstBmap);
            }
        }
    }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Start image converter (IC) post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M24BD_DrawRectEntryImg24Bit
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
VOID M24BD_DrawRectEntryImg24Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* Check if the destination bitmap points to user memory. */
    if (dstBmap->devMode == cMEMORY || dstBmap->devMode == cUSER)
    {
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = (BPP / 8) * sRect.Xmin;
        dstBgnByte = (BPP / 8) * dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = srcPixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8);
        dstNextRow = dstBmap->pixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8);
    }

    else
    {
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = 3 * sRect.Xmin;
        dstBgnByte = 3 * dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = srcPixBytes - (3 * byteCntM1) - 3;
        dstNextRow = dstBmap->pixBytes - (3 * byteCntM1) - 3;
    }

    srcPtr = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes))
                + srcBgnByte;
    dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                + dstBgnByte;

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
*    M24BD_WriteImage1M24Bit
*
* DESCRIPTION
*
*    A special case optimization for 1->24Bit WriteImage.
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
VOID M24BD_WriteImage1M24Bit(blitRcd *blitRec)
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
        sRect.Xmin = 0;

        srcPixBytes = srcImag->imBytes;
        rListPtr =  (rect *) blitRec->blitList;

        dstBmap =  blitRec->blitDmap;
        dstClass = blitRec->blitRop & 0x20;

        if (!dstClass)
        {
            dstClass = blitRec->blitRop & 0x1f;
        }

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

        /* Call the function for pre-processing. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

        M_PAUSE(dstBmap);

        /* background color */
        bkColr24 = blitRec->blitBack;

        /* foreground color */
        pnColr24 = blitRec->blitFore;

        /* Convert the color to target format. */
        COLOR_CONVERT(bkColr24, lclbkColr24);
        COLOR_CONVERT(pnColr24, lclpnColr24);

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
            optPtr = &M24BD_RepDest1M24Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M24BD_OXADest1M24Bit;
            break;

        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M24BF_SetDestMem24;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M24BD_NotDestSolid1M24Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M24BF_InvertDestMem24;
            break;

        /* Memory transparent */
        case 16:    /* xREPx   :           !0src         */
        case 17:    /* xORx    :       !0src OR  dst     */
        case 25:    /* xORNx   :     !0src OR  (NOT dst) */
        case 28:    /* xSETx   :           1's           */
            optPtr = &M24BD_SetTrans1M24Bit;
            break;

        case 18:    /* xXORx   :       !0src XOR dst     */
        case 27:    /* xANDNx  :     !0src AND (NOT dst) */
        case 30:    /* xINVERTx:        (NOT dst)        */
            optPtr = &M24BD_InvertTrans1M24Bit;
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
        case 32:    //xAVGx    :

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
            pnColr24 = ~pnColr24;
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
            pnColr24 = 0;
            lclpnColr24 = 0;
            break;
        case 12:    /* zSETz */
        case 14:    /* zINVERTz */
            /* sets all source bits to 1 */
            pnColr24 = ~0;
            lclpnColr24 = ~0;
            break;

        case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
            /* set all source bits to 1 */
            pnColr24 = 0;
            lclpnColr24 = 0;

#endif  /* GLOBAL_ALPHA_SUPPORT */

            break;

        default:    /* all transparent cases */
            break;
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
            if( (dRect.Xmax - dRect.Xmin) > srcImag->imWidth)
            {
                dRect.Xmax = dRect.Xmin + srcImag->imWidth;
            }

            /* See if the destination is too high  */
            if( (dRect.Ymax - dRect.Ymin) > srcImag->imHeight )
            {
                dRect.Ymax = dRect.Ymin + srcImag->imHeight;
            }

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
                        FillDrawer = &M24BD_DrawRectEntryImg1B24;
                        CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                        done = NU_TRUE;
                    }
                }
                
#endif  /* NO_REGION_CLIP */
                
                if( !done )
                {
                    /* Blit the image to the memory */
                    M24BD_DrawRectEntryImg1B24(blitRec);
                }
            }
        }
        nuResume(dstBmap);
    }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing the LCD buffer. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M24BD_DrawRectEntryImg1B24
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
VOID M24BD_DrawRectEntryImg1B24(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = 3 * dRect.Xmin;
    srcBgnByte = (sRect.Xmin >> 3);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = dstBmap->pixBytes - (3 * byteCntM1) - 3;

    srcNextRow = srcPixBytes - ((sRect.Xmin + byteCntM1 + 1) >> 3)
                    + srcBgnByte;
    srcPtr = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes)
                    + srcBgnByte);

    dstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                    + dstBgnByte;

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
*    M24BD_ReadImage24Bit
*
* DESCRIPTION
*
*    A special case optimization for ReadImage, 24 bit per pixel,
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
VOID M24BD_ReadImage24Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                          INT32 gblYmin, INT32 gblXmin)
{
    /* # of bytes across a source bitmap row  */
    INT32   srcRowBytes;

    /* # of bytes across an dstImage row  */
    INT32   dstRowBytes;

    /* bitmap limits  */
    INT32 cXmax,cYmax;
    long  *srcMapTable;
    INT32 count;
    INT32 done = NU_FALSE;

    gblXmax--;
    gblYmax--;

    M_PAUSE(rdiBmap);

    /* # of UINT8 across the source  */
    srcRowBytes = rdiBmap->pixBytes;

    /* store mapTable[0] pointer locally */
    srcMapTable = (INT32 *)rdiBmap->mapTable[0];

    /* get the source bitmap's limits  */
    cXmax = rdiBmap->pixWidth - 1;
    cYmax = rdiBmap->pixHeight - 1;

    /* Initialize dstImage header and destination copy parameters  */
    /* assume no span segment  */
    dstImage->imFlags   = 0;

    /* always plane # 1  */
    dstImage->imPlanes  = 1;

    /* 24 bits per pixel  */
    dstImage->imBits    = 24;

    /* set to 0 if we start an even address and one if it is odd  */
    dstImage->imAlign = gblXmin & 1;
    dstImage->imWidth = gblXmax - gblXmin + 1;
    dstImage->imBytes = 3 * ((dstImage->imWidth + dstImage->imAlign + 1) & ~1);

    dstRowBytes = dstImage->imBytes;
    dstPtr = (UINT8 *)(((INT32 ) &dstImage->imData[0]) + (3 * dstImage->imAlign));

    /* Calculate dstImage height in pixels  */
    dstImage->imHeight = gblYmax - gblYmin + 1;

    if( (dstImage->imWidth < 0) || (dstImage->imHeight < 0) )
    {
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
            if( gblXmin < 0 )
            {
                dstPtr -= gblXmin;
                gblXmin = 0;
            }

            /* Check that gblYmin is not off bitmap  */
            if( gblYmin < 0 )
            {
                dstPtr = (UINT8 *) (((INT32 ) dstPtr) + (dstRowBytes * (-gblYmin)));
                gblYmin = 0;
            }

            /* Check that gblYmax is not off bitmap  */
            if( cYmax < gblYmax)
            {
                gblYmax = cYmax;
            }

            /* Check that gblXmax is not off bitmap */
            if( cXmax < gblXmax)
            {
                gblXmax = cXmax;
            }

            /* Now set the horizontal and vertical */
            byteCntM1 = 3 * (gblXmax - gblXmin + 1);
            if( byteCntM1 < 0 )
            {
                done = NU_TRUE;
            }

            if( !done )
            {
                lineCntM1 = (gblYmax - gblYmin);
                if( lineCntM1 < 0)
                {
                    done = NU_TRUE;
                }

                if( !done )
                {
                    /*Set the offset from the end of one dstImage row to the start of the next */
                    dstNextRow = dstRowBytes - byteCntM1;

                    /*Set offset from the end of one source rectangle row to the start of the next */
                    srcNextRow = srcRowBytes - byteCntM1;

                    /* point to row table entry for the first row  */
                    srcPtr = (UINT8 *) *(srcMapTable + gblYmin) + (3 * gblXmin);

                    do
                    {

#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)

                        for( count = 0; count < byteCntM1; count++)
                        {
                            *dstPtr++ = *srcPtr++;
                        }

#else

                        for( count = 0; count < byteCntM1; count+=3)
                        {
                            UINT32 lclColor, lclSrcColr;

                            lclSrcColr = (*srcPtr) |
                                         (*(srcPtr + 1) << 8) |
                                         (*(srcPtr + 2) << 16);

                            /* Convert the color. */
                            COLOR_BACK_CONVERT(lclSrcColr, lclColor);

                            *dstPtr++ = (lclColor & 0xFF);
#if (BPP > 8)
                            *dstPtr++ = ((lclColor >> 8) & 0xFF);
#endif
#if (BPP > 16)
                            *dstPtr++ = ((lclColor >> 16) & 0xFF);
#endif
#if (BPP > 24)
                            *dstPtr++ = ((lclColor >> 24) & 0xFF);
#endif
                            srcPtr += 3;
                        }

#endif

                        dstPtr += dstNextRow;
                        srcPtr += srcNextRow;

                    } while( --lineCntM1 >= 0 );
                }
            }
        }
    }
    nuResume(rdiBmap);
}
#endif /* #ifndef NO_IMAGE_SUPPORT */

#ifdef  GLOBAL_ALPHA_SUPPORT

/***************************************************************************
* FUNCTION
*
*    M24BD_Transparency
*
* DESCRIPTION
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
VOID M24BD_Transparency(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    UINT8 pnColrB, pnColrR, pnColrG;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;
    /* set up local colors */
    pnColrB = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrR = (UINT8)(lclpnColr24  & 0xff);

    if (lclpnColr24 == 0 )
    {
        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                *lclDstPtr = (UINT8)((float)(*lclSrcPtr)*(1.0-alpha_level)+
                             (float)*lclDstPtr*alpha_level);
                lclDstPtr++;
                lclSrcPtr++;

                *lclDstPtr = (UINT8)((float)(*lclSrcPtr)*(1.0-alpha_level)+
                             (float)*lclDstPtr*alpha_level);
                lclDstPtr++;
                lclSrcPtr++;

                *lclDstPtr = (UINT8)((float)(*lclSrcPtr)*(1.0-alpha_level)+
                             (float)*lclDstPtr*alpha_level);
                lclDstPtr++;
                lclSrcPtr++;

#else
                UINT32 lclColor, lclSrcColr;

                lclSrcColr = *lclSrcPtr | (*(lclSrcPtr + 1) << 8) | (*(lclSrcPtr + 2) << 16);

                /* Convert the color. */
                COLOR_CONVERT(lclSrcColr, lclColor);

                /* for each UINT8 in the row */
                *lclDstPtr = (UINT8)((float)(lclColor & 0xFF)*(1.0-alpha_level)+
                             (float)*lclDstPtr*alpha_level);
                lclDstPtr++;

                *lclDstPtr = (UINT8)((float)((lclColor >> 8) & 0xFF)*(1.0-alpha_level)+
                             (float)*lclDstPtr*alpha_level);
                lclDstPtr++;

                *lclDstPtr = (UINT8)((float)((lclColor >> 16) & 0xFF)*(1.0-alpha_level)+
                             (float)*lclDstPtr*alpha_level);
                lclDstPtr++;

                lclSrcPtr += 3;
#endif
            }

            /* advance to next row */
            lclDstPtr += dstNextRow;
            lclSrcPtr += srcNextRow;

        }

    }
    else
    {
        lclLineCnt = lineCntM1;

        while( lclLineCnt-- >= 0 )
        {
        /* for each row */
        lclByteCnt = byteCntM1;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         pnColrR * (1.0-alpha_level));
            lclDstPtr++;


            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         pnColrG*(1.0-alpha_level));
            lclDstPtr++;


            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         pnColrB*(1.0-alpha_level));
            lclDstPtr++;

        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
        }



    }
}

/***************************************************************************
* FUNCTION
*
*    M24BD_Text_Transparency
*
* DESCRIPTION
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
VOID M24BD_Text_Transparency(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr, bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* set up local colors */
    pnColrB = (UINT8)(lclpnColr24 >> 16);
    pnColrG = (UINT8)((lclpnColr24 >> 8) & 0xff);
    pnColrR = (UINT8)(lclpnColr24  & 0xff);

    /* set up local colors */
    bkColrB = (UINT8)(lclbkColr24 >> 16);
    bkColrG = (UINT8)((lclbkColr24 >> 8) & 0xff);
    bkColrR = (UINT8)(lclbkColr24  & 0xff);

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {

            /* for each UINT8 in the row */
            if(*lclSrcPtr & pxBit )
            {
                /* for each UINT8  in the row */
                *lclDstPtr = ((UINT8)((1-alpha_level) * pnColrB) + (UINT8)(alpha_level * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_level) * pnColrG) + (UINT8)(alpha_level * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_level) * pnColrR) + (UINT8)(alpha_level * (*lclDstPtr)));

                lclDstPtr++;

            }
            else
            {
                if(set_back_color == NU_TRUE)
                {
                    /* source is 0 */
                    *lclDstPtr = ((UINT8)((1-alpha_level) * bkColrB) + (UINT8)(alpha_level * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_level) * bkColrG) + (UINT8)(alpha_level * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_level) * bkColrR) + (UINT8)(alpha_level * (*lclDstPtr)));

                }
                else
                {
                    lclDstPtr+=2;
                }
                lclDstPtr++;

            }

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

        /* advance to next row */
        lclDstPtr += dstNextRow;
        lclSrcPtr += srcNextRow;
    }
}

#endif  /* GLOBAL_ALPHA_SUPPORT */

#endif /* #ifdef INCLUDE_24_BIT */

