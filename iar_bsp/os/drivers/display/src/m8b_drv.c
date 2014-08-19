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
*  m8b_drv.c
*
* DESCRIPTION
*
*  This file contains 8-bit memory primitive driver routines.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M8BD_BlitMonoToSelf8Bit
*  M8BD_BlitSelfToSelf8Bit
*  M8BD_DrawRectEntryImg
*  M8BD_DrawRectEntryBlit1M
*  M8BD_DrawRectEntryBlitMM
*  M8BD_DrawRectEntryImg1Bit
*  M8BD_DrawRectEntry_SetPixel
*  M8BD_GetPixelFor8Bit
*  M8BD_InvertTrans1M8Bit
*  M8BD_NotDestSolidM8Bit
*  M8BD_NotDestSolid1M8Bit
*  M8BD_OXADest1M8Bit
*  M8BD_OXADestM8Bit
*  M8BD_ReadImage8Bit
*  M8BD_RepDest1M8Bit
*  M8BD_RepDestM8Bit
*  M8BD_SetTrans1M8Bit
*  M8BD_SetPixelFor8Bit
*  M8BD_WriteImage8Bit
*  M8BD_WriteImage1M8Bit
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

UINT32   lclpnColr, lclbkColr;

/***************************************************************************
* FUNCTION
*
*    M8BD_BlitMonoToSelf8Bit
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
VOID M8BD_BlitMonoToSelf8Bit(blitRcd *blitRec)
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
    lclPortMask = blitRec->blitMask;

    /* pause the source */
    M_PAUSE(srcBmap);

    /* background color */
    bkColr = blitRec->blitBack;

    /* foreground color */
    pnColr = blitRec->blitFore;

    /* Convert the color to target platform. */
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
        optPtr = &M8BD_RepDest1M8Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M8BD_OXADest1M8Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M8BF_SetDestMem;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M8BD_NotDestSolid1M8Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M8BF_InvertDestMem;
        break;

    /* Memory transparent */
    case 16:    /* xREPx   :           !0src         */
    case 17:    /* xORx    :       !0src OR  dst     */
    case 25:    /* xORNx   :     !0src OR  (NOT dst) */
    case 28:    /* xSETx   :           1's           */
        optPtr = &M8BD_SetTrans1M8Bit;
        break;

    case 18:    /* xXORx   :       !0src XOR dst     */
    case 27:    /* xANDNx  :     !0src AND (NOT dst) */
    case 30:    /* xINVERTx:        (NOT dst)        */
        optPtr = &M8BD_InvertTrans1M8Bit;
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
            if(clipToRectFlag != 0 )
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

                if (dRect.Ymin >= dRect.Ymax)
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
                    FillDrawer = &M8BD_DrawRectEntryBlit1M;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M8BD_DrawRectEntryBlit1M(blitRec);
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
*    M8BD_DrawRectEntryBlit1M
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
VOID M8BD_DrawRectEntryBlit1M(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = sRect.Xmin >> 3;
    dstBgnByte  = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    srcNextRow = srcPixBytes - (sRect.Xmax >> 3) + srcBgnByte;
    dstNextRow = dstBmap->pixBytes - byteCntM1 - 1;

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
*    M8BD_NotDestSolid1M8Bit
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
VOID M8BD_NotDestSolid1M8Bit(VOID)
{
    /* invert the destination */
    M8BF_InvertDestMem();

    /* fill this rectangle */
    M8BD_OXADest1M8Bit();
}

/***************************************************************************
* FUNCTION
*
*    M8BD_RepDest1M8Bit
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
VOID M8BD_RepDest1M8Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;
    lclLineCnt = lineCntM1;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*lclSrcPtr, lclColr);

            /* for each UINT8 in the row */
            if( lclColr & pxBit )
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
*    M8BD_SetTrans1M8Bit
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
VOID M8BD_SetTrans1M8Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        pxBit = pxBitVal;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*lclSrcPtr, lclColr);

            /* for each UINT8 in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
            }

            lclDstPtr++;

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
*    M8BD_InvertTrans1M8Bit
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
VOID M8BD_InvertTrans1M8Bit(VOID)
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
        lclByteCnt = byteCntM1;
        pxBit = pxBitVal;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*lclSrcPtr, lclColr);

            /* for each UINT8 in the row */
            if( lclColr & pxBit )
            {
                /* source is 1 */
                pixlVal = *lclDstPtr;
                *lclDstPtr = ~pixlVal;
            }

            lclDstPtr++;

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
*    M8BD_OXADest1M8Bit
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
VOID M8BD_OXADest1M8Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* only two lower bits needed */
    logFnc = (dstClass & 3);

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        pxBit = pxBitVal;
        while( lclByteCnt-- >= 0 )
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*lclSrcPtr, lclColr);

            /* for each UINT8 in the row */
            if( !(lclColr & pxBit) )
            {
                /* source is 0 */
                if( (logFnc == 3) || (*lclDstPtr == 0) )
                {
                    /* "AND" or 0,0 */
                    *lclDstPtr = (UINT8)(lclbkColr & lclPortMask);
                }
                else
                {
                    /* "OR","XOR" with 0,1 */
                    *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
                }
            }
            else
            {
                /* source is 1 */
                if( logFnc == 1 )
                {
                    /* "OR" with 1,X */
                    *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
                }
                else
                {
                    if( logFnc == 3 )
                    {
                        /* "AND" */
                        if( *lclDstPtr )
                        {
                            /* "AND", with 1,1 */
                            *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
                        }
                        else
                        {
                            /* "AND", with 1,0 */
                            *lclDstPtr = (UINT8)(lclbkColr & lclPortMask);
                        }
                    }
                    else
                    {
                        /* "XOR" */
                        if( *lclDstPtr )
                        {
                            /* "XOR", with 1,1 */
                            *lclDstPtr = (UINT8)(lclbkColr & lclPortMask);
                        }
                        else
                        {
                            /* "XOR", with 1,0 */
                            *lclDstPtr = (UINT8)(lclpnColr & lclPortMask);
                        }
                    }
                }
            }

            lclDstPtr++;

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
*    M8BD_BlitSelfToSelf8Bit
*
* DESCRIPTION
*
*    A special case optimization for memory to memory,
*    rectangular clipping with no color translation.
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
VOID M8BD_BlitSelfToSelf8Bit(blitRcd *blitRec)
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

    lclPortMask =  blitRec->blitMask;

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x0f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing. */
    SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    HideCursor();
