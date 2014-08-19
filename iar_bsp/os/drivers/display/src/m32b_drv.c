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
*  m32b_drv.c
*
* DESCRIPTION
*
*  This file contains 32-bit memory primitive driver routines.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*   M32BD_BlitMonoToSelf32Bit
*   M32BD_BlitSelfToSelf32Bit
*   M32BD_DrawRectEntryImg32Bit
*   M32BD_DrawRectEntryBlitMM32Bit
*   M32BD_DrawRectEntryImg1B32
*   M32BD_DrawRectEntryBlit1M32Bit
*   M32BD_DrawRectEntry_SetPixel32
*   M32BD_GetPixelFor32Bit
*   M32BD_InvertTrans1M32Bit
*   M32BD_NotDestSolid1M32Bit
*   M32BD_NotDestSolidM32Bit
*   M32BD_OXADest1M32Bit
*   M32BD_OXADestM32Bit
*   M32BD_ReadImage32Bit
*   M32BD_RepDestM32Bit
*   M32BD_RepDest1M32Bit
*   M32BD_SetPixelFor32Bit
*   M32BD_SetTrans1M32Bit
*   M32BD_WriteImage32Bit
*   M32BD_WriteImage1M32Bit
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

UINT32 lclpnColr32, lclbkColr32;

/***************************************************************************
* FUNCTION
*
*    M32BD_BlitMonoToSelf32Bit
*
* DESCRIPTION
*
*    A special case optimization for monochrome to 32-bit memory,
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
VOID M32BD_BlitMonoToSelf32Bit(blitRcd *blitRec)
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
    bkColr32 = blitRec->blitBack;

    /* foreground color */
    pnColr32 = blitRec->blitFore;

    /* Convert the colors to target format. */
    COLOR_CONVERT(pnColr32, lclpnColr32);
    COLOR_CONVERT(bkColr32, lclbkColr32);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x1f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
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
        optPtr = &M32BD_RepDest1M32Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M32BD_OXADest1M32Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M32BF_SetDestMem32;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M32BD_NotDestSolid1M32Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;
    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M32BF_InvertDestMem32;
        break;

    /* Memory transparent */
    case 16:    /* xREPx   :           !0src         */
    case 17:    /* xORx    :       !0src OR  dst     */
    case 25:    /* xORNx   :     !0src OR  (NOT dst) */
    case 28:    /* xSETx   :           1's           */
        optPtr = &M32BD_SetTrans1M32Bit;
        break;

    case 18:    /* xXORx   :       !0src XOR dst     */
    case 27:    /* xANDNx  :     !0src AND (NOT dst) */
    case 30:    /* xINVERTx:        (NOT dst)        */
        optPtr = &M32BD_InvertTrans1M32Bit;
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
                    FillDrawer = &M32BD_DrawRectEntryBlit1M32Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M32BD_DrawRectEntryBlit1M32Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M32BD_DrawRectEntryBlit1M32Bit
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
VOID M32BD_DrawRectEntryBlit1M32Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = sRect.Xmin >> 3;

    dstBgnByte = 4 * dRect.Xmin;

    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    srcNextRow = srcPixBytes - (sRect.Xmax >> 3) + srcBgnByte;

    dstNextRow = dstBmap->pixBytes - (4 * byteCntM1) - 4;


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
*    M32BD_NotDestSolid1M32Bit
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
VOID M32BD_NotDestSolid1M32Bit(VOID)
{
    /* invert the destination */
    M32BF_InvertDestMem32();

    /* fill this rectangle */
    M32BD_OXADest1M32Bit();
}

/***************************************************************************
* FUNCTION
*
*    M32BD_RepDest1M32Bit
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
VOID M32BD_RepDest1M32Bit(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr, bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
    UINT8 bkColrA = (UINT8)(lclbkColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* set up local colors */
    pnColrR = (UINT8)((lclpnColr32 >> 16) & 0xFF);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xFF);
    pnColrB = (UINT8)(lclpnColr32  & 0xFF);

    bkColrR = (UINT8)((lclbkColr32 >> 16) & 0xFF);
    bkColrG = (UINT8)((lclbkColr32 >> 8) & 0xFF);
    bkColrB = (UINT8)(lclbkColr32  & 0xFF);

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            double alpha_value;

            /* Get the alpha value to blend the pixel */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
            if (alpha_level != 0.0)
            {
                alpha_value = alpha_level;
            }
            else

#endif  /* GLOBAL_ALPHA_SUPPORT */

            {
                alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);
            }

            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* for each UINT8  in the row */
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                lclDstPtr++;
            }
#else
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* for each UINT8  in the row */
                *lclDstPtr++ = pnColrB;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrA;
            }
#endif

            else
            {
                if(set_back_color == NU_TRUE)
                {
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)
                    /* source is 0 */
                    *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    lclDstPtr++;
#else
                    /* source is 0 */
                    *lclDstPtr++ = bkColrB;
                    *lclDstPtr++ = bkColrG;
                    *lclDstPtr++ = bkColrR;
                    *lclDstPtr++ = bkColrA;
#endif
                }
                else
                {
                    lclDstPtr+=4;
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
*    M32BD_SetTrans1M32Bit
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr32 if the source is 1 else it
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
VOID M32BD_SetTrans1M32Bit(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    UINT8 pnColrR, pnColrG, pnColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;


    /* set up local colors */
    pnColrB = (UINT8)((lclpnColr32 >> 16) & 0xff);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xff);
    pnColrR = (UINT8)(lclpnColr32  & 0xff);

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            /* Get the alpha value to blend the pixel */
            double alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);

            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip alpha byte. */
                lclDstPtr++;
            }
