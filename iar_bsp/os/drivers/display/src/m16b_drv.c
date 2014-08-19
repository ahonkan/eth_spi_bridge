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
*  m16b_drv.c
*
* DESCRIPTION
*
*  This file contains 16-bit memory primitive driver routines.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M16BD_BlitMonoToSelf16Bit
*  M16BD_BlitSelfToSelf16Bit
*  M16BD_DrawRectEntryBlit1M16Bit
*  M16BD_DrawRectEntryImg1B16
*  M16BD_DrawRectEntry_SetPixel16
*  M16BD_DrawRectEntryImg16Bit
*  M16BD_DrawRectEntryBlitMM16Bit
*  M16BD_GetPixelFor16Bit
*  M16BD_InvertTrans1M16Bit
*  M16BD_NotDestSolidM16Bit
*  M16BD_NotDestSolid1M16Bit
*  M16BD_OXADestM16Bit
*  M16BD_OXADest1M16Bit
*  M16BD_ReadImage16Bit
*  M16BD_RepDestM16Bit
*  M16BD_RepDest1M16Bit
*  M16BD_SetTrans1M16Bit
*  M16BD_SetPixelFor16Bit
*  M16BD_WriteImage16Bit
*  M16BD_WriteImage1M16Bit
*  M16BD_Transparency
*  M16BD_Text_Transparency
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

UINT32 lclpnColr16, lclbkColr16;

/* For storing the device mode. */
SIGNED Display_Dev_Mode;

/***************************************************************************
* FUNCTION
*
*    M16BD_BlitMonoToSelf16Bit
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
VOID M16BD_BlitMonoToSelf16Bit(blitRcd *blitRec)
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
    bkColr16 = blitRec->blitBack;

    /* foreground color */
    pnColr16 = blitRec->blitFore;

    /* Convert the color. */
    COLOR_CONVERT(pnColr16,lclpnColr16);
    COLOR_CONVERT(bkColr16,lclbkColr16);

    dstBmap  =  blitRec->blitDmap;

    dstClass = blitRec->blitRop & 0x20;

    if (!dstClass)
    {
        dstClass = blitRec->blitRop & 0x1f;
    }

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    /* Call pre-processing function. */
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
        optPtr = &M16BD_RepDest1M16Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M16BD_OXADest1M16Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M16BF_SetDestMem16;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M16BD_NotDestSolid1M16Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M16BF_InvertDestMem16;
        break;

    /* Memory transparent */
    case 16:    /* xREPx   :           !0src         */
    case 17:    /* xORx    :       !0src OR  dst     */
    case 25:    /* xORNx   :     !0src OR  (NOT dst) */
    case 28:    /* xSETx   :           1's           */
        optPtr = &M16BD_SetTrans1M16Bit;
        break;

    case 18:    /* xXORx   :       !0src XOR dst     */
    case 27:    /* xANDNx  :     !0src AND (NOT dst) */
    case 30:    /* xINVERTx:        (NOT dst)        */
        optPtr = &M16BD_InvertTrans1M16Bit;
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

    case 8:     /* zCLEARz */
        /* set all source bits to 0 */
        bkColr16 = 0;
        pnColr16 = 0;
        lclbkColr16 = 0;
        lclpnColr16 = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* set all source bits to 1 */
        bkColr16 = ~0;
        pnColr16 = ~0;
        lclbkColr16 = ~0;
        lclpnColr16 = ~0;
        break;

    case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
        /* set all source bits to 1 */
        bkColr16 = 0;
        pnColr16 = 0;
        lclbkColr16 = 0;
        lclpnColr16 = 0;

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

            /* do we need to worry about clipping? */
            if( clipToRectFlag != 0 )
            {
                /* yes, do trivial reject */
                if( dRect.Xmin < clipR.Xmin)
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
                    FillDrawer = &M16BD_DrawRectEntryBlit1M16Bit;
                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M16BD_DrawRectEntryBlit1M16Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif
}

/***************************************************************************
* FUNCTION
*
*    M16BD_DrawRectEntryBlit1M16Bit
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
VOID M16BD_DrawRectEntryBlit1M16Bit(blitRcd *blitRec)
{

#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    srcBgnByte = sRect.Xmin >> 3;
    dstBgnByte = dRect.Xmin;
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;

    srcNextRow = srcPixBytes - (sRect.Xmax >> 3) + srcBgnByte;

#ifndef SMART_LCD
    dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;

    dstPtr16 = (UINT16 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                    + dstBgnByte;
#endif /* SMART_LCD */
    srcPtr   = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                    + srcBgnByte;

    /* blit the rectangle */
    optPtr();
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
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
*    M16BD_NotDestSolid1M16Bit
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
VOID M16BD_NotDestSolid1M16Bit(VOID)
{
    /* invert the destination */
    M16BF_InvertDestMem16();

    /* fill this rectangle */
    M16BD_OXADest1M16Bit();
}

/***************************************************************************
* FUNCTION
*
*    M16BD_RepDest1M16Bit
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr if the source is 1 else
*    bkColr16.
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
VOID M16BD_RepDest1M16Bit(VOID)
{
#ifndef SMART_LCD
    UINT16 *lclDstPtr;
#endif /* !SMART_LCD */
    UINT8  *lclSrcPtr;
    INT32  lclLineCnt, lclByteCnt;
    INT32  pxBit;
    INT32  pxBitVal;

#ifndef SMART_LCD
    lclDstPtr  = dstPtr16;
#endif /* !SMART_LCD */
    lclSrcPtr  = srcPtr;
    lclLineCnt = lineCntM1;

    pxBitVal = 128 >> (sRect.Xmin & 7);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        pxBit = pxBitVal;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
#ifdef SMART_LCD
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                *lclDstPtr = (UINT16)lclpnColr16;
#endif /* SMART_LCD */
            }
#ifndef SMART_LCD
            else
            {
                /* source is 0 */
                if(set_back_color == NU_TRUE)
                {
                    *lclDstPtr = (UINT16)lclbkColr16;
                }
            }

            lclDstPtr++;
#endif /* SMART_LCD */

            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }
#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BD_SetTrans1M16Bit
*
* DESCRIPTION
*
*    Sets the destination pixel data to pnColr16 if the source is 1 else it
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
VOID M16BD_SetTrans1M16Bit(VOID)
{
#ifndef SMART_LCD
    UINT16 *lclDstPtr;
#endif /* !SMART_LCD */
    UINT8  *lclSrcPtr;
    INT32  lclLineCnt, lclByteCnt;
    INT32  pxBit;
    INT32  pxBitVal;

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr  = dstPtr16;
#endif /* !SMART_LCD */
    lclSrcPtr  = srcPtr;
    lclLineCnt = lineCntM1;
    pxBitVal   = 128 >> (sRect.Xmin & 7);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        pxBit = pxBitVal;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit )
            {
                /* source is 1 */
#ifdef SMART_LCD
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                *lclDstPtr = lclpnColr16;
#endif /* SMART_LCD */
            }

#ifndef SMART_LCD
            lclDstPtr++;