#endif

    M_PAUSE(dstBmap);

    /* look up the optimization routine */
    switch (dstClass)
    {
                    /* LCD non-transparent */
    case 0:     /* zREPz   :           src           */
    case 4:     /* zNREPz  :        (NOT src)        */
        optPtr = &M8BD_RepDestM8Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M8BD_OXADestM8Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M8BF_SetDestMem;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M8BD_NotDestSolidM8Bit;
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
        /* sets all source bits to 0 */
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

                if( dRect.Xmin >= dRect.Xmax)
                {
                    continue;
                }

#ifndef NO_REGION_CLIP
                
                /* do we need to worry about region clipping? */
                if( clipToRegionFlag != 0 )
                {
                    /* yes */
                    FillDrawer = &M8BD_DrawRectEntryBlitMM;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M8BD_DrawRectEntryBlitMM(blitRec);
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
*    M8BD_DrawRectEntryBlitMM
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
VOID M8BD_DrawRectEntryBlitMM(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = sRect.Xmin;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - byteCntM1 - 1;
    dstNextRow = dstBmap->pixBytes - byteCntM1 - 1;

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
*    M8BD_NotDestSolidM8Bit
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
VOID M8BD_NotDestSolidM8Bit(VOID)
{
    /* invert the destination */
    M8BF_InvertDestMem();

    /* fill this rectangle */
    M8BD_OXADestM8Bit();
}

/***************************************************************************
* FUNCTION
*
*    M8BD_RepDestM8Bit
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
VOID M8BD_RepDestM8Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;

    if( (dstBmap == srcBmap)
        &&
        (   (sRect.Ymin < dRect.Ymin)
            ||
            (  (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin)  )
        )
      )
    {
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
            + dRect.Xmax;
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
            + sRect.Xmax;

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclColr);

                /* for each UINT8 in the row */
                *lclDstPtr-- = lclpnColr ^ lclColr;
                lclSrcPtr--;
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
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclColr);

                /* for each UINT8 in the row */
                *lclDstPtr++ = lclpnColr ^ lclColr;
                lclSrcPtr++;
            }

            /* advance to next row */
            lclDstPtr += dstNextRow;
            lclSrcPtr += srcNextRow;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M8BD_OXADestM8Bit
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
VOID M8BD_OXADestM8Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc, locColr;

    if( (dstBmap == srcBmap)
        &&
        (   (sRect.Ymin < dRect.Ymin)
            ||
            ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin) )
        )
      )
    {
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                            + dRect.Xmax;
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                            + sRect.Xmax;

        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclColr);

                /* for each UINT8 in the row */
                locColr = lclpnColr ^ lclColr;
                lclSrcPtr--;

                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = locColr;
                    break;
                case 2: /* "XOR" */
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = locColr;
                    break;
                case 3: /* "AND" */
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = locColr;
                    break;
                }

                lclDstPtr--;
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
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclColr);

                /* for each UINT8 in the row */
                locColr = lclpnColr ^ lclColr;
                lclSrcPtr++;

                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = locColr;
                    break;
                case 2: /* "XOR" */
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = locColr;
                    break;
                case 3: /* "AND" */
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = locColr;
                    break;
                }

                lclDstPtr++;
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
*    M8BD_GetPixelFor8Bit
*
* DESCRIPTION
*
*     Special case optimization for GetPixel, 8-bit per pixel memory source.
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
INT32 M8BD_GetPixelFor8Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap)
{
    UINT8 *pixelPtr;
    INT32 pixelValue = -1;

    /* check if off bitmap */
    if( (gblX < 0) || (gblY < 0) || (gblX >= getpBmap->pixWidth) ||
        (gblY >= getpBmap->pixHeight))
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

        pixelPtr = (UINT8 *) (*(getpBmap->mapTable[0] + gblY)) + gblX;
        pixelValue = *pixelPtr;

        /* Convert the color to source format. */
        COLOR_BACK_CONVERT(pixelValue, pixelValue);

        nuResume(getpBmap);
    }

    return(pixelValue);
}