#else
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr++ = pnColrB;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrA;
            }
#endif

            else
            {

                lclDstPtr += 4;
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
*    M32BD_InvertTrans1M32Bit
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
VOID M32BD_InvertTrans1M32Bit(VOID)
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
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)
            /* Get the alpha value to blend the pixel */
            double alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);

            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr = ((UINT8)((1-alpha_value) * ~(*lclDstPtr)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * ~(*lclDstPtr)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * ~(*lclDstPtr)) + (UINT8)(alpha_value * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip Alpha byte */
                lclDstPtr++;
            }
#else
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
                *lclDstPtr = ~(*lclDstPtr);
                lclDstPtr++;
                *lclDstPtr = ~(*lclDstPtr);
                lclDstPtr++;
                *lclDstPtr = ~(*lclDstPtr);
                lclDstPtr++;
                *lclDstPtr = ~(*lclDstPtr);
                lclDstPtr++;
            }
#endif
            else
            {
                lclDstPtr += 4;
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
*    M32BD_OXADest1M32Bit
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
VOID M32BD_OXADest1M32Bit(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr, bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc;
    INT32 pxBit;
    INT32 pxBitVal;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
    UINT8 bkColrA = (UINT8)(lclbkColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* set up local colors */
    pnColrB = (UINT8)((lclpnColr32 >> 16) & 0xff);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xff);
    pnColrR = (UINT8)(lclpnColr32  & 0xff);

    bkColrB = (UINT8)((lclbkColr32 >> 16) & 0xff);
    bkColrG = (UINT8)((lclbkColr32 >> 8) & 0xff);
    bkColrR = (UINT8)(lclbkColr32  & 0xff);

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
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

            /* Get the alpha value to blend the pixel */
            double alpha_value = (double)((double)(*(lclDstPtr + 3))/255.0);

            /* for each UINT8 in the row */
            if( !(*lclSrcPtr & pxBit) )
            {
                /* source is 0 */
                if( (logFnc == 3) || (*lclDstPtr == 0) )
                {
                    /* "AND" or 0,0 */
                    *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;

                    /* Skip Alpha Byte */
                    lclDstPtr++;
                }
                else
                {
                    /* "OR","XOR" with 0,1 */
                    *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;

                    /* Skip Alpha Byte */
                    lclDstPtr++;
                }
            }
            else
            {
                /* source is 1 */
                if( logFnc == 1 )
                {
                    /* "OR" with 1,X */
                    *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                    lclDstPtr++;

                    /* Skip Alpha Byte */
                    lclDstPtr++;
                }
                else
                {
                    if( logFnc == 3 )
                    {
                        /* "AND" */
                        if( *lclDstPtr )
                        {
                            /* "AND", with 1,1 */
                            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;

                            /* Skip Alpha Byte */
                            lclDstPtr++;
                        }
                        else
                        {
                            /* "AND", with 1,0 */
                            *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;

                            /* Skip Alpha Byte */
                            lclDstPtr++;
                        }
                    }
                    else
                    {
                        /* "XOR" */
                        if( *lclDstPtr )
                        {
                            /* "XOR", with 1,1 */
                            *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * bkColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;

                            /* Skip Alpha Byte */
                            lclDstPtr++;
                        }
                        else
                        {
                            /* "XOR", with 1,0 */
                            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrB) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrG) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;
                            *lclDstPtr = ((UINT8)((1-alpha_value) * pnColrR) + (UINT8)(alpha_value * (*lclDstPtr)));
                            lclDstPtr++;

                            /* Skip Alpha Byte */
                            lclDstPtr++;
                        }
                    }
                }
            }