#endif /* SMART_LCD */
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }
#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BD_InvertTrans1M16Bit
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
VOID M16BD_InvertTrans1M16Bit(VOID)
{
#ifndef SMART_LCD
    UINT16 *lclDstPtr;
#endif /* !SMART_LCD */
    UINT8  *lclSrcPtr;
    INT32  lclLineCnt, lclByteCnt;
    INT32  pxBit;
    INT32  pxBitVal;
#ifdef SMART_LCD
    UINT32 pixelValue;
#else /* SMART_LCD */
    UINT16 pixlVal;
#endif /* SMART_LCD */

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr  = dstPtr16;
#endif /* !SMART_LCD */
    lclSrcPtr  = srcPtr;
    lclLineCnt = lineCntM1;
    pxBitVal   = 128 >> (sRect.Xmin & 7);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        pxBit      = pxBitVal;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( *lclSrcPtr & pxBit)
            {
                /* source is 1 */
#ifdef SMART_LCD
                /* Call the function to get the pixel. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = ~pixelValue;
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                pixlVal    = *lclDstPtr;
                *lclDstPtr = ~pixlVal;
#endif /* SMART_LCD */
            }
#ifndef SMART_LCD
            lclDstPtr++;
#endif  /* SMART_LCD */
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

        /* advance to next row */
#ifndef SMART_LCD
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BD_OXADest1M16Bit
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
VOID M16BD_OXADest1M16Bit(VOID)
{
#ifndef SMART_LCD
    UINT16 *lclDstPtr;
#endif /* !SMART_LCD */
    UINT8  *lclSrcPtr;
    INT32  lclLineCnt, lclByteCnt;
    INT32  logFnc;
    INT32  pxBit;
    INT32  pxBitVal;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr = dstPtr16;
#endif /* !SMART_LCD */
    lclSrcPtr = srcPtr;

    lclLineCnt = lineCntM1;

    /* only two lower bits needed */
    logFnc   = (dstClass & 3);
    pxBitVal = 128 >> (sRect.Xmin & 7);

    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        lclByteCnt = byteCntM1;
        pxBit      = pxBitVal;

        while( lclByteCnt-- >= 0 )
        {
            /* for each UINT8 in the row */
            if( !(*lclSrcPtr & pxBit) )
            {
                /* source is 0 */
#ifdef SMART_LCD
                /* Call the function to get the pixel. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                if( (logFnc == 3) || (pixelValue == 0) )
#else /* SMART_LCD */
                if( (logFnc == 3) || (*lclDstPtr == 0) )
#endif /* SMART_LCD */
                {
                    /* "AND" or 0,0 */
#ifdef SMART_LCD
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclbkColr16);
#else /* SMART_LCD */
                    *lclDstPtr = lclbkColr16;
#endif /* SMART_LCD */
                }
                else
                {
                    /* "OR","XOR" with 0,1 */
#ifdef SMART_LCD
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                    *lclDstPtr = lclpnColr16;
#endif /* SMART_LCD */
                }
            }
            else
            {
                /* source is 1 */
                if( logFnc == 1 )
                {
                    /* "OR" with 1,X */
#ifdef SMART_LCD
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                    *lclDstPtr = lclpnColr16;
#endif /* SMART_LCD */
                }
                else
                {
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
#endif /* SMART_LCD */
                    if( logFnc == 3 )
                    {
                        /* "AND" */
#ifdef SMART_LCD
                        if (pixelValue)
#else /* SMART_LCD */
                        if (*lclDstPtr )
#endif /* SMART_LCD */
                        {
                            /* "AND", with 1,1 */
#ifdef SMART_LCD
                            /* Call the function to set the pixel. */
                            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                            *lclDstPtr = lclpnColr16;
#endif /* SMART_LCD */
                        }
                        else
                        {
                            /* "AND", with 1,0 */
#ifdef SMART_LCD
                            /* Call the function to set the pixel. */
                            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclbkColr16);
#else /* SMART_LCD */
                            *lclDstPtr = lclbkColr16;
#endif /* SMART_LCD */
                        }
                    }
                    else
                    {
                        /* "XOR" */
#ifdef SMART_LCD
                        if (pixelValue)
#else /* SMART_LCD */
                        if (*lclDstPtr )
#endif /* SMART_LCD */
                        {
                            /* "XOR", with 1,1 */
#ifdef SMART_LCD
                            /* Call the function to set the pixel. */
                            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclbkColr16);
#else /* SMART_LCD */
                            *lclDstPtr = lclbkColr16;
#endif /* SMART_LCD */
                        }
                        else
                        {
                            /* "XOR", with 1,0 */
#ifdef SMART_LCD
                            /* Call the function to set the pixel. */
                            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)lclpnColr16);
#else /* SMART_LCD */
                            *lclDstPtr = lclpnColr16;
#endif /* SMART_LCD */
                        }
                    }
                }
            }

#ifndef SMART_LCD
            lclDstPtr++;
#endif /* SMART_LCD */
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

#ifndef SMART_LCD
        /* advance to next row */
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
        lclSrcPtr += srcNextRow;
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BD_BlitSelfToSelf16Bit
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
VOID M16BD_BlitSelfToSelf16Bit(blitRcd *blitRec)
{
    rect    clipR;
    rect    *rListPtr;
    INT32   blitMayOverlap = NU_FALSE;
    INT32   isLine         = NU_FALSE;
    INT16   done           = NU_FALSE;

    rectCnt     =  blitRec->blitCnt;
    rListPtr    =  (rect *) blitRec->blitList;
    srcBmap     =  blitRec->blitSmap;
    srcPixBytes = srcBmap->pixBytes;

    /* pause the source */
    M_PAUSE(srcBmap);

    /* background color */
    bkColr16 = blitRec->blitBack;

    /* foreground color */
    pnColr16 = blitRec->blitFore;

    /* Convert the color to target format. */
    COLOR_CONVERT(pnColr16, lclpnColr16);
    COLOR_CONVERT(bkColr16, lclbkColr16);

    dstBmap =  blitRec->blitDmap;
    dstClass = blitRec->blitRop & 0x0f;

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
        optPtr = &M16BD_RepDestM16Bit;
        break;

    case 1:     /* zORz    :       src OR  dst       */
    case 2:     /* zXORz   :       src XOR dst       */
    case 3:     /* zNANDz  :    (NOT src) AND dst    */
    case 5:     /* zNORz   :    (NOT src) OR  dst    */
    case 6:     /* zNXORz  :    (NOT src) XOR dst    */
    case 7:     /* zANDz   :       src AND dst       */
        optPtr = &M16BD_OXADestM16Bit;
        break;

    case 8:     /* zCLEARz :           0's           */
    case 12:    /* zSETz   :           1's           */
        optPtr = &M16BF_SetDestMem16;
        break;

    case 9:     /* zORNz   :     src OR  (NOT dst)   */
    case 11:    /* zANDNz  :     src AND (NOT dst)   */
    case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
    case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
        optPtr = &M16BD_NotDestSolidM16Bit;
        break;

    case 10:    /* zNOPz   :           dst <NOP>     */
        optPtr = &SCREENS_Nop;
        break;

    case 14:    /* zINVERTz:        (NOT dst)        */
        optPtr = &M16BF_InvertDestMem16;
        break;
    case 32:    /* xAVGx */

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
        pnColr16 = ~0;
        lclpnColr16 = ~0;
        break;

    case 0:     /* zREPz  */
    case 1:     /* zORz   */
    case 2:     /* zXORz  */
    case 7:     /* zANDz  */
    case 9:     /* zORNz  */
    case 10:    /* zNOPz  */
    case 11:    /* zANDNz */
        pnColr16 = 0;
        lclpnColr16 = 0;
        break;

    case 8:     /* zCLEARz */
        /* set all source bits to 0 */
        pnColr16 = 0;
        lclpnColr16 = 0;
        break;

    case 12:    /* zSETz */
    case 14:    /* zINVERTz */
        /* sets all source bits to 1 */
        pnColr16 = ~0;
        lclpnColr16 = ~0;
        break;

    case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
        /* set all source bits to 1 */
        bkColr16 = 0;
        lclbkColr16 = 0;

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

                if( dRect.Xmin >= dRect.Xmax )
                {
                    continue;
                }

#ifndef NO_REGION_CLIP
                
                /* do we need to worry about region clipping? */
                if( clipToRegionFlag != 0 )
                {
                    /* yes */
                    FillDrawer = &M16BD_DrawRectEntryBlitMM16Bit;
                    if( CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect) == 0 )
                    {
                        break;
                    }
                    continue;
                }
                
#endif  /* NO_REGION_CLIP */
                
            }

            /* blit the rectangle */
            M16BD_DrawRectEntryBlitMM16Bit(blitRec);
        }
    }
    nuResume(srcBmap);
    nuResume(dstBmap);

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif
}

/***************************************************************************
* FUNCTION
*
*    M16BD_DrawRectEntryBlitMM16Bit
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
VOID M16BD_DrawRectEntryBlitMM16Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* Store the device mode. */
    Display_Dev_Mode = blitRec->blitSmap->devMode;

    /* Check if the destination bitmap points to user memory. */
    if (dstBmap->devMode == cMEMORY || dstBmap->devMode == cUSER)
    {
#if (BPP == 24)
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = (BPP / 8) * sRect.Xmin;
        dstBgnByte = (BPP / 8) * dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = srcPixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8);
        dstNextRow = dstBmap->pixBytes - 
		             ((BPP / 8) * byteCntM1) - (
					 BPP / 8);
#else
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = ((BPP / 8) * 
		             sRect.Xmin) >> 1;
        dstBgnByte = ((BPP / 8) * 
		             dRect.Xmin) >> 1;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = (srcPixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8)) >> 1;
        dstNextRow = (dstBmap->pixBytes - 
		             ((BPP / 8) * byteCntM1) - 
					 (BPP / 8)) >> 1;
#endif
    }

    else
    {
#if (BPP == 16)
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = sRect.Xmin;
        dstBgnByte = dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = (srcPixBytes >> 1) - byteCntM1 - 1;
        dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;
#elif (BPP == 24)
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = (BPP / 8) * sRect.Xmin;
        dstBgnByte = (dRect.Xmin << 1);
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = srcPixBytes - 
		((BPP / 8) * byteCntM1) - 
		(BPP / 8);
        dstNextRow = (dstBmap->pixBytes - (byteCntM1 << 1) - 2);
#elif (BPP == 32)
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = sRect.Xmin << 1;
        dstBgnByte = dRect.Xmin;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = (srcPixBytes - (byteCntM1 << 2) - 4) >> 1;
        dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;
#endif
    }