/***************************************************************************
* FUNCTION
*
*    M8BD_SetPixelFor8Bit
*
* DESCRIPTION
*
*    A special case optimization for SetPixel,8-bit-per-pixel memory
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
VOID M8BD_SetPixelFor8Bit(blitRcd *setpRec )
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
            M8BD_DrawRectEntry_SetPixel(setpRec);
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
                    M8BD_DrawRectEntry_SetPixel(setpRec);
                }
                
#ifndef NO_REGION_CLIP
                
                else
                {
                    /* yes, clip to the region and draw */
                    dRect.Xmax = dRect.Xmin + 1;
                    dRect.Ymax = dRect.Ymin + 1;
                    FillDrawer = &M8BD_DrawRectEntry_SetPixel;
                    CLIP_Fill_Clip_Region(setpRec, &dRect);
                }
            }
                
#endif  /* NO_REGION_CLIP */
                
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
*    M8BD_DrawRectEntry_SetPixel
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
VOID M8BD_DrawRectEntry_SetPixel(blitRcd *drwPRec)
{
    grafMap *drwGmap;
    INT32  *rowTable;
    UINT8  *bytePtr;
    UINT8  pColor;
    INT16  done = NU_FALSE;

#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* get grafMap */
    drwGmap = drwPRec->blitDmap;


    if( (dRect.Ymin < drwGmap->mapWinYmin[0])   /* below window #0? */
        ||
        (dRect.Ymin > drwGmap->mapWinYmax[0]))  /* above window #0? */
    {
        /* yes, map in bank */
        drwGmap->mapBankMgr(drwGmap, dRect.Ymin, -1, 0);
    }

    /* look up the starting row */
    rowTable = (INT32  *) drwGmap->mapTable[0] + dRect.Ymin;

    /* point to pixel */
    bytePtr = (UINT8 *) ((*rowTable) + dRect.Xmin);

    /* pattern 0? */
    if( drwPRec->blitPat == 0 )
    {
        /* yes, use background color */
        if(drwPRec->blitRop & 0x10)
        {
            /* done if transparent */
            done = NU_TRUE;
        }

        if( !done )
        {
            pColor = (UINT8) drwPRec->blitBack;
        }
    }
    else
    {
        /* no, use foreground color */
        pColor = (UINT8) drwPRec->blitFore;
    }

    /* Convert the color to target format. */
    COLOR_CONVERT(pColor, pColor);

    if( !done )
    {
        /* draw the pixel */
        *bytePtr = pColor;
    }
#ifdef USING_DIRECT_X
    BlitBacktoFront(&scrnRect);
#endif
}