#else

            /* for each UINT8 in the row */
            if( !(*lclSrcPtr & pxBit) )
            {
                /* source is 0 */
                if( (logFnc == 3) || (*lclDstPtr == 0) )
                {
                    /* "AND" or 0,0 */
                    *lclDstPtr++ = bkColrB;
                    *lclDstPtr++ = bkColrG;
                    *lclDstPtr++ = bkColrR;
                    *lclDstPtr++ = bkColrA;
                }
                else
                {
                    /* "OR","XOR" with 0,1 */
                    *lclDstPtr++ = pnColrB;
                    *lclDstPtr++ = pnColrG;
                    *lclDstPtr++ = pnColrR;
                    *lclDstPtr++ = pnColrA;
                }
            }
            else
            {
                /* source is 1 */
                if( logFnc == 1 )
                {
                    /* "OR" with 1,X */
                    *lclDstPtr++ = pnColrB;
                    *lclDstPtr++ = pnColrG;
                    *lclDstPtr++ = pnColrR;
                    *lclDstPtr++ = pnColrA;
                }
                else
                {
                    if( logFnc == 3 )
                    {
                        /* "AND" */
                        if( *lclDstPtr )
                        {
                            /* "AND", with 1,1 */
                            *lclDstPtr++ = pnColrB;
                            *lclDstPtr++ = pnColrG;
                            *lclDstPtr++ = pnColrR;
                            *lclDstPtr++ = pnColrA;
                        }
                        else
                        {
                            /* "AND", with 1,0 */
                            *lclDstPtr++ = bkColrB;
                            *lclDstPtr++ = bkColrG;
                            *lclDstPtr++ = bkColrR;
                            *lclDstPtr++ = bkColrA;
                        }
                    }
                    else
                    {
                        /* "XOR" */
                        if( *lclDstPtr )
                        {
                            /* "XOR", with 1,1 */
                            *lclDstPtr++ = bkColrB;
                            *lclDstPtr++ = bkColrG;
                            *lclDstPtr++ = bkColrR;
                            *lclDstPtr++ = bkColrA;
                        }
                        else
                        {
                            /* "XOR", with 1,0 */
                            *lclDstPtr++ = pnColrB;
                            *lclDstPtr++ = pnColrG;
                            *lclDstPtr++ = pnColrR;
                            *lclDstPtr++ = pnColrA;
                        }
                    }
                }
            }

#endif /* (HARDWARE_ALPHA_SUPPORTED == NU_FALSE) */

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
*    M32BD_BlitSelfToSelf32Bit
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
VOID M32BD_BlitSelfToSelf32Bit(blitRcd *blitRec)
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
    bkColr32 = blitRec->blitBack;

    /* foreground color */
    pnColr32 = blitRec->blitFore;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr32, lclpnColr32);
    COLOR_CONVERT(bkColr32, lclbkColr32);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x0f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call the function for pre-processing task. */
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
        optPtr = &M32BD_RepDestM32Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M32BD_OXADestM32Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M32BF_SetDestMem32;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M32BD_NotDestSolidM32Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M32BF_InvertDestMem32;
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
        pnColr32 = ~0;
        lclpnColr32 = ~0;
        break;

    case 0:     /* zREPz  */
    case 1:     /* zORz   */
    case 2:     /* zXORz  */
    case 7:     /* zANDz  */
    case 9:     /* zORNz  */
    case 10:    /* zNOPz  */
    case 11:    /* zANDNz */
        pnColr32 = 0;
        lclpnColr32 = 0;
        break;

    case 8:     /* zCLEARz */
        /* set all source bits to 0 */
        pnColr32 = 0;
        lclpnColr32 = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* sets all source bits to 1 */
        pnColr32 = ~0;
        lclpnColr32 = ~0;
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
                    FillDrawer = &M32BD_DrawRectEntryBlitMM32Bit;

                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0)
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M32BD_DrawRectEntryBlitMM32Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif
    
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M32BD_DrawRectEntryBlitMM32Bit
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
VOID M32BD_DrawRectEntryBlitMM32Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    srcBgnByte = 4 * sRect.Xmin;
    dstBgnByte = 4 * dRect.Xmin;
    srcNextRow = srcPixBytes - (4 * byteCntM1) - 4;
    dstNextRow = dstBmap->pixBytes - (4 * byteCntM1) - 4;

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
*    M32BD_NotDestSolidM32Bit
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
VOID M32BD_NotDestSolidM32Bit(VOID)
{
    /* invert the destination */
    M32BF_InvertDestMem32();

    /* fill this rectangle */
    M32BD_OXADestM32Bit();
}

/***************************************************************************
* FUNCTION
*
*    M32BD_RepDestM32Bit
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
VOID M32BD_RepDestM32Bit(VOID)
{
	/* Use register variables for improved performance */
    R1 UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    R1 UINT8 *lclSrcPtr;
    R1 INT32 lclLineCnt, lclByteCnt;
    R1 double alpha_value;

    /* set up local colors */

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
#endif

    pnColrR = (UINT8)((lclpnColr32 >> 16)  & 0xff);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xff);
    pnColrB = (UINT8)((lclpnColr32 >> 0) & 0xff);

    /* Suppress the harmless warning. */
    (VOID)alpha_value;

    if( (dstBmap == srcBmap)
        &&
        ( (sRect.Ymin < dRect.Ymin)
          ||
        ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin)))
      )
    {
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                + (4 * dRect.Xmax) + 4;
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                                + (4 * sRect.Xmax) + 4;

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;

            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)

#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

                /* Get the alpha value to blend the pixel */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
                if (alpha_level != 0.0)
                {
                    alpha_value = alpha_level;
                }
                else

#endif  /* GLOBAL_ALPHA_SUPPORT */

                {
                    alpha_value = (float)((float)(*(lclSrcPtr + 3))/255.0);
                }

                /* for each UINT8 in the row */
                *lclDstPtr = ((UINT8)((alpha_value) * (*lclDstPtr)) + (UINT8)((1-alpha_value) * (pnColrR ^ *lclSrcPtr)));
                lclDstPtr--;
                lclSrcPtr--;
                *lclDstPtr = ((UINT8)((alpha_value) * (*lclDstPtr)) + (UINT8)((1-alpha_value) * (pnColrG ^ *lclSrcPtr)));
                lclDstPtr--;
                lclSrcPtr--;
                *lclDstPtr = ((UINT8)((alpha_value) * (*lclDstPtr)) + (UINT8)((1-alpha_value)* (pnColrB ^ *lclSrcPtr)));
                lclDstPtr--;
                lclSrcPtr--;

                /* Skip the Alpha Byte */
                lclDstPtr--;
                lclSrcPtr--;