#ifndef SMART_LCD
    dstPtr16   = (UINT16 *)(*(dstBmap->mapTable[0] + dRect.Ymin))
                          + dstBgnByte;
#endif /* SMART_LCD */
    srcPtr16   = (UINT16 *)(*(srcBmap->mapTable[0] + sRect.Ymin))
                          + srcBgnByte;

    dRect.Ymax--;
    sRect.Ymax--;

    /* blit the rectangle */
    optPtr();
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

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
*    M16BD_NotDestSolidM16Bit
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
VOID M16BD_NotDestSolidM16Bit(VOID)
{
    /* invert the destination */
    M16BF_InvertDestMem16();

    /* fill this rectangle */
    M16BD_OXADestM16Bit();
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
VOID M16BD_RepDestM16Bit(VOID)
{
	/* Use register variables for improved performance */
    R1 INT32  lclLineCnt, lclByteCnt;

#if (BPP == 24)
    UINT8 *lclSrcPtr, *lclDstPtr;
#else
#ifdef SMART_LCD
    R1 UINT16 *lclSrcPtr;
#else /* SMART_LCD */
    R1 UINT16 *lclSrcPtr, *lclDstPtr;
#ifdef LCD_OPTIMIZE_BLIT
    R1 UINT8  *lclSrcPtr8, *lclDstPtr8;
#endif
#endif /* SMART_LCD */
#endif

#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */
    if( ( dstBmap == srcBmap)
        &&
        ( (sRect.Ymin < dRect.Ymin)
          ||
          ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin) )
        )
      )
    {
#if (BPP == 24)
#ifndef SMART_LCD
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                    + (dRect.Xmax * (BPP/8));
#endif /* SMART_LCD */
        lclSrcPtr = (UINT8 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                    + (sRect.Xmax * (BPP/8));
#else
#ifndef SMART_LCD
        /* blit bottom to top, right to left */
        lclDstPtr = (UINT16 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
                    + dRect.Xmax;
#endif /* SMART_LCD */
        lclSrcPtr = (UINT16 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
                    + sRect.Xmax;
#endif

        lclLineCnt = lineCntM1;
        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {

#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
#ifdef SMART_LCD
                /* Call the function to get the pixel. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                /* for each UINT8 in the row */
                pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr--;
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                /* for each UINT8 in the row */
                *lclDstPtr-- = (UINT16)lclpnColr16 ^ *lclSrcPtr--;
#endif /* SMART_LCD */
#else
                UINT32 lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

#ifdef SMART_LCD
                /* Call the function to get the pixel. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                /* for each UINT8 in the row */
                pixelValue = (UINT16)lclpnColr16 ^ lclSrcColr;
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
                lclSrcPtr--;

#else /* SMART_LCD */

                *lclDstPtr-- = (UINT16)lclpnColr16 ^ lclSrcColr;
                lclSrcPtr--;
#endif /* SMART_LCD */
#endif      /* (COLOR_CONVERSION_NEEDED == NU_FALSE) */
            }

#ifndef SMART_LCD
            /* advance to next row */
            lclDstPtr -= dstNextRow;
#endif /* SMART_LCD */
            lclSrcPtr -= srcNextRow;
        }
    }
    else
    {
#if (BPP == 24)
        /* blit top to bottom, left to right */
        /* set up local pointers */

#ifndef SMART_LCD
        lclDstPtr = (UINT8 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                    + dstBgnByte;
#endif /* SMART_LCD */
        if (Display_Dev_Mode == 0x10 || Display_Dev_Mode == 0x11)
        {
            lclSrcPtr   = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes))
                           + srcBgnByte;
        }
        else
        {
            lclSrcPtr = (UINT8 *) (*(srcBmap->mapTable[0] + sRect.Ymin))
                         + srcBgnByte;
        }
#else
        /* blit top to bottom, left to right */
        /* set up local pointers */
#ifndef SMART_LCD
        lclDstPtr = dstPtr16;
#endif /* SMART_LCD */
        lclSrcPtr = srcPtr16;
#endif
        lclLineCnt = lineCntM1;

        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;

#ifdef LCD_OPTIMIZE_BLIT

            lclByteCnt += 1;

            /* Both buffers are 4 byte aligned */
            if( ESAL_GE_MEM_ALIGNED_CHECK(lclDstPtr, 4) && ESAL_GE_MEM_ALIGNED_CHECK(lclSrcPtr, 4) &&
              (lclByteCnt > 1) )
            {
            	while(lclByteCnt > 1)
            	{
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    /* for each UINT8 in the row */
                    *((UINT32 *)lclDstPtr) = *((UINT32 *)lclSrcPtr);
#else /* COLOR_CONVERSION_NEEDED */
                    UINT32 lclSrcColr1, lclSrcColr2;

                    /* Convert the color to target format. */
                    COLOR_CONVERT(*lclSrcPtr, lclSrcColr1);
                    lclSrcColr2 = lclSrcColr1 << 16;
                    COLOR_CONVERT(*(lclSrcPtr + 1), lclSrcColr1);
                    lclSrcColr2 |= lclSrcColr1;

                    /* for each UINT8 in the row */
                    *((UINT32 *)lclDstPtr) = lclSrcColr2;
#endif /* COLOR_CONVERSION_NEEDED */

                    lclDstPtr += 2;
                    lclSrcPtr += 2;
                    lclByteCnt -= 2;
                }
                if ( lclByteCnt == 1 )
                {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    /* for each UINT16 in the row */
                    *lclDstPtr++ = *lclSrcPtr++;
#else /* COLOR_CONVERSION_NEEDED */
                    UINT32 lclSrcColr;

                    /* Convert the color to target format. */
                    COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

                    /* for each UINT16 in the row */
                    *lclDstPtr++ = lclSrcColr;
                    lclSrcPtr++;
#endif /* COLOR_CONVERSION_NEEDED */
                }
            }
            /* Both buffers are 2 byte aligned */
            else if( ESAL_GE_MEM_ALIGNED_CHECK(lclDstPtr, 2) && ESAL_GE_MEM_ALIGNED_CHECK(lclSrcPtr, 2) )
            {
            	while(lclByteCnt > 0)
                {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    /* for each UINT16 in the row */
                    *lclDstPtr++ = *lclSrcPtr++;
#else /* COLOR_CONVERSION_NEEDED */
                    UINT32 lclSrcColr;

                    /* Convert the color to target format. */
                    COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

                    /* for each UINT16 in the row */
                    *lclDstPtr++ = lclSrcColr;
                    lclSrcPtr++;
#endif /* COLOR_CONVERSION_NEEDED */
                    lclByteCnt--;
                }
            }
            /* Buffers are unaligned
             * Each byte will be copied individually */
            else
            {
                /* Convert to byte pointer */
                lclDstPtr8 = (UINT8*)lclDstPtr;
                lclSrcPtr8 = (UINT8*)lclSrcPtr;

                while(lclByteCnt > 0)
                {
#if (COLOR_CONVERSION_NEEDED == NU_FALSE)
                    /* for each UINT8 in the row */
                    *((UINT8 *)lclDstPtr8++)= *((UINT8 *)lclSrcPtr8++);
                    *((UINT8 *)lclDstPtr8++)= *((UINT8 *)lclSrcPtr8++);
#else /* COLOR_CONVERSION_NEEDED */
                    UINT32 lclSrcColr;
                    UINT8  *lclSrcColr8 = &lclSrcColr;

                    /* Convert the color to target format. */
                    COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
                    /* for each UINT8 in the row */
                    *lclDstPtr8++ = lclSrcColr8[2];
                    *lclDstPtr8++ = lclSrcColr8[3];
#else
                    /* for each UINT8 in the row */
                    *lclDstPtr8++ = lclSrcColr8[0];
                    *lclDstPtr8++ = lclSrcColr8[1];
#endif
                    lclSrcPtr++;
#endif /* COLOR_CONVERSION_NEEDED */
                    lclByteCnt--;
                }

                lclDstPtr = (UINT16*)lclDstPtr8;
                lclSrcPtr = (UINT16*)lclSrcPtr8;
            }

#else /* LCD_OPTIMIZE_BLIT */

            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
#ifdef SMART_LCD
                /* for each UINT8 in the row */
                pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                /* for each UINT8 in the row */
                *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
#endif /* SMART_LCD */
#else
                /* Check if the destination bitmap points to user memory. */
                if (dstBmap->devMode == cMEMORY || dstBmap->devMode == cUSER)
                {
#if (BPP == 16)
#ifdef SMART_LCD
                    /* for each UINT8 in the row */
                    pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    /* for each UINT8 in the row */
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
#endif /* SMART_LCD */
#elif (BPP == 24)
#ifdef SMART_LCD
                    /* for each UINT8 in the row */
                    pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt  - 1 + 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt  - 1 + 2), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    /* for each UINT8 in the row */
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
#endif /* SMART_LCD */

#elif (BPP == 32)
#ifdef SMART_LCD
                    /* for each UINT8 in the row */
                    pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1 + 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    /* for each UINT8 in the row */
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
#endif /* SMART_LCD */
#endif
                }

                else
                {
                    UINT32 lclSrcColr;

#if (BPP == 16)
                    /* Convert the color to target format. */
                    COLOR_CONVERT(*lclSrcPtr, lclSrcColr);
#ifdef SMART_LCD
                    pixelValue = (UINT16)lclpnColr16 ^ lclSrcColr;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ lclSrcColr;
#endif /* SMART_LCD */
                    lclSrcPtr++;

#elif (BPP == 24)
                    /* Convert the color to target format. */
                    COLOR_CONVERT( (*lclSrcPtr | (*(lclSrcPtr+1) << 8) | (*(lclSrcPtr+2) << 16)) , lclSrcColr);
                    lclSrcPtr+=3;
#ifdef SMART_LCD
                    pixelValue = (UINT16)lclpnColr16 ^ (lclSrcColr & 0xFFFF);
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    *((UINT16 *)lclDstPtr) = (UINT16)lclpnColr16 ^ (lclSrcColr & 0xFFFF);
                    lclDstPtr+=2;
#endif /* SMART_LCD */

#elif (BPP == 32)
                    /* Convert the color to target format. */
                    COLOR_CONVERT((*lclSrcPtr | (*(lclSrcPtr+1) << 16)), lclSrcColr);
#ifdef SMART_LCD
                    pixelValue = (UINT16)lclpnColr16 ^ lclSrcColr;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    *lclDstPtr++ = (UINT16)lclpnColr16 ^ lclSrcColr;
#endif /* SMART_LCD */
                    lclSrcPtr+=2;
#endif
                }
#endif
            }