/***************************************************************************
* FUNCTION
*
*    M8BD_WriteImage8Bit
*
* DESCRIPTION
*
*    A special case optimization for 8 Bit WriteImage, interleaved or linear.
*    Also passes through M8BD_WriteImage1M8Bit.
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
VOID M8BD_WriteImage8Bit(blitRcd *blitRec)
{
    rect *rListPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_FALSE;
    INT16 done           = NU_FALSE;

    srcImag = (image *) blitRec->blitSmap;
    srcBmap = 0;

    /*is this a monochrome image? */
    if( srcImag->imBits == 1 )
    {
        /* Let the 1->8 bit handler handle */
        M8BD_WriteImage1M8Bit(blitRec);
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

            dstBmap  =  blitRec->blitDmap;
            dstClass =  blitRec->blitRop & 0x0f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

            /* Call the function for pre-processing. */
            SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

            M_PAUSE(dstBmap);

            /* look up the optimization routine */
            switch (dstClass)
            {
            /* Memory non-transparent */
            case 0:     /* zREPz   :           src           */
            case 4:     /* zNREPz  :        (NOT src)        */
                optPtr = &M8BD_RepDestM8Bit;
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 2:     /* zXORz   :       src XOR dst       */
            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M8BD_OXADestM8Bit;
                break;

            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                optPtr = &M8BF_SetDestMem;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M8BD_NotDestSolidM8Bit;
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
                if( (dRect.Ymax - dRect.Ymin) > srcImag->imHeight)
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
                        /* sXmax not altered because not used  */
                    }
                    if( dRect.Ymax > cRect.Ymax )
                    {
                        /* Reset src & dst Ymax clip limit */
                        dRect.Ymax  = cRect.Ymax;
                        /* sYmax not altered because not used  */
                    }
                    if( (dRect.Xmin >= dRect.Xmax) || (dRect.Ymin >= dRect.Ymax) )
                    {
                        done = NU_TRUE;
                    }

#ifndef NO_REGION_CLIP
                
                    if( !done )
                        {/* do we need to worry about region clipping?  */
                        if( clipToRegionFlag != 0 )
                        {
                            /* yes, go do it  */
                            FillDrawer = &M8BD_DrawRectEntryImg;
                            CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                            done = NU_TRUE;
                        }
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                if ( !done )
                {
                    /* Draw the image */
                    M8BD_DrawRectEntryImg(blitRec);
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
*    M8BD_DrawRectEntryImg
*
* DESCRIPTION
*
*    Draw the Image.
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
VOID M8BD_DrawRectEntryImg(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = dstBmap->pixBytes - byteCntM1 - 1;
    srcNextRow = srcPixBytes - byteCntM1 - 1;
    srcBgnByte = sRect.Xmin;

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
*    M8BD_WriteImage1M8Bit
*
* DESCRIPTION
*
*    A special case optimization for 1->8Bit WriteImage.
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
VOID M8BD_WriteImage1M8Bit(blitRcd *blitRec)
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
        rListPtr = (rect *) blitRec->blitList;

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
        COLOR_CONVERT(pnColr,lclpnColr);
        COLOR_CONVERT(bkColr,lclbkColr);

        /* look up the optimization routine */
        switch (dstClass)
        {
                        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
            optPtr = &M8BD_RepDest1M8Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M8BD_OXADest1M8Bit;
            break;

        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M8BF_SetDestMem;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M8BD_NotDestSolid1M8Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M8BF_InvertDestMem;
            break;

            /* Memory transparent */
        case 16:    /* xREPx   :           !0src         */
        case 17:    /* xORx    :       !0src OR  dst     */
        case 25:    /* xORNx   :     !0src OR  (NOT dst) */
        case 28:    /* xSETx   :           1's           */
            optPtr = &M8BD_SetTrans1M8Bit;
            break;

        case 18:    /* xXORx   :       !0src XOR dst     */
        case 27:    /* xANDNx  :     !0src AND (NOT dst) */
        case 30:    /* xINVERTx:        (NOT dst)        */
            optPtr = &M8BD_InvertTrans1M8Bit;
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
                        FillDrawer = &M8BD_DrawRectEntryImg1Bit;
                        CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                        done = NU_TRUE;
                    }
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            if( !done )
            {
                /* Blit the image to the memory */
                M8BD_DrawRectEntryImg1Bit(blitRec);
            }
        }
        nuResume(dstBmap);
    }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M8BD_DrawRectEntryImg1Bit
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
VOID M8BD_DrawRectEntryImg1Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    srcBgnByte = (sRect.Xmin >> 3);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = dstBmap->pixBytes - byteCntM1 - 1;
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
*    M8BD_ReadImage8Bit
*
* DESCRIPTION
*
*    A special case optimization for ReadImage, 8 bit per pixel,
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
VOID M8BD_ReadImage8Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                        INT32 gblYmin, INT32 gblXmin)
{
    /* # of bytes across a source bitmap row  */
    INT32  srcRowBytes;

    /* # of bytes across an dstImage row  */
    INT32  dstRowBytes;

    /* bitmap limits  */
    INT32  cXmax,cYmax;
    INT16  grafErrValue;
    INT32  *srcMapTable;
    INT32  count;
    INT32  done = NU_FALSE;

    gblXmax--;
    gblYmax--;

    M_PAUSE(rdiBmap);

    /* # of UINT8 across the source  */
    srcRowBytes = rdiBmap->pixBytes;

    /* store mapTable[0] pointer locally  */
    srcMapTable = (INT32  *)rdiBmap->mapTable[0];

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

    /* set to 0 if we start an even address and one if it is odd  */
    dstImage->imAlign = gblXmin & 1;
    dstImage->imWidth = gblXmax - gblXmin + 1;
    dstImage->imBytes = (dstImage->imWidth + dstImage->imAlign + 1) & ~1;
    dstRowBytes = dstImage->imBytes;

    dstPtr = (UINT8 *)(((INT32 ) &dstImage->imData[0]) + dstImage->imAlign);

    /* Calculate dstImage height in pixels  */
    dstImage->imHeight = gblYmax - gblYmin + 1;

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
            byteCntM1 = (gblXmax - gblXmin);
            if( byteCntM1 < 0)
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
                    /*Set the offset from the end of one dstImage row to the start of the next  */
                    dstNextRow = dstRowBytes - byteCntM1 - 1;

                    /*Set offset from the end of one source rectangle row to the start of the next  */
                    srcNextRow = srcRowBytes - byteCntM1 - 1;

                    /* point to row table entry for the first row  */
                    srcPtr = (UINT8 *) *(srcMapTable + gblYmin) + gblXmin;

                    do{
                        for( count = 0; count <= byteCntM1; count++)
                        {
                            UINT8 lclColr;

                            /* Convert the color to target format. */
                            COLOR_BACK_CONVERT(*srcPtr, lclColr);

                            *dstPtr++ = lclColr;
                            srcPtr++;
                        }

                        dstPtr += dstNextRow;
                        srcPtr += srcNextRow;

                    }while( --lineCntM1 >= 0 );
                }
            }
        }
    }
    nuResume(rdiBmap);

}

#endif /* #ifndef NO_IMAGE_SUPPORT */

#endif /* #ifdef INCLUDE_8_BIT */