#else
                /* for each UINT8 in the row */
                *lclDstPtr = (pnColrB ^ *lclSrcPtr);
                lclDstPtr--;
                lclSrcPtr--;
                *lclDstPtr = (pnColrG ^ *lclSrcPtr);
                lclDstPtr--;
                lclSrcPtr--;
                *lclDstPtr = (pnColrR ^ *lclSrcPtr);
                lclDstPtr--;
                lclSrcPtr--;
                *lclDstPtr = (pnColrA ^ *lclSrcPtr);
                lclDstPtr--;
                lclSrcPtr--;
#endif

#else
                UINT32  lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*((UINT32 *)lclSrcPtr), lclSrcColr);

#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

                /* Get the alpha value to blend the pixel */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
                if (alpha_level != 0.0)
                {
                    alpha_value = alpha_level;
                }
                else

#endif  /* GLOBAL_ALPHA_SUPPORT */

                {
                    alpha_value = (float)((float)(lclSrcColr >> 24)/255.0);
                }

                /* for each UINT8 in the row */
                *lclDstPtr = ((UINT8)((alpha_value) * (*lclDstPtr)) + (UINT8)((1-alpha_value) * (pnColrR ^ (lclSrcColr & 0xFF))));
                lclDstPtr--;

                *lclDstPtr = ((UINT8)((alpha_value) * (*lclDstPtr)) + (UINT8)((1-alpha_value) * (pnColrG ^ ((lclSrcColr >> 8) & 0xFF))));
                lclDstPtr--;

                *lclDstPtr = ((UINT8)((alpha_value) * (*lclDstPtr)) + (UINT8)((1-alpha_value)* (pnColrB ^ ((lclSrcColr >> 16) & 0xFF))));
                lclDstPtr--;

                /* Skip the Alpha Byte */
                lclDstPtr--;

                lclSrcPtr-=4;
#else
                /* for each UINT8 in the row */
                *lclDstPtr = (pnColrB ^ ((lclSrcColr) & 0xFF));
                lclDstPtr--;

                *lclDstPtr = (pnColrG ^ ((lclSrcColr >> 8) & 0xFF));
                lclDstPtr--;

                *lclDstPtr = (pnColrR ^ ((lclSrcColr >> 16) & 0xFF));
                lclDstPtr--;

                *lclDstPtr = (pnColrA ^ ((lclSrcColr >> 24) & 0xFF));
                lclDstPtr--;

                lclSrcPtr-=4;
#endif

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
            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)

#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

                /* Get the alpha value to blend the pixel */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
                if (alpha_level != 0.0)
                {
                    alpha_value = alpha_level;
                }
                else

#endif  /* GLOBAL_ALPHA_SUPPORT */

                {
                    alpha_value = (double)((double)(*(lclSrcPtr + 3))/255.0);
                }

                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclSrcPtr)) + (UINT8)((alpha_value) * ( *lclDstPtr)));
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclSrcPtr)) + (UINT8)((alpha_value) * ( *lclDstPtr)));
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclSrcPtr)) + (UINT8)((alpha_value) * (*lclDstPtr)));
                lclDstPtr++;
                lclSrcPtr++;

                /* Skip Alpha Byte */
                lclDstPtr++;
                lclSrcPtr++;
#else

#ifdef	LCD_OPTIMIZE_BLIT
                /* Both buffers are 4 byte aligned */
                if( ESAL_GE_MEM_ALIGNED_CHECK(lclDstPtr, 4) && ESAL_GE_MEM_ALIGNED_CHECK(lclSrcPtr, 4) )
                {
                    /* Copy 4 bytes at a time to improve performance */
                    *((UINT32 *)lclDstPtr) = *((UINT32 *)lclSrcPtr);
                    lclDstPtr += 4;
                    lclSrcPtr += 4;
                }
                /* Both buffers are 2 byte aligned */
                else if( ESAL_GE_MEM_ALIGNED_CHECK(lclDstPtr, 2) && ESAL_GE_MEM_ALIGNED_CHECK(lclSrcPtr, 2) )
                {
                    *((UINT16 *)lclDstPtr) = *((UINT16 *)lclSrcPtr);
                    lclDstPtr += 2;
                    lclSrcPtr += 2;

                    *((UINT16 *)lclDstPtr) = *((UINT16 *)lclSrcPtr);
                    lclDstPtr += 2;
                    lclSrcPtr += 2;
                }
                /* Buffers are unaligned
                * Each byte will be copied individually */
                else
                {
                    *lclDstPtr = *lclSrcPtr;
                    lclDstPtr++;
                    lclSrcPtr++;
                    *lclDstPtr = *lclSrcPtr;
                    lclDstPtr++;
                    lclSrcPtr++;
                    *lclDstPtr = *lclSrcPtr;
                    lclDstPtr++;
                    lclSrcPtr++;
                    *lclDstPtr = *lclSrcPtr;
                    lclDstPtr++;
                    lclSrcPtr++;
                }