#endif /* LCD_OPTIMIZE_BLIT */

            /* advance to next row */
#ifndef SMART_LCD
            lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
            lclSrcPtr += srcNextRow;
        }
    }

}

/***************************************************************************
* FUNCTION
*
*    M16BD_OXADestM16Bit
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
VOID M16BD_OXADestM16Bit(VOID)
{
    UINT16 *lclSrcPtr;
    INT32  lclLineCnt, lclByteCnt;
    INT32  logFnc, locColr;
#ifdef SMART_LCD
    UINT32 pixelValue;
#else /* SMART_LCD */
    UINT16 *lclDstPtr;
#endif /* SMART_LCD */

    if( (dstBmap == srcBmap)
        &&
        ( (sRect.Ymin < dRect.Ymin)
          ||
        ( (sRect.Ymin == dRect.Ymin) && (sRect.Xmin < dRect.Xmin)) )
        )
    {
        /* blit bottom to top, right to left */
#ifndef SMART_LCD
        lclDstPtr = (UINT16 *) (*(dstBmap->mapTable[0] + dRect.Ymax))
            + dRect.Xmax;
#endif /* SMART_LCD */
        lclSrcPtr = (UINT16 *) (*(dstBmap->mapTable[0] + sRect.Ymax))
            + sRect.Xmax;
        lclLineCnt = lineCntM1;

        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;

            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                locColr = lclpnColr16 ^ *lclSrcPtr--;
#else
                UINT32 lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

                locColr = lclpnColr16 ^ lclSrcColr;
                lclSrcPtr--;
#endif

                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = locColr | pixelValue;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = locColr;
#endif /* SMART_LCD */
                    break;

                case 2: /* "XOR" */
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = locColr ^ pixelValue;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = locColr;
#endif /* SMART_LCD */
                    break;

                case 3: /* "AND" */
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = locColr & pixelValue;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = locColr;
#endif /* SMART_LCD */
                    break;
                }

#ifndef SMART_LCD
               lclDstPtr--;
#endif /* SMART_LCD */
            }

            /* advance to next row */
#ifndef SMART_LCD
            lclDstPtr -= dstNextRow;
#endif /* SMART_LCD */
            lclSrcPtr -= srcNextRow;
        }
    }
    else
    {
        /* blit top to bottom, left to right */
        /* set up local pointers */
#ifndef SMART_LCD
        lclDstPtr  = dstPtr16;
#endif /* SMART_LCD */
        lclSrcPtr  = srcPtr16;
        lclLineCnt = lineCntM1;

        /* only two lower bits needed */
        logFnc = (dstClass & 3);

        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            lclByteCnt = byteCntM1;

            while( lclByteCnt-- >= 0 )
            {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                /* for each UINT8 in the row */
                locColr = (UINT16)lclpnColr16 ^ *lclSrcPtr++;
#else
                UINT32 lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

                locColr = (UINT16)lclpnColr16 ^ lclSrcColr;
                lclSrcPtr++;
#endif

                switch (logFnc)
                {
                case 0: /* can't happen */
                case 1: /* "OR" */
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = locColr | pixelValue;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    locColr = locColr | *lclDstPtr;
                    *lclDstPtr = locColr;
#endif /* SMART_LCD */
                    break;

                case 2: /* "XOR" */
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = locColr ^ pixelValue;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    locColr = locColr ^ *lclDstPtr;
                    *lclDstPtr = locColr;
#endif /* SMART_LCD */
                    break;

                case 3: /* "AND" */
#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    /* for each UINT8 in the row */
                    pixelValue = locColr & pixelValue;
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), (UINT16)pixelValue);
#else /* SMART_LCD */
                    locColr = locColr & *lclDstPtr;
                    *lclDstPtr = locColr;
#endif /* SMART_LCD */
                    break;
                }
#ifndef SMART_LCD
                lclDstPtr++;
#endif /* SMART_LCD */
            }

            /* advance to next row */
#ifndef SMART_LCD
            lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
            lclSrcPtr += srcNextRow;
        }
    }
   
}

#endif  /* ((!defined(NO_BLIT_SUPPORT)) || (!defined(NO_IMAGE_SUPPORT))) */

/***************************************************************************
* FUNCTION
*
*    M16BD_GetPixelFor16Bit
*
* DESCRIPTION
*
*     Special case optimization for GetPixel, 16-bit per pixel memory source.
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
INT32 M16BD_GetPixelFor16Bit(INT32 gblX, INT32 gblY, grafMap *getpBmap)
{
#ifndef SMART_LCD
    UINT16 *pixelPtr;
#endif /* SMART_LCD */
    INT32  pixelValue = -1;

#ifdef SMART_LCD
    /* check if off bitmap */
    if((gblX < 0) || (gblY < 0))
    {
        pixelValue = 0;
    }

#else /* SMART_LCD */
    /* check if off bitmap */
    if( (gblX < 0) || (gblY < 0) || (gblX >= getpBmap->pixWidth)
        || (gblY >= getpBmap->pixHeight) )
    {
        pixelValue = 0;
    }
#endif /* SMART_LCD */

    if(pixelValue != 0)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(gblX, gblY, (UINT32*)(&pixelValue));
#else /* SMART_LCD */
        /* lock grafMap */
        M_PAUSE(getpBmap);

        if( (gblY < getpBmap->mapWinYmin[0]) || (gblY > getpBmap->mapWinYmax[0]) )
        {
            getpBmap->mapBankMgr(getpBmap, gblY, -1);
        }

        pixelPtr = (UINT16 *) (*(getpBmap->mapTable[0] + gblY)) + gblX;
        pixelValue = *pixelPtr;

        /* Convert the color to source format. */
        COLOR_BACK_CONVERT(pixelValue, pixelValue);

        nuResume(getpBmap);
#endif /* SMART_LCD */
    }

    return(pixelValue);
}


/***************************************************************************
* FUNCTION
*
*    M16BD_SetPixelFor16Bit
*
* DESCRIPTION
*
*    A special case optimization for SetPixel,16-bit-per-pixel memory
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
VOID M16BD_SetPixelFor16Bit(blitRcd *setpRec )
{
    point  *drawPt;
    INT32  blitMayOverlap = NU_FALSE;
    INT32  isLine         = NU_FALSE;
    INT16  done           = NU_FALSE;
#ifdef SMART_LCD
    UINT16 pColor;
#endif /* SMART_LCD */

    /* set up the rectangular/region clip info */
    if( CLIP_Set_Up_Clip(setpRec, &cRect, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* set up pointer */
        drawPt = (point *) setpRec->blitList;

#ifdef SMART_LCD
        /* pattern 0? */
        if( setpRec->blitPat == 0 )
        {
            /* yes, use background color */
            if( setpRec->blitRop & 0x10 )
            {
                /* done if transparent */
                done = NU_TRUE;
            }

            if( !done )
            {
                pColor = (UINT16) setpRec->blitBack;
            }
        }
        else
        {
            /* no, use foreground color */
            pColor = (UINT16) setpRec->blitFore;
        }

        /* Convert the color to target platform. */
        COLOR_CONVERT(pColor, pColor);

        /* Call the function to set the pixel. */
        SCREENI_Display_Device.display_set_pixel_hook(drawPt->X, drawPt->Y, (UINT16)pColor);

#else /* SMART_LCD */

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
            M16BD_DrawRectEntry_SetPixel16(setpRec);
        }
        else
        {
            /* yes first check trivial reject */
            if( (dRect.Xmin >= cRect.Xmin) && (dRect.Ymin >= cRect.Ymin) &&
                (dRect.Xmin < cRect.Xmax) && (dRect.Ymin < cRect.Ymax))
            {
                
#ifndef NO_REGION_CLIP
                
                /* it passes so far - do we need to worry about region clipping? */
                if( clipToRegionFlag == 0 )
                
#endif  /* NO_REGION_CLIP */
                
                {
                    /* no, draw the point */
                    M16BD_DrawRectEntry_SetPixel16(setpRec);
                }
                
#ifndef NO_REGION_CLIP
                
                else
                {
                    /* yes, clip to the region and draw */
                    dRect.Xmax = dRect.Xmin + 1;
                    dRect.Ymax = dRect.Ymin + 1;
                    FillDrawer = &M16BD_DrawRectEntry_SetPixel16;
                    CLIP_Fill_Clip_Region(setpRec, &dRect);
                }
                
#endif  /* NO_REGION_CLIP */
                
            }
        }

        nuResume(setpRec->blitDmap);
#endif /* SMART_LCD */

    }
}

/***************************************************************************
* FUNCTION
*
*    M16BD_DrawRectEntry_SetPixel16
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
VOID M16BD_DrawRectEntry_SetPixel16(blitRcd *drwPRec)
{
    grafMap *drwGmap;
    INT32  *rowTable;
    UINT16  *wordPtr;
    UINT16   pColor;
    INT16   done = NU_FALSE;

#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* get grafMap */
    drwGmap = drwPRec->blitDmap;
    if( (dRect.Ymin < drwGmap->mapWinYmin[0])
        ||
        (dRect.Ymin > drwGmap->mapWinYmax[0]) )
    {
        /* yes, map in bank */
        drwGmap->mapBankMgr(drwGmap, dRect.Ymin, -1, 0);
    }

    /* point to row table */
    rowTable = (INT32 *) drwGmap->mapTable[0];

    /* look up the starting row */
    rowTable = rowTable + dRect.Ymin;

    /* point to pixel */
    wordPtr = (UINT16 *) ((*rowTable) + (2 * dRect.Xmin));

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
            pColor = (UINT16) drwPRec->blitBack;
        }
    }
    else
    {
        /* no, use foreground color */
        pColor = (UINT16) drwPRec->blitFore;
    }

    /* Convert the color to target platform. */
    COLOR_CONVERT(pColor, pColor);

    if( !done )
    {
        /* draw the pixel */
        *wordPtr = pColor;
    }
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef USING_DIRECT_X
    BlitBacktoFront(&scrnRect);
#endif
}

/***************************************************************************
* FUNCTION
*
*    M16BD_WriteImage16Bit
*
* DESCRIPTION
*
*    A special case optimization for 16 Bit WriteImage, interleaved or linear.
*    Also passes through M16BD_WriteImage1M16Bit.
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
VOID M16BD_WriteImage16Bit(blitRcd *blitRec)
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
        M16BD_WriteImage1M16Bit(blitRec);
        done = NU_TRUE;
    }

    if( !done )
    {
        if( (srcImag->imWidth <= 0) || (srcImag->imHeight <= 0) )
        {
            done = NU_TRUE;
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */
        }
        if( !done )
        {
            sRect.Ymin = 0;
            sRect.Xmin = srcImag->imAlign;
            srcPixBytes = srcImag->imBytes;
            rListPtr = (rect *) blitRec->blitList;

            dstBmap  = blitRec->blitDmap;
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
                optPtr = &M16BD_RepDestM16Bit;
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 2:     /* zXORz   :       src XOR dst       */
            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M16BD_OXADestM16Bit;
                break;

            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                optPtr = &M16BF_SetDestMem16;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M16BD_NotDestSolidM16Bit;
                break;

            case 10:    /* zNOPz   :           dst <NOP>     */
                optPtr = &SCREENS_Nop;
                break;

            case 14:    /* zINVERTz:        (NOT dst)        */
                optPtr = &M16BF_InvertDestMem16;
                break;
            case 32:    /* xAVGx */

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
                pnColr16 = ~0;
                lclpnColr16 = ~0;
                break;

            case 0:     /* zREPz  */
            case 1:     /* zORz   */
            case 2:     /* zXORz  */
            case 7:     /* zANDz  */
            case 9:     /* zORNz  */
            case 10:    /* zNOPz  */
            case 11:    /* zANDNz */
                pnColr16 = 0;
                lclpnColr16 = 0;
                break;

            case 8:     /* zCLEARz */
                /* set all source bits to 0 */
                pnColr16 = 0;
                lclpnColr16 = 0;
                break;

            case 12:    /* zSETz */
            case 14:    /* zINVERTz */
                /* sets all source bits to 1 */
                pnColr16 = ~0;
                lclpnColr16 = ~0;
                break;

            case 32:    /* xAVGx */
                
#ifdef  GLOBAL_ALPHA_SUPPORT
        
                /* sets all source bits to 0 */
                pnColr16 = 0;
                lclpnColr16 = 0;
        
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

                /* See if the destination is too wide  */
                if( (dRect.Xmax - dRect.Xmin) > srcImag->imWidth )
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
                            FillDrawer = &M16BD_DrawRectEntryImg16Bit;
                            CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                            done = NU_TRUE;

                        }
                    }
                