#else				
                *lclDstPtr = *lclSrcPtr;
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = *lclSrcPtr;
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = *lclSrcPtr;
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = *lclSrcPtr;
                lclDstPtr++;
                lclSrcPtr++;
#endif

#endif

#else
                UINT32  lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*((UINT32 *)lclSrcPtr), lclSrcColr);

#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)

                /* Get the alpha value to blend the pixel */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
                if (alpha_level != 0.0)
                {
                    alpha_value = alpha_level;
                }
                else

#endif  /* GLOBAL_ALPHA_SUPPORT */

                {
                    alpha_value = (double)((double)(lclSrcColr >> 24)/255.0);
                }

                *lclDstPtr = ((UINT8)((1-alpha_value) * (lclSrcColr & 0xFF)) + (UINT8)((alpha_value) * ( *lclDstPtr)));
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * ((lclSrcColr >> 8) & 0xFF)) + (UINT8)((alpha_value) * ( *lclDstPtr)));
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_value) * ((lclSrcColr >> 16) & 0xFF)) + (UINT8)((alpha_value) * (*lclDstPtr)));
                lclDstPtr++;
                lclSrcPtr++;

                /* Skip Alpha Byte */
                lclDstPtr++;
                lclSrcPtr++;
#else
                *lclDstPtr = (lclSrcColr & 0xFF);
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((lclSrcColr >> 8) & 0xFF);
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((lclSrcColr >> 16) & 0xFF);
                lclDstPtr++;
                lclSrcPtr++;
                *lclDstPtr = ((lclSrcColr >> 24) & 0xFF);
                lclDstPtr++;
                lclSrcPtr++;
#endif