#endif  /* NO_REGION_CLIP */
                
                }

                if ( !done )
                {
                    /* Draw the image */
                    M16BD_DrawRectEntryImg16Bit(blitRec);
                }
            }
            nuResume(dstBmap);
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BD_DrawRectEntryImg16Bit
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
VOID M16BD_DrawRectEntryImg16Bit(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif

    /* Check if the destination bitmap points to user memory. */
    if (dstBmap->devMode == cMEMORY || dstBmap->devMode == cUSER)
    {
        lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
        srcBgnByte = ((BPP / 8) * sRect.Xmin) >> 1;
        dstBgnByte = ((BPP / 8) * dRect.Xmin) >> 1;
        byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
        srcNextRow = (srcPixBytes - 
                     ((BPP / 8) * byteCntM1) - 
					 (BPP / 8)) >> 1;
        dstNextRow = (dstBmap->pixBytes - 
		             ((BPP / 8) * byteCntM1) - 
		             (BPP / 8)) >> 1;
    }

    else
    {
        if (blitRec->blitSmap->devMode == 0x10)
        {
            /* Store the device mode. */
            Display_Dev_Mode = 0x10;

#if (BPP == 24)
            lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
            srcBgnByte = sRect.Xmin << 1;
            dstBgnByte = dRect.Xmin << 1;
            byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
            srcNextRow = srcPixBytes - (byteCntM1 << 1) - 2;
            dstNextRow = dstBmap->pixBytes - (byteCntM1 << 1) - 2;
#else
            lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
            srcBgnByte = sRect.Xmin;
            dstBgnByte = dRect.Xmin;
            byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
            srcNextRow = (srcPixBytes >> 1) - byteCntM1 - 1;
            dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;
#endif
        }
        else
        {
            /* Store the device mode. */
            Display_Dev_Mode = 0x11;

#if (BPP == 16)
            lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
            srcBgnByte = sRect.Xmin;
            dstBgnByte = dRect.Xmin;
            byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
            srcNextRow = (srcPixBytes >> 1) - byteCntM1 - 1;
            dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;
#elif (BPP == 24)
            lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
            srcBgnByte = (BPP / 8) * sRect.Xmin;
            dstBgnByte = (dRect.Xmin << 1);
            byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
            srcNextRow = srcPixBytes - 
			             ((BPP / 8) * byteCntM1) - 
						 (BPP / 8);
            dstNextRow = (dstBmap->pixBytes - (byteCntM1 << 1) - 2);
#elif (BBPPPP == 32)
            lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
            srcBgnByte = sRect.Xmin << 1;
            dstBgnByte = dRect.Xmin;
            byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
            srcNextRow = (srcPixBytes - (byteCntM1 << 2) - 4) >> 1;
            dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;
#endif
        }
    }

    srcPtr16   = (UINT16 *) (srcImag->imData + (sRect.Ymin * srcPixBytes))
                           + srcBgnByte;
#ifndef SMART_LCD
    dstPtr16   = (UINT16 *)(*(dstBmap->mapTable[0] + dRect.Ymin))
                           + dstBgnByte;
#endif /* SMART_LCD */

    /* blit the rectangle */
    optPtr();
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
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
*    M16BD_WriteImage1M16Bit
*
* DESCRIPTION
*
*    A special case optimization for 1->16Bit WriteImage.
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
VOID M16BD_WriteImage1M16Bit(blitRcd *blitRec)
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

        /* Call the function for pre-processing. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

        M_PAUSE(dstBmap);

        /* background color */
        bkColr16 = blitRec->blitBack;

        /* foreground color */
        pnColr16 = blitRec->blitFore;

        /* Convert the color to target format. */
        COLOR_CONVERT(bkColr16, lclbkColr16);
        COLOR_CONVERT(pnColr16, lclpnColr16);

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* Memory non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
            optPtr = &M16BD_RepDest1M16Bit;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 2:     /* zXORz   :       src XOR dst       */
        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M16BD_OXADest1M16Bit;
            break;

        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M16BF_SetDestMem16;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M16BD_NotDestSolid1M16Bit;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 14:    /* zINVERTz:        (NOT dst)        */
            optPtr = &M16BF_InvertDestMem16;
            break;

        /* Memory transparent */
        case 16:    /* xREPx   :           !0src         */
        case 17:    /* xORx    :       !0src OR  dst     */
        case 25:    /* xORNx   :     !0src OR  (NOT dst) */
        case 28:    /* xSETx   :           1's           */
            optPtr = &M16BD_SetTrans1M16Bit;
            break;

        case 18:    /* xXORx   :       !0src XOR dst     */
        case 27:    /* xANDNx  :     !0src AND (NOT dst) */
        case 30:    /* xINVERTx:        (NOT dst)        */
            optPtr = &M16BD_InvertTrans1M16Bit;
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
        case 32:    /* xAVGx */

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
            pnColr16 = ~pnColr16;
            lclpnColr16 = ~lclpnColr16;
        case 0:     /* zREPz */
        case 1:     /* zORz */
        case 2:     /* zXORz */
        case 7:     /* zANDz */
        case 9:     /* zORNz */
        case 10:    /* zNOPz */
        case 11:    /* zANDNz */
            break;

        case 8:     /* zCLEARz */
            /* sets all source bits to 0 */
            pnColr16 = 0;
            lclpnColr16 = 0;
            break;

        case 12:    /* zSETz    */
        case 14:    /* zINVERTz */
            /* sets all source bits to 1 */
            pnColr16 = ~0;
            lclpnColr16 = ~0;
            break;

        case 32:    /* xAVGx */

#ifdef  GLOBAL_ALPHA_SUPPORT
        
            /* sets all source bits to 0 */
            pnColr16 = 0;
            lclpnColr16 = 0;

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
            if( (dRect.Xmax - dRect.Xmin) > srcImag->imWidth )
            {
                dRect.Xmax = dRect.Xmin + srcImag->imWidth;
            }

            /* See if the destination is too high  */
            if( (dRect.Ymax - dRect.Ymin) > srcImag->imHeight )
            {
                dRect.Ymax = dRect.Ymin + srcImag->imHeight;
            }

            /* do we need to worry about clipping */
            if( clipToRectFlag != 0)
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
                        FillDrawer = &M16BD_DrawRectEntryImg1B16;
                        CLIP_Blit_Clip_Region(blitRec, &dRect, &sRect);
                        done = NU_TRUE;
                    }
                }

                
#endif  /* NO_REGION_CLIP */
                
                if( !done )
                {
                    /* Blit the image to the memory */
                    M16BD_DrawRectEntryImg1B16(blitRec);
                }
            }
        }
        nuResume(dstBmap);


    }
}
/***************************************************************************
* FUNCTION
*
*    M16BD_DrawRectEntryImg1B16
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
VOID M16BD_DrawRectEntryImg1B16(blitRcd *blitRec)
{
#ifdef USING_DIRECT_X
    rect scrnRect;

    scrnRect = dRect;
#endif
    lineCntM1  = dRect.Ymax - dRect.Ymin - 1;
    dstBgnByte = dRect.Xmin;
    srcBgnByte = (sRect.Xmin >> 3);
    byteCntM1  = dRect.Xmax - dRect.Xmin - 1;
    dstNextRow = (dstBmap->pixBytes >> 1) - byteCntM1 - 1;

    srcNextRow = srcPixBytes - ((sRect.Xmin + byteCntM1 + 1) >> 3)
                 + srcBgnByte;
    srcPtr = (UINT8 *) (srcImag->imData + (sRect.Ymin * srcPixBytes)
            + srcBgnByte);

#ifndef SMART_LCD
    dstPtr16   = (UINT16 *) (*(dstBmap->mapTable[0] + dRect.Ymin))
                           + dstBgnByte;
#endif /* SMART_LCD */

    /* blit the rectangle */
    optPtr();