#endif
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
*    M32BD_OXADestM32Bit
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
VOID M32BD_OXADestM32Bit(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB, pnColrA;
    UINT8 *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 logFnc, locColr;

    /* Suppress the harmless warning. */
    (VOID)pnColrA;

    /* set up local colors */
    pnColrA = (UINT8)(lclpnColr32 >> 24);
    pnColrR = (UINT8)((lclpnColr32 >> 16)  & 0xff);
    pnColrG = (UINT8)((lclpnColr32 >> 8) & 0xff);
    pnColrB = (UINT8)((lclpnColr32 >> 0) & 0xff);

    if( (dstBmap == srcBmap)
        &&
        ( (sRect.Ymin < dRect.Ymin)
          ||
        ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin) ) )
      )
    {
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                                + (4 * dRect.Xmax) + 4;
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                                + (4 * sRect.Xmax) + 4;


        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;


            while( lclByteCnt-- >= 0 )
            {
                double alpha_value;
                UINT32 lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*((UINT32 *)lclSrcPtr),lclSrcColr);

                /* Get the alpha value to blend the pixel */
                
#ifdef  GLOBAL_ALPHA_SUPPORT
        
                if (alpha_level != 0.0)
                {
                    alpha_value = alpha_level;
                }
                else

#endif  /* GLOBAL_ALPHA_SUPPORT */

                {
                    alpha_value = (float)((float)(lclSrcColr >> 24)/255.0);
                }

                /* for each UINT8 in the row */
                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
                    locColr = pnColrB ^ ((lclSrcColr >> 0) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    locColr = pnColrG ^ ((lclSrcColr >> 8) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    locColr = pnColrR ^ ((lclSrcColr >> 16) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    /* Skip Alpha Byte */
                    lclDstPtr--;
                    lclSrcPtr-=4;
                    break;

                case 2: /* "XOR" */

                    locColr = pnColrB ^ ((lclSrcColr >> 0) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    locColr = pnColrG ^ ((lclSrcColr >> 8) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    locColr = pnColrR ^ ((lclSrcColr >> 16) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    /* Skip Alpha Byte */
                    lclDstPtr--;
                    lclSrcPtr-=4;


                    break;

                case 3: /* "AND" */

                    locColr = pnColrB ^ ((lclSrcColr >> 0) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    locColr = pnColrG ^ ((lclSrcColr >> 8) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    locColr = pnColrR ^ ((lclSrcColr >> 16) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr--;

                    /* Skip Alpha Byte */
                    lclDstPtr--;
                    lclSrcPtr-=4;

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
                UINT32 lclSrcColr;
                double alpha_value;

                /* Convert the color to target format. */
                COLOR_CONVERT(*((UINT32 *)lclSrcPtr),lclSrcColr);

                /* Get the alpha value to blend the pixel */
                
#ifdef  GLOBAL_ALPHA_SUPPORT
        
                if (alpha_level != 0.0)
                {
                    alpha_value = alpha_level;
                }
                else

#endif  /* GLOBAL_ALPHA_SUPPORT */

                {
                    alpha_value = (float)((float)(lclSrcColr >> 24)/255.0);
                }
                
                /* for each UINT8 in the row */
                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
                    locColr = pnColrB ^ (lclSrcColr & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    locColr = pnColrG ^ ((lclSrcColr >> 8) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    locColr = pnColrR ^ ((lclSrcColr >> 16) & 0xFF);
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    /* Skip Alpha Byte */
                    lclDstPtr++;
                    lclSrcPtr+=4;

                    break;

                case 2: /* "XOR" */


                    locColr = pnColrB ^ (lclSrcColr & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    locColr = pnColrG ^ ((lclSrcColr >> 8) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    locColr = pnColrR ^ ((lclSrcColr >> 16) & 0xFF);
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    /* Skip Alpha Byte */
                    lclDstPtr++;
                    lclSrcPtr+=4;

                    break;

                case 3: /* "AND" */


                    locColr = pnColrB ^ (lclSrcColr & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    locColr = pnColrG ^ ((lclSrcColr >> 8) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    locColr = pnColrR ^ ((lclSrcColr >> 16) & 0xFF);
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = ((UINT8)((1-alpha_value) * (*lclDstPtr)) + (UINT8)(alpha_value * locColr));
                    lclDstPtr++;

                    /* Skip Alpha Byte */
                    lclDstPtr++;
                    lclSrcPtr+=4;

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
*    M32BD_GetPixelFor32Bit
*
* DESCRIPTION
*
*     Special case optimization for GetPixel, 32-bit per pixel memory source.
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
INT32 M32BD_GetPixelFor32Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap)
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

        pixelPtr = (UINT8 *) (*(getpBmap->mapTable[0] + gblY)) + (4 * gblX);

        pixelValue |=  *pixelPtr++;
        pixelValue |= (*pixelPtr++ << 8);
        pixelValue |= (*pixelPtr++ << 16);
        pixelValue  = (*pixelPtr++ << 24);

        /* Convert the pixel color back to the source format. */
        COLOR_BACK_CONVERT(pixelValue,pixelValue);

        nuResume(getpBmap);
    }

    return(pixelValue);
}

/***************************************************************************
* FUNCTION
*
*    M32BD_SetPixelFor32Bit
*
* DESCRIPTION
*
*    A special case optimization for SetPixel,32-bit-per-pixel memory
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
VOID M32BD_SetPixelFor32Bit(blitRcd *setpRec)
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
            M32BD_DrawRectEntry_SetPixel32(setpRec);
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
                    M32BD_DrawRectEntry_SetPixel32(setpRec);
                }

#ifndef NO_REGION_CLIP
                
                else
                {
                    /* yes, clip to the region and draw */
                    dRect.Xmax = dRect.Xmin + 1;
                    dRect.Ymax = dRect.Ymin + 1;
                    FillDrawer = &M32BD_DrawRectEntry_SetPixel32;
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
*    M32BD_DrawRectEntry_SetPixel32
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
VOID M32BD_DrawRectEntry_SetPixel32(blitRcd *drwPRec)
{
    grafMap *drwGmap;
    long    *rowTable;
    UINT8   *bytePtr;
    UINT8   pColorR, pColorG, pColorB;
#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8   pColorA;
#endif
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
    bytePtr = (UINT8 *) ((*rowTable) + (4 * dRect.Xmin));

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

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
            pColorA = (UINT8) (  lclblitBack >> 24 );
#endif
            pColorR = (UINT8) ( (lclblitBack >> 16) & 0xff );
            pColorG = (UINT8) ( (lclblitBack >> 8) & 0xff );
            pColorB = (UINT8) ( (lclblitBack >> 0) & 0xff );
        }
    }
    else
    {
        UINT32 lclblitFore;

        /* Convert the color to target format. */
        COLOR_CONVERT(drwPRec->blitFore, lclblitFore);

        /* no, use foreground color */
#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
        pColorA = (UINT8) (  lclblitFore >> 24 );
#endif
        pColorR = (UINT8) ( (lclblitFore >> 16) & 0xff );
        pColorG = (UINT8) ( (lclblitFore >> 8) & 0xff );
        pColorB = (UINT8) ( (lclblitFore >> 0) & 0xff );
    }

    if( !done )
    {
        /* draw the pixel */
        *bytePtr++ = pColorB;
        *bytePtr++ = pColorG;
        *bytePtr++ = pColorR;
#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
        *bytePtr++ = pColorA;
#endif
    }

#ifdef USING_DIRECT_X
    BlitBacktoFront(&scrnRect);
#endif
}

/***************************************************************************
* FUNCTION
*
*    M32BD_WriteImage32Bit
*
* DESCRIPTION
*
*    A special case optimization for 32 Bit WriteImage, interleaved or linear.
*    Also passes through M32BD_WriteImage1M32Bit.
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
VOID M32BD_WriteImage32Bit(blitRcd *blitRec)
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
        M32BD_WriteImage1M32Bit(blitRec);
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
            dstClass = blitRec->blitRop & 0x0f;

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
                optPtr = &M32BD_RepDestM32Bit;
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 2:     /* zXORz   :       src XOR dst       */
            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M32BD_OXADestM32Bit;
                break;

            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                optPtr = &M32BF_SetDestMem32;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M32BD_NotDestSolidM32Bit;
                break;

            case 10:    /* zNOPz   :           dst <NOP>     */
                optPtr = &SCREENS_Nop;
                break;

            case 14:    /* zINVERTz:        (NOT dst)        */
                optPtr = &M32BF_InvertDestMem32;
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
                pnColr32 = ~0;
                lclpnColr32 = ~0;
                break;

            case 0:     /* zREPz  */
            case 1:     /* zORz   */
            case 2:     /* zXORz  */
            case 7:     /* zANDz  */
            case 9:     /* zORNz  */
            case 10:    /* zNOPz  */
            case 11:    /* zANDNz */
                pnColr32 = 0;
                lclpnColr32 = 0;
                break;

            case 8: /* zCLEARz */
                /* set all source bits to 0 */
                pnColr32 = 0;
                lclpnColr32 = 0;
                break;

            case 12:    /* zSETz */
            case 14:    /* zINVERTz */
                /* sets all source bits to 1 */
                pnColr32 = ~0;
                lclpnColr32 = ~0;
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
                            FillDrawer = &M32BD_DrawRectEntryImg32Bit;
                            CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                            done = NU_TRUE;
                        }
                    }
                
#endif  /* NO_REGION_CLIP */
                
                    if ( !done )
                    {
                        /* Draw the image */
                        M32BD_DrawRectEntryImg32Bit(blitRec);
                    }
                }
                nuResume(dstBmap);
            }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

            /* Call the function for post-processing task. */
            SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M32BD_DrawRectEntryImg32Bit
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
VOID M32BD_DrawRectEntryImg32Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    dstBgnByte = 4 * dRect.Xmin;
    dstNextRow = dstBmap->pixBytes - (4 * byteCntM1) - 4;

    srcNextRow = srcPixBytes - (4 * byteCntM1) - 4;
    srcBgnByte = 4 * sRect.Xmin;

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
*    M32BD_WriteImage1M32Bit
*
* DESCRIPTION
*
*    A special case optimization for 1->32Bit WriteImage.
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
VOID M32BD_WriteImage1M32Bit(blitRcd *blitRec)
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
        dstClass = blitRec->blitRop & 0x1f;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

        /* Call the function for pre-processing task. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

        M_PAUSE(dstBmap);

        /* background color */
        bkColr32 = blitRec->blitBack;

        /* foreground color */
        pnColr32 = blitRec->blitFore;

        /* Convert the color to target format. */
        COLOR_CONVERT(bkColr32, lclbkColr32);
        COLOR_CONVERT(pnColr32, lclpnColr32);

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
            optPtr = &M32BD_RepDest1M32Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M32BD_OXADest1M32Bit;
            break;

        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M32BF_SetDestMem32;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M32BD_NotDestSolid1M32Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M32BF_InvertDestMem32;
            break;

        /* Memory transparent */
        case 16:    /* xREPx   :           !0src         */
        case 17:    /* xORx    :       !0src OR  dst     */
        case 25:    /* xORNx   :     !0src OR  (NOT dst) */
        case 28:    /* xSETx   :           1's           */
            optPtr = &M32BD_SetTrans1M32Bit;
            break;

        case 18:    /* xXORx   :       !0src XOR dst     */
        case 27:    /* xANDNx  :     !0src AND (NOT dst) */
        case 30:    /* xINVERTx:        (NOT dst)        */
            optPtr = &M32BD_InvertTrans1M32Bit;
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
            pnColr32 = ~pnColr32;
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
            pnColr32 = 0;
            lclpnColr32 = 0;
            break;
        case 12:    /* zSETz */
        case 14:    /* zINVERTz */
            /* sets all source bits to 1 */
            pnColr32 = ~0;
            lclpnColr32 = ~0;
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
                        FillDrawer = &M32BD_DrawRectEntryImg1B32;
                        CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                        done = NU_TRUE;
                    }
                }
                
#endif  /* NO_REGION_CLIP */
                
                if( !done )
                {
                    /* Blit the image to the memory */
                    M32BD_DrawRectEntryImg1B32(blitRec);
                }
            }
        }
        nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        ShowCursor();
#endif
    }

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M32BD_DrawRectEntryImg1B32
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
VOID M32BD_DrawRectEntryImg1B32(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;


    dstBgnByte = 4 * dRect.Xmin;
    srcBgnByte = (sRect.Xmin >> 3);
    dstNextRow = dstBmap->pixBytes - (4 * byteCntM1) - 4;
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
*    M32BD_ReadImage32Bit
*
* DESCRIPTION
*
*    A special case optimization for ReadImage, 32 bit per pixel,
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
VOID M32BD_ReadImage32Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
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

    /* 32 bits per pixel  */
    dstImage->imBits    = 32;

    /* set to 0 if we start an even address and one if it is odd  */
    dstImage->imAlign = gblXmin & 1;
    dstImage->imWidth = gblXmax - gblXmin + 1;
    dstImage->imBytes = 4 * dstImage->imWidth;
    dstRowBytes = dstImage->imBytes;

    dstPtr = (UINT8 *)(((INT32 ) &dstImage->imData[0]) + (4 * dstImage->imAlign));

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
            byteCntM1 = 4 * (gblXmax - gblXmin + 1);
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
                    srcPtr = (UINT8 *) *(srcMapTable + gblYmin) + (4 * gblXmin);
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    do
                    {
                        for( count = 0; count < byteCntM1; count++)
                        {
                            /* By default the image being read will be made fully visible */
                            /* and hence the fourth byte will be 255 */
                            if(((count+1)%4) == 0)
                            {
                                *dstPtr++ = 255;
                                srcPtr++;
                            }
                            else
                            {
                                *dstPtr++ = *srcPtr++;
                            }
                        }

                        dstPtr += dstNextRow;
                        srcPtr += srcNextRow;

                    } while( --lineCntM1 >= 0 );
#else
                    do
                    {
                        for( count = 0; count < byteCntM1; count+=4)
                        {
                            UINT32 lclColr;

                            /* Convert the color to target format. */
                            COLOR_BACK_CONVERT(*((UINT32 *)srcPtr),lclColr);

                            *((UINT32 *)dstPtr) = lclColr;

                            dstPtr += 4;
                            srcPtr += 4;
                        }

                        dstPtr += dstNextRow;
                        srcPtr += srcNextRow;

                    } while( --lineCntM1 >= 0 );
#endif
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
*    M32BD_Transparency
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
VOID M32BD_Transparency(VOID)
{
    UINT8 *lclDstPtr, *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)

            /* for each UINT8 in the row */
            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)*lclSrcPtr*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;

            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)*lclSrcPtr*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;

            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)*lclSrcPtr*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;

            lclDstPtr++;
            lclSrcPtr++;