#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing. */
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
*    M16BD_ReadImage16Bit
*
* DESCRIPTION
*
*    A special case optimization for ReadImage, 16 bit per pixel,
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
VOID M16BD_ReadImage16Bit(grafMap * rdiBmap, image *dstImage, INT32 gblYmax, INT32 gblXmax,
                          INT32 gblYmin, INT32 gblXmin)
{
    /* # of bytes across a source bitmap row  */
    INT32   srcRowBytes;

    /* # of bytes across an dstImage row  */
    INT32   dstRowBytes;

    /* bitmap limits  */
    INT32   cXmax,cYmax;
    INT32   count;
    INT32   done = NU_FALSE;
#ifdef SMART_LCD
    UINT32 pixelValue;
#else
    INT32  *srcMapTable;
#endif /* SMART_LCD */

    gblXmax--;
    gblYmax--;

    M_PAUSE(rdiBmap);

    /* # of UINT8 across the source */
    srcRowBytes = (rdiBmap->pixBytes >> 1);

#ifndef SMART_LCD
    /* store mapTable[0] pointer locally */
    srcMapTable = (INT32 *)rdiBmap->mapTable[0];
#endif /* !SMART_LCD */

    /* get the source bitmap's limits  */
    cXmax = rdiBmap->pixWidth - 1;
    cYmax = rdiBmap->pixHeight - 1;

    /* Initialize dstImage header and destination copy parameters  */
    /* assume no span segment  */
    dstImage->imFlags  = 0;

    /* always plane # 1  */
    dstImage->imPlanes = 1;

    /* # bits per pixel  */
    dstImage->imBits   = 16;

    /* set to 0   */
    dstImage->imAlign  = 0;
    dstImage->imWidth  = gblXmax - gblXmin + 1;
    dstRowBytes        = dstImage->imWidth;
    dstImage->imBytes  = dstRowBytes << 1;
    dstPtr16           = (UINT16 *)((INT32) &dstImage->imData[0]);

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
            if( gblXmin < 0)
            {
                dstPtr16 -= gblXmin;
                gblXmin = 0;
            }

            /* Check that gblYmin is not off bitmap  */
            if( gblYmin < 0 )
            {
                dstPtr16 = dstPtr16 - (dstRowBytes * gblYmin);
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
                    /*Set the offset from the end of one dstImage row to the start of the next  */
                    dstNextRow = dstRowBytes - byteCntM1 - 1;

                    /*Set offset from the end of one source rectangle row to the start of the next */
                    srcNextRow = srcRowBytes - byteCntM1 - 1;
#ifndef SMART_LCD
                    /* point to row table entry for the first row  */
                    srcPtr16 = (UINT16 *) *(srcMapTable + gblYmin) + gblXmin;
#endif /* SMART_LCD */

                    do
                    {
#if         (COLOR_CONVERSION_NEEDED == NU_FALSE)
                        for( count = 0; count <= byteCntM1; count++)
                        {
#ifdef SMART_LCD
                            /* Call the function to get the pixel. */
                            SCREENI_Display_Device.display_get_pixel_hook(gblXmin + (byteCntM1 + count), gblYmin + ((gblYmax - gblYmin) - lineCntM1), &pixelValue);
                            *dstPtr16++ = pixelValue;
#else /* SMART_LCD */
                            /* point to row table entry for the first row  */
                            *dstPtr16++ = *srcPtr16++;
#endif /* SMART_LCD */
                        }

#else

#if (BPP == 16)
                        for( count = 0; count <= byteCntM1; count++)
                        {
                            UINT32 lclSrcColr;
#ifdef SMART_LCD
                            /* Call the function to get the pixel. */
                            SCREENI_Display_Device.display_get_pixel_hook(gblXmin + (byteCntM1 + count), gblYmin + ((gblYmax - gblYmin) - lineCntM1), &pixelValue);
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(UINT16(pixelValue), lclSrcColr);
#else /* SMART_LCD */
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(*srcPtr16, lclSrcColr);
#endif /* SMART_LCD */

                            *dstPtr16++ = (lclSrcColr & 0xFFFF);
#ifndef SMART_LCD
                            srcPtr16++;
#endif /* SMART_LCD */
                        }
#elif (BPP == 24)
                        for( count = 0; count <= byteCntM1; count+=2)
                        {
                            UINT32 lclSrcColr1, lclSrcColr2;

                            /* Convert the color. */
#ifdef SMART_LCD
                            UINT32 pixelValue2;
                            /* Call the function to get the pixel. */
                            SCREENI_Display_Device.display_get_pixel_hook(gblXmin + (byteCntM1 + count), gblYmin + ((gblYmax - gblYmin) - lineCntM1), &pixelValue);
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(UINT16(pixelValue), lclSrcColr);
                            /* Call the function to get the pixel. */
                            SCREENI_Display_Device.display_get_pixel_hook(gblXmin + (byteCntM1 + count) + 1, gblYmin + ((gblYmax - gblYmin) - lineCntM1), &pixelValue);
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(UINT16(pixelValue2), lclSrcColr2);
#else /* SMART_LCD */
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(*srcPtr16, lclSrcColr);
                            COLOR_BACK_CONVERT(*(srcPtr16+1), lclSrcColr2);
#endif /* SMART_LCD */

                            *dstPtr16++ = (lclSrcColr1 & 0xFFFF);
                            *dstPtr16++ = ((lclSrcColr1 >> 16) & 0xFF) | ((lclSrcColr2 << 8) & 0xFF00);
                            *dstPtr16++ = ((lclSrcColr2 >> 8) & 0xFFFF);

#ifndef SMART_LCD
                            srcPtr16+=2;
#endif /* SMART_LCD */
                        }
#elif (BPP == 32)
                        for( count = 0; count <= byteCntM1; count++)
                        {
                            UINT32 lclSrcColr;
#ifdef SMART_LCD
                            /* Call the function to get the pixel. */
                            SCREENI_Display_Device.display_get_pixel_hook(gblXmin + (byteCntM1 + count), gblYmin + ((gblYmax - gblYmin) - lineCntM1), &pixelValue);
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(UINT16(pixelValue), lclSrcColr);
#else /* SMART_LCD */
                            /* Convert the color. */
                            COLOR_BACK_CONVERT(*srcPtr16, lclSrcColr);
#endif /* SMART_LCD */
                            *dstPtr16++ = (lclSrcColr & 0xFFFF);
                            *dstPtr16++ = ((lclSrcColr >> 16) & 0xFFFF);
#ifndef SMART_LCD
                            srcPtr16++;
#endif /* SMART_LCD */
                        }
#endif

#endif
                        dstPtr16 += dstNextRow;
#ifndef SMART_LCD
                        srcPtr16 += srcNextRow;
#endif /* SMART_LCD */
                    }while( --lineCntM1 >= 0);
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
*    M16BD_Transparency
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
VOID M16BD_Transparency(VOID)
{ 
    UINT16 pnColrR, pnColrG, pnColrB,lclRed,lclBlue,lclGreen;
    UINT16 srcColrR, srcColrG, srcColrB;
    UINT16 TransR, TransB, TransG, tempcolor;

#ifndef SMART_LCD
    UINT16 *lclDstPtr;
#else /*  */
    UINT32 pixelValue;
#endif /* SMART_LCD */
    UINT16 *lclSrcPtr;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr = dstPtr16;
#endif /* SMART_LCD */
    lclSrcPtr = srcPtr16;

#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
#endif /* SMART_LCD */
#ifdef CM565
    {
        /* set up local colors */
        pnColrR = (UINT16)(lclpnColr16 >> 11);
        pnColrG = (UINT16)((lclpnColr16 >> 5) & 0x3f);
        pnColrB = (UINT16)(lclpnColr16  & 0x1f);

#ifdef SMART_LCD
        lclRed = (UINT16)(pixelValue >> 11);
        lclGreen = (UINT16)((pixelValue  >> 5) & 0x3f);
        lclBlue = (UINT16)(pixelValue  & 0x1f);
#else /* SMART_LCD */
        lclRed = (UINT16)(*lclDstPtr >> 11);
        lclGreen = (UINT16)((*lclDstPtr  >> 5) & 0x3f);
        lclBlue = (UINT16)(*lclDstPtr  & 0x1f);
#endif /* SMART_LCD */

    }
#else
    {
        pnColrR = (UINT16)((lclpnColr16 >> 10)& 0x1f);
        pnColrG = (UINT16)((lclpnColr16 >> 5) & 0x1f);
        pnColrB = (UINT16)(lclpnColr16  & 0x1f);

#ifdef SMART_LCD
        lclRed = (UINT16)((pixelValue >> 10)& 0x1f);
        lclGreen = (UINT16)((pixelValue >> 5) & 0x1f);
        lclBlue = (UINT16)(pixelValue  & 0x1f);
#else /* SMART_LCD */
        lclRed = (UINT16)((*lclDstPtr >> 10)& 0x1f);
        lclGreen = (UINT16)((*lclDstPtr  >> 5) & 0x1f);
        lclBlue = (UINT16)(*lclDstPtr  & 0x1f);
#endif /* SMART_LCD */
    }
#endif

    if (pnColr16 != 0 )
    {
        pxBitVal = 128 >> (sRect.Xmin & 7);

        lclLineCnt = lineCntM1;

        while( lclLineCnt-- >= 0 )
        {
            /* for each row */
            pxBit = pxBitVal;

            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
                /* Get the alpha value to blend the pixel */
                //alpha_level = (float)((float)(*(lclDstPtr + 3))/255.0);

                /* for each UINT8  in the row */
                TransR = (((1-alpha_level) * pnColrR) + (alpha_level * (lclRed)));

                TransG = (((1-alpha_level) * pnColrG) + (alpha_level * (lclGreen)));

                TransB = (((1-alpha_level) * pnColrB) + (alpha_level * (lclBlue)));


#ifdef CM565
                tempcolor = (TransR << 11);
                tempcolor = tempcolor | (TransG << 5);

#else
#ifdef SMART_LCD
                /* Call the function to get the pixel. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue >> 15;
                tempcolor = pixelValue << 15;
                tempcolor = (TransR << 10);
                tempcolor = tempcolor | (TransG << 5);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr >> 15;
                tempcolor = *lclDstPtr << 15;
                tempcolor = (TransR << 10);
                tempcolor = tempcolor | (TransG << 5);
#endif /* SMART_LCD */
#endif
                tempcolor = tempcolor | (TransB & 0x1f);

#ifdef SMART_LCD
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), tempcolor);
#else /* SMART_LCD */
                *lclDstPtr = tempcolor;
                lclDstPtr++;
#endif /* SMART_LCD */
                pxBit = pxBit >> 1;
                if( pxBit == 0 )
                {
                    pxBit = 128;
                    lclSrcPtr++;
                }
            }

            /* advance to next row */
#ifndef SMART_LCD
            lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
            lclSrcPtr += srcNextRow;
        }
    }

    else
    {
        /* blit top to bottom, left to right */
        /* set up local pointers */
        lclDstPtr = dstPtr16;
        lclSrcPtr = srcPtr16;

        lclLineCnt = lineCntM1;
        pxBitVal = 128 >> (sRect.Xmin & 7);
        while( lclLineCnt-- >= 0 )
        {
            pxBit = pxBitVal;
            /* for each row */
            lclByteCnt = byteCntM1;
            while( lclByteCnt-- >= 0 )
            {
#ifdef CM565
                UINT32 lclSrcColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

                /* set up local colors */
                srcColrR = (UINT16)(lclSrcColr >> 11);
                srcColrG = (UINT16)((lclSrcColr  >> 5) & 0x3f);
                srcColrB = (UINT16)(lclSrcColr & 0x1f);

#ifdef SMART_LCD
                lclRed = (UINT16)(pixelValue >> 11);
                lclGreen = (UINT16)((pixelValue  >> 5) & 0x3f);
                lclBlue = (UINT16)(pixelValue  & 0x1f);
#else /* SMART_LCD */
                lclRed = (UINT16)(*lclDstPtr >> 11);
                lclGreen = (UINT16)((*lclDstPtr  >> 5) & 0x3f);
                lclBlue = (UINT16)(*lclDstPtr  & 0x1f);
#endif /* SMART_LCD */

#else
                UINT32 lclSrcColr, lclSrcPtrtemp;

                /* Convert the color to target format. */
                COLOR_CONVERT(*lclSrcPtr, lclSrcColr);

                srcColrR = (UINT16)((lclSrcColr >> 10)& 0x1f);
                srcColrG = (UINT16)((lclSrcColr >> 5) & 0x1f);
                srcColrB = (UINT16)(lclSrcColr & 0x1f);

#ifdef SMART_LCD
                lclRed = (UINT16)((pixelValue >> 10)& 0x1f);
                lclGreen = (UINT16)((pixelValue >> 5) & 0x1f);
                lclBlue = (UINT16)(pixelValue  & 0x1f);
#else /* SMART_LCD */
                lclRed = (UINT16)((*lclDstPtr >> 10)& 0x1f);
                lclGreen = (UINT16)((*lclDstPtr  >> 5) & 0x1f);
                lclBlue = (UINT16)(*lclDstPtr  & 0x1f);
#endif /* SMART_LCD */
#endif

                TransR = (((1-alpha_level) * srcColrR) + (alpha_level * (lclRed)));

                TransG = (((1-alpha_level) * srcColrG) + (alpha_level * (lclGreen)));

                TransB = (((1-alpha_level) * srcColrB) + (alpha_level * (lclBlue)));


#ifdef CM565
                tempcolor = (TransR << 11);
                tempcolor = tempcolor | (TransG << 5);

#else
                lclSrcPtrtemp = *lclSrcPtr >> 15;
                tempcolor = lclSrcPtrtemp << 15;
                tempcolor = (TransR << 10);
                tempcolor = tempcolor | (TransG << 5);
#endif
                tempcolor = tempcolor | (TransB & 0x1f);

#ifdef SMART_LCD
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), tempcolor);
#else /* SMART_LCD */
                *lclDstPtr = tempcolor;
                lclDstPtr++;
#endif /* SMART_LCD */
                lclSrcPtr++;

            }

            /* advance to next row */