#else
            UINT32 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(*((UINT32 *)lclSrcPtr),lclColr);

            /* for each UINT8 in the row */
            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)(lclColr & 0xFF)*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;

            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)((lclColr >> 8) & 0xFF)*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;

            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)((lclColr >> 16) & 0xFF)*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;

            *lclDstPtr = (UINT8)((float)(*lclDstPtr)*(alpha_level)+
                         (float)((lclColr >> 24) & 0xFF)*(1.0-alpha_level));
            lclDstPtr++;
            lclSrcPtr++;
#endif
        }
        /* advance to next row */
        lclDstPtr += dstNextRow;
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M32BD_Text_Transparency
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
VOID M32BD_Text_Transparency(VOID)
{
    UINT8 *lclDstPtr, pnColrR, pnColrG, pnColrB;
    UINT8 *lclSrcPtr, bkColrR, bkColrG, bkColrB;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

#if (HARDWARE_ALPHA_SUPPORTED == NU_TRUE)
    UINT8 pnColrA = (UINT8)(lclpnColr32 >> 24);
    UINT8 bkColrA = (UINT8)(lclbkColr32 >> 24);
#endif

    /* set up local pointers */
    lclDstPtr = dstPtr;
    lclSrcPtr = srcPtr;

    /* set up local colors */
    pnColrB = (UINT8)((pnColr32 >> 16) & 0xff);
    pnColrG = (UINT8)((pnColr32 >> 8) & 0xff);
    pnColrR = (UINT8)(pnColr32  & 0xff);

    /* set up local colors */
    bkColrB = (UINT8)((pnColr32 >> 16) & 0xff);
    bkColrG = (UINT8)((pnColr32 >> 8) & 0xff);
    bkColrR = (UINT8)(pnColr32  & 0xff);

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
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)
                /* for each UINT8  in the row */
                *lclDstPtr = ((UINT8)((1-alpha_level) * pnColrB) + (UINT8)(alpha_level * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_level) * pnColrG) + (UINT8)(alpha_level * (*lclDstPtr)));
                lclDstPtr++;
                *lclDstPtr = ((UINT8)((1-alpha_level) * pnColrR) + (UINT8)(alpha_level * (*lclDstPtr)));
                lclDstPtr++;

                /* Skip the alpha byte. */
                lclDstPtr++;