#ifndef SMART_LCD
            lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
            lclSrcPtr += srcNextRow;
        }
    }   
}

/***************************************************************************
* FUNCTION
*
*    M16BD_Text_Transparency
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
VOID M16BD_Text_Transparency(VOID)
{   
    UINT16 pnColrR, pnColrG, pnColrB;
    UINT16 bkColrR, bkColrG, bkColrB;
#ifndef SMART_LCD
    UINT16 *lclDstPtr;
#else /*  */
    UINT32 pixelValue;
#endif /* SMART_LCD */
    UINT8 *lclSrcPtr;
    UINT16 tempcolor, TransR, TransB, TransG,lclRed,lclGreen,lclBlue;
    INT32 lclLineCnt, lclByteCnt;
    INT32 pxBit;
    INT32 pxBitVal;

    /* set up local pointers */
#ifndef SMART_LCD
    lclDstPtr = dstPtr16;
#endif /* SMART_LCD */
    lclSrcPtr = srcPtr16;

#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
#endif /* SMART_LCD */
#ifdef CM565
    {
        /* set up local colors */
        pnColrR =(UINT16)(lclpnColr16 >> 11);
        pnColrG = (UINT16)((lclpnColr16 >> 5) & 0x3f);
        pnColrB = (UINT16)(lclpnColr16  & 0x1f);

        bkColrR = (UINT16)(lclbkColr16 >> 11);
        bkColrG = (UINT16)((lclbkColr16 >> 5) & 0x3f);
        bkColrB = (UINT16)(lclbkColr16  & 0x1f);

#ifdef SMART_LCD
        lclRed = (UINT16)(pixelValue >> 11);
        lclGreen = (UINT16)((pixelValue  >> 5) & 0x3f);
        lclBlue = (UINT16)(pixelValue  & 0x1f);
#else /* SMART_LCD */
        lclRed = (UINT16)(*lclDstPtr >> 11);
        lclGreen =(UINT16) ((*lclDstPtr >> 5) & 0x3f);
        lclBlue = (UINT16)(*lclDstPtr & 0x1f);
#endif /* SMART_LCD */
     }
    #else
    {

        pnColrR = (UINT16)((lclpnColr16 >> 10)& 0x1f);
        pnColrG = (UINT16)((lclpnColr16 >> 5) & 0x1f);
        pnColrB = (UINT16)(lclpnColr16  & 0x1f);

        bkColrR = (UINT16)((lclbkColr16 >> 10)& 0x1f);
        bkColrG = (UINT16)((lclbkColr16 >> 5) & 0x1f);
        bkColrB = (UINT16)(lclbkColr16  & 0x1f);

#ifdef SMART_LCD
        lclRed = (UINT16)((pixelValue >> 10)& 0x1f);
        lclGreen = (UINT16)((pixelValue >> 5) & 0x1f);
        lclBlue = (UINT16)(pixelValue  & 0x1f);
#else /* SMART_LCD */
        lclRed = (UINT16)((*lclDstPtr>> 10)& 0x1f);
        lclGreen = (UINT16)((*lclDstPtr >> 5) & 0x1f);
        lclBlue = (UINT16)(*lclDstPtr & 0x1f);
#endif /* SMART_LCD */
    }
    #endif

    pxBitVal = 128 >> (sRect.Xmin & 7);

    lclLineCnt = lineCntM1;
    while( lclLineCnt-- >= 0 )
    {
        /* for each row */
        pxBit = pxBitVal;

        lclByteCnt = byteCntM1;
        while( lclByteCnt-- >= 0 )
        {
            /* Get the alpha value to blend the pixel */
            //alpha_level = (float)((float)(*(lclDstPtr + 3))/255.0);

            /* for each UINT8 in the row */
            if(*lclSrcPtr & pxBit )
            {
                /* for each UINT8  in the row */
                TransR = (((1-alpha_level) * pnColrR) + (alpha_level * (lclRed)));

                TransG = (((1-alpha_level) * pnColrG) + (alpha_level * (lclGreen)));

                TransB = (((1-alpha_level) * pnColrB) + (alpha_level * (lclBlue)));


#ifdef CM565
                tempcolor = (TransR << 11);
                tempcolor = tempcolor | (TransG << 5);

#else
#ifdef SMART_LCD
                /* Call the function to get the pixel. */
                SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                pixelValue = pixelValue >> 15;
                tempcolor = pixelValue << 15;
                tempcolor = (TransR << 10);
                tempcolor = tempcolor | (TransG << 5);
#else /* SMART_LCD */
                *lclDstPtr = *lclDstPtr >> 15;
                tempcolor = *lclDstPtr << 15;
                tempcolor = (TransR << 10);
                tempcolor = tempcolor | (TransG << 5);
#endif /* SMART_LCD */
#endif
                tempcolor = tempcolor | (TransB & 0x1f);

#ifdef SMART_LCD
                /* Call the function to set the pixel. */
                SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), tempcolor);
#else /* SMART_LCD */
                *lclDstPtr = tempcolor;
#endif /* SMART_LCD */

            }
            else
            {
                if(set_back_color == NU_TRUE)
                {
                    /* source is 0 */
                    /* for each UINT8  in the row */
                    TransR = (((1-alpha_level) * bkColrR) + (alpha_level * (lclRed)));

                    TransG = (((1-alpha_level) * bkColrG) + (alpha_level * (lclGreen)));

                    TransB = (((1-alpha_level) * bkColrB) + (alpha_level * (lclBlue)));


#ifdef CM565
                    tempcolor = (TransR << 11);
                    tempcolor = tempcolor | (TransG << 5);

#else

#ifdef SMART_LCD
                    /* Call the function to get the pixel. */
                    SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), &pixelValue);
                    pixelValue = pixelValue >> 15;
                    tempcolor = pixelValue << 15;
                    tempcolor = (TransR << 10);
                    tempcolor = tempcolor | (TransG << 5);
#else /* SMART_LCD */
                    *lclDstPtr = *lclDstPtr >> 15;
                    tempcolor = *lclDstPtr << 15;
                    tempcolor = (TransR << 10);
                    tempcolor = tempcolor | (TransG << 5);
#endif /* SMART_LCD */
#endif
                    tempcolor = tempcolor | (TransB & 0x1f);

#ifdef SMART_LCD
                    /* Call the function to set the pixel. */
                    SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + (byteCntM1 - lclByteCnt - 1), dRect.Ymin + (lineCntM1 - lclLineCnt - 1), tempcolor);
#else /* SMART_LCD */
                    *lclDstPtr = tempcolor;
#endif /* SMART_LCD */
                }
            }
#ifndef SMART_LCD
            lclDstPtr++;
#endif /* SMART_LCD */
            pxBit = pxBit >> 1;
            if( pxBit == 0 )
            {
                pxBit = 128;
                lclSrcPtr++;
            }
        }

        /* advance to next row */
#ifndef SMART_LCD
        lclDstPtr += dstNextRow;
#endif /* SMART_LCD */
        lclSrcPtr += srcNextRow;
    }    
}

#endif  /* GLOBAL_ALPHA_SUPPORT */

#endif /* #ifdef INCLUDE_16_BIT */