#else
                /* for each UINT8  in the row */
                *lclDstPtr++ = pnColrB;
                *lclDstPtr++ = pnColrG;
                *lclDstPtr++ = pnColrR;
                *lclDstPtr++ = pnColrA;
#endif
            }
            else
            {
                if(set_back_color == NU_TRUE)
                {
#if (HARDWARE_ALPHA_SUPPORTED == NU_FALSE)
                    /* source is 0 */
                    *lclDstPtr = ((UINT8)((1-alpha_level) * bkColrB) + (UINT8)(alpha_level * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_level) * bkColrG) + (UINT8)(alpha_level * (*lclDstPtr)));
                    lclDstPtr++;
                    *lclDstPtr = ((UINT8)((1-alpha_level) * bkColrR) + (UINT8)(alpha_level * (*lclDstPtr)));
                    lclDstPtr++;

                    /* Skip the alpha byte. */
                    lclDstPtr++;
#else
                    /* source is 0 */
                    *lclDstPtr++ = bkColrB;
                    *lclDstPtr++ = bkColrG;
                    *lclDstPtr++ = bkColrR;
                    *lclDstPtr++ = bkColrA;
#endif
                }
                else
                {
                    lclDstPtr+=4;
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

#endif  /* GLOBAL_ALPHA_SUPPORT */

#endif /* #ifdef INCLUDE_32_BIT */

