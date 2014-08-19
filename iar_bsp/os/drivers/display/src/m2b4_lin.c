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
*  m2b4_lin.c
*
* DESCRIPTION
*
*  This file contains the thin line drawing functions for 2/4 bit memory
*  or Display (VGA or LCD).
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M2B4L_ThinLine2_4Bit
*  M2B4L_M2_4BURTable
*  M2B4L_M2_4BMOTable
*  M2B4L_M2_4BUXTable
*  M2B4L_M2_4BMATable
*  M2B4L_M2_4BMO_NDTable
*  M2B4L_M2_4BMA_NDTable
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

/***************************************************************************
* FUNCTION
*
*    M2B4L_ThinLine2_4Bit
*
* DESCRIPTION
*
*    Is a special case optimization for thin line drawing to 2/4 bit memory
*    destinations, rectangular and/or region clipping.
*
* INPUTS
*
*    blitRcd * lineRec
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID M2B4L_ThinLine2_4Bit(blitRcd *lineRec)
{
    lineS *listPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_TRUE;
    INT16 done           = NU_FALSE;

    /* Table used to NOT the source color according to raster op. */
    UINT8 FlipTable[16] = {0,    0,    0,    0x0f,
                           0x0f, 0x0f, 0x0f, 0,
                           0,    0,    0,    0,
                           0,    0x0f, 0,    0x0f};

    /* Table used to force the source color to 0x0f according to raster op. */
    UINT8 ForceTableOr[16] = {0,    0,    0,    0,
                              0,    0,    0,    0,
                              0,    0,    0,    0,
                              0x0f, 0,    0x0f, 0};

    /* Table used to force the source color to 0 according to raster op. */
    UINT8 ForceTableAnd[16] = {0x0f, 0x0f, 0x0f, 0x0f,
                               0x0f, 0x0f, 0x0f, 0x0f,
                               0,    0x0f, 0x0f, 0x0f,
                               0x0f, 0x0f, 0x0f, 0x0f};

    /* set up the rectangular/region clip info */
    if( CLIP_Set_Up_Clip(lineRec, &cRect, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        
#ifndef NO_REGION_CLIP
                
        /* do we need to worry about clipping at all*/
        if( clipToRegionFlag == 0)
                
#endif  /* NO_REGION_CLIP */
                
        {
            /* no -- valid coordinates are guaranteed at a higher level */
            /* the clipping code expects an old-style clip rect */
            cRect.Xmax--;
            cRect.Ymax--;

            cRect.Xmin++;
            cRect.Ymin++;

        }
        
#ifndef NO_REGION_CLIP
                
        else
        {
            /* yes, copy the clip rect to the base, unchanging clip rect */
            bcXmin = cRect.Xmin;
            bcYmin = cRect.Ymin;
            bcXmax = cRect.Xmax - 1;
            bcYmax = cRect.Ymax - 1;
        }
                
#endif  /* NO_REGION_CLIP */
                
        /* set up local variables */
        listPtr = (lineS *) lineRec->blitList;
        rectCnt     = lineRec->blitCnt;
        lclPortMask = lineRec->blitMask;
        dstClass    = lineRec->blitRop;

        /* pattern 0? */
        if( lineRec->blitPat == 0)
        {
            /* yes, use background color */
            if( dstClass & 0x10)
            {
                /* done if transparent */
                done = NU_TRUE;
            }

            if(!done )
            {
                UINT8 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT((UINT8) lineRec->blitBack, lclColr);

                lclPenColor = lclColr;
            }
        }
        else
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT((UINT8) lineRec->blitFore, lclColr);

            /* pattern 1 is the only other possibility */
            lclPenColor = lclColr;
        }
    }
    if( !done )
    {

        /* we don't care about transparency */
        dstClass = dstClass & 0x0f;

        /* NOT the color if appropriate */
        lclPenColor = lclPenColor ^ FlipTable[dstClass];

        /* make the color 0x0f if appropriate */
        lclPenColor = lclPenColor | ForceTableOr[dstClass];

        /* make the color 0 if appropriate */
        lclPenColor = lclPenColor & ForceTableAnd[dstClass];

        /* mask it */
        lclPenColor = lclPenColor & ((UINT8) lclPortMask);

        dstBmap =  lineRec->blitDmap;

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

        /* Call the function for pre-processing task. */
        SCREENI_Display_Device.display_pre_process_hook();

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        HideCursor();
#endif

        /* lock grafMap */
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
            lclPenColor = (lclPenColor & 0x03) << 6;
        }
        else
        {
            /* 4 bpp */
            flipMask    = 1;
            shiftMask   = 0xf0;
            firstOffset = 0x01;
            lclPenColor = lclPenColor << 4;
        }

        rowTablePtr[1] = (INT32 *) dstBmap->mapTable[0];

        /* look up the optimization routine */
        switch (dstClass)
        {
        /* LCD non-transparent */
        case 0:     /* zREPz   :           src           */
        case 4:     /* zNREPz  :        (NOT src)        */
        case 8:     /* zCLEARz :           0's           */
        case 12:    /* zSETz   :           1's           */
            optPtr = &M2B4L_M2_4BURTable;
            break;

        case 1:     /* zORz    :       src OR  dst       */
        case 5:     /* zNORz   :    (NOT src) OR  dst    */
            optPtr = &M2B4L_M2_4BMOTable;
            break;

        case 2:     /* zXORz   :       src XOR dst       */
        case 6:     /* zNXORz  :    (NOT src) XOR dst    */
        case 14:    /* zINVERTz:        (NOT dst)        */
                optPtr = &M2B4L_M2_4BUXTable;
            break;

        case 3:     /* zNANDz  :    (NOT src) AND dst    */
        case 7:     /* zANDz   :       src AND dst       */
            optPtr = &M2B4L_M2_4BMATable;
            break;

        case 9:     /* zORNz   :     src OR  (NOT dst)   */
        case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
            optPtr = &M2B4L_M2_4BMO_NDTable;
            break;

        case 10:    /* zNOPz   :           dst <NOP>     */
            optPtr = &SCREENS_Nop;
            break;

        case 11:    /* zANDNz  :     src AND (NOT dst)   */
        case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
            optPtr = &M2B4L_M2_4BMA_NDTable;
            break;
        }

#ifndef NO_REGION_CLIP
                
        if( clipToRegionFlag != 0)
        {
            /* handle region clipping in a separate loop */
            LineDrawer = &CLIP_ClipAndDrawEntry;
            CLIP_Line_Clip_Region(rectCnt, listPtr);
            nuResume(dstBmap);
            done = NU_TRUE;
        }
                
#endif  /* NO_REGION_CLIP */
                
    }
    if( !done )
    {
        while ( rectCnt-- > 0 )
        {
            /* Main line-drawing loop. */
            dRect.Xmin = listPtr->lStart.X;
            dRect.Ymin = listPtr->lStart.Y;
            dRect.Xmax = listPtr->lEnd.X;
            dRect.Ymax = listPtr->lEnd.Y;

            drawStat = (signed char) listPtr->flStat;
            listPtr++;

            /* clip and draw the line */
            CLIP_ClipAndDrawEntry();
        }
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        ShowCursor();
#endif
    }
    nuResume(dstBmap);

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    /* Call the function for post-processing task. */
    SCREENI_Display_Device.display_post_process_hook();

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

}

/***************************************************************************
* FUNCTION
*
*    M2B4L_M2_4BURTable
*
* DESCRIPTION
*
*    Unmasked replace table.
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
VOID M2B4L_M2_4BURTable(VOID)
{
    INT32 i;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;
    INT16 done = NU_FALSE;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxColor = (UINT8) (lclPenColor >> pxShift);
    pxBit   = (UINT8) (shiftMask >> pxShift);

    /* Low-level line drawing optimizations. */
    /* Unmasked low-level optimizations. */
    /*  Horizontal left->right replace. */
    if (lineDir == 5)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (dRect.Xmin >> flipMask);

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            *dstPtr = (*dstPtr & ~pxBit) | pxColor;

            pxColor = pxColor >> shfCnt;
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                dstPtr++;
                pxBit = shiftMask;
                pxColor = lclPenColor;
            }
        }
        done = NU_TRUE;
    }

    if ( !done )
    {
        dstBgnByte = dRect.Xmin >> flipMask;

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
             dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
            *dstPtr = (*dstPtr & ~pxBit) | pxColor;

            /* appropriate for the line direction */
            switch (lineDir)
            {

            /* Y major top->bottom, right->left replace. */
            case 0:
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

                    pxColor = pxColor << shfCnt;
                    pxBit = (pxBit << shfCnt);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        if( shfCnt == 2 )
                        {
                            pxBit = 0x03;
                            pxColor = lclPenColor >> 6;
                        }
                        else
                        {
                            pxBit = 0x0f;
                            pxColor = lclPenColor >> 4;
                        }
                    }
                }
            break;

            /* Vertical top->bottom replace. */
            case 1:
                rowTablePtr[0]++;
                break;

            /* Y major top->bottom, left->right replace. */
            case 2:
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

                    pxColor = pxColor >> shfCnt;
                    pxBit = (pxBit >> shfCnt);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = shiftMask;
                        pxColor = lclPenColor;
                    }
                }
            break;

            /* Diagonal top->bottom, left->right replace. */
            case 3:
                rowTablePtr[0]++;

                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }
            break;

            /* X major top->bottom, left->right replace. */
            case 4:
                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
            break;


            /* Diagonal top->bottom, right->left replace. */
            case 6:
                rowTablePtr[0]++;

                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }
            break;

            /* X major top->bottom, right->left replace. */
            case 7:
                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
            break;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4L_M2_4BMOTable
*
* DESCRIPTION
*
*    The Masked OR Table
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

VOID M2B4L_M2_4BMOTable(VOID)
{
    INT32 i;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxColor = (UINT8) (lclPenColor >> pxShift);
    pxBit = (UINT8) (shiftMask >> pxShift);

    dstBgnByte = dRect.Xmin >> flipMask;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
         dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
        *dstPtr = *dstPtr | pxColor;

        /* appropriate for the line direction */
        switch (lineDir)
        {
        /* Y major top->bottom, right->left masked, OR. */
        case 0:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }
            }
        break;

        /* Y major top->bottom, left->right masked, OR. */
        /* Y major top->bottom, vertical masked, OR. */
        case 1:
        case 2:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }
            }
        break;
        /* X major top->bottom, left->right masked, OR. */
        /* Diagonal top->bottom, left->right masked, OR. */
        /* Horizontal left->right masked, OR. */
        case 3:
        case 4:
        case 5:
            pxColor = pxColor >> shfCnt;
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = shiftMask;
                pxColor = lclPenColor;
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;

        /* X major top->bottom, right->left masked, OR. */
        /* Diagonal top->bottom, right->left masked, OR. */
        case 6:
        case 7:

            pxColor = pxColor << shfCnt;
            pxBit = (pxBit << shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                if( shfCnt == 2 )
                {
                    pxBit = 0x03;
                    pxColor = lclPenColor >> 6;
                }
                else
                {
                    pxBit = 0x0f;
                    pxColor = lclPenColor >> 4;
                }
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4L_M2_4BUXTable
*
* DESCRIPTION
*
*    Unmasked XOR table
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
VOID M2B4L_M2_4BUXTable(VOID)
{
    INT32 i;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;
    UINT8 storeBits;
    UINT8 xorBits;
    INT16 done = NU_FALSE;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxColor = (UINT8) (lclPenColor >> pxShift);
    pxBit   = (UINT8) (shiftMask >> pxShift);

    /*  Horizontal left->right XOR. */
    if (lineDir == 5)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (dRect.Xmin >> flipMask);

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            *dstPtr = (*dstPtr & ~pxBit) | pxColor;

            pxColor = pxColor >> shfCnt;
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                dstPtr++;
                pxBit = shiftMask;
                pxColor = lclPenColor;
            }
        }
        done = NU_TRUE;
    }

    if ( !done )
    {
        dstBgnByte = dRect.Xmin >> flipMask;

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
             dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
            /* store the 4 bits that aren't going */
            /* to be XOR'd and clear the other 4 */
            storeBits = *dstPtr & ~pxBit;

            /* XOR the entire byte then clear the 4 */
            /* bits that aren't being modified */
            xorBits = (*dstPtr ^ pxColor) & pxBit;

            /* write the entire byte to the screen */
            *dstPtr = xorBits | storeBits;

            /* appropriate for the line direction */
            switch (lineDir)
            {
            /* Y major top->bottom, left->right XOR. */
            case 0:
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

                    pxColor = pxColor << shfCnt;
                    pxBit = (pxBit << shfCnt);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        if( shfCnt == 2 )
                        {
                            pxBit = 0x03;
                            pxColor = lclPenColor >> 6;
                        }
                        else
                        {
                            pxBit = 0x0f;
                            pxColor = lclPenColor >> 4;
                        }
                    }
                }
            break;

            /* Vertical top->bottom XOR. */
            case 1:
               rowTablePtr[0]++;
            break;

            /* Y major top->bottom, left->right XOR. */
            case 2:
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

                    pxColor = pxColor >> shfCnt;
                    pxBit = (pxBit >> shfCnt);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = shiftMask;
                        pxColor = lclPenColor;
                    }
                }
            break;

            /* Diagonal top->bottom, left->right XOR. */
            case 3:
                rowTablePtr[0]++;

                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }
            break;

            /* X major top->bottom, left->right XOR. */
            case 4:
                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
            break;

            /* Diagonal top->bottom, right->left XOR. */
            case 6:
                rowTablePtr[0]++;

                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }
            break;

            /* X major top->bottom, right->left XOR. */
            case 7:
                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }

            break;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4L_M2_4BMATable
*
* DESCRIPTION
*
*    Masked AND Table.
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
VOID M2B4L_M2_4BMATable(VOID)
{
    INT32 i;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxColor = (UINT8) (lclPenColor >> pxShift);
    pxBit = (UINT8) (shiftMask >> pxShift);

    dstBgnByte = dRect.Xmin >> flipMask;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
         dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
        *dstPtr = *dstPtr | pxColor;

        /* appropriate for the line direction */
        switch (lineDir)
        {
        /* Y major top->bottom, right->left masked, AND. */
        case 0:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }
            }
        break;

        /* Y major top->bottom, left->right masked, AND. */
        /* Y major top->bottom, vertical masked, AND. */
        case 1:
        case 2:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }
            }
        break;

        /* X major top->bottom, left->right masked, AND. */
        /* Diagonal top->bottom, left->right masked, AND. */
        /* Horizontal left->right masked, AND. */
        case 3:
        case 4:
        case 5:
            pxColor = pxColor >> shfCnt;
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = shiftMask;
                pxColor = lclPenColor;
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;

        /* X major top->bottom, right->left masked, AND. */
        /* Diagonal top->bottom, right->left masked, AND. */
        case 6:
        case 7:
            pxColor = pxColor << shfCnt;
            pxBit = (pxBit << shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                if( shfCnt == 2 )
                {
                    pxBit = 0x03;
                    pxColor = lclPenColor >> 6;
                }
                else
                {
                    pxBit = 0x0f;
                    pxColor = lclPenColor >> 4;
                }
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;

        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4L_M2_4BMO_NDTable
*
* DESCRIPTION
*
*   Masked OR NOT Table.
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
VOID M2B4L_M2_4BMO_NDTable(VOID)
{
    INT32 i;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxColor = (UINT8) (lclPenColor >> pxShift);
    pxBit = (UINT8) (shiftMask >> pxShift);

    dstBgnByte = dRect.Xmin >> flipMask;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
         dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
        *dstPtr = (*dstPtr ^ pxBit) | pxColor;

        /* appropriate for the line direction */
        switch (lineDir)
        {
        /* Y major top->bottom, right->left masked, OR NOT dest. */
        case 0:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }
            }
        break;
        /* Y major top->bottom, left->right masked, OR NOT dest. */
        /* Y major top->bottom, vertical masked, OR NOT dest. */
        case 1:
        case 2:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }
            }
        break;

        /* X major top->bottom, left->right masked, OR NOT dest. */
        /* Diagonal top->bottom, left->right masked, OR NOT dest. */
        /* Horizontal left->right masked, OR NOT dest. */
        case 3:
        case 4:
        case 5:
            pxColor = pxColor >> shfCnt;
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = shiftMask;
                pxColor = lclPenColor;
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;

        /* X major top->bottom, right->left masked, OR NOT dest. */
        /* Diagonal top->bottom, right->left masked, OR NOT dest. */
        case 6:
        case 7:
            pxColor = pxColor << shfCnt;
            pxBit = (pxBit << shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                if( shfCnt == 2 )
                {
                    pxBit = 0x03;
                    pxColor = lclPenColor >> 6;
                }
                else
                {
                    pxBit = 0x0f;
                    pxColor = lclPenColor >> 4;
                }
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M2B4L_M2_4BMA_NDTable
*
* DESCRIPTION
*
*   Masked AND NOT Table.
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
VOID M2B4L_M2_4BMA_NDTable(VOID)
{
    INT32 i;
    INT32 pxShift;
    UINT8 pxBit;
    UINT8 pxColor;

    pxShift =  (dRect.Xmin & firstOffset) * shfCnt;
    pxColor = (UINT8) (lclPenColor >> pxShift);
    pxBit = (UINT8) (shiftMask >> pxShift);

    dstBgnByte = dRect.Xmin >> flipMask;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
         dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
        *dstPtr = (*dstPtr ^ pxBit) & (~pxBit | pxColor);

        /* appropriate for the line direction */
        switch (lineDir)
        {
        /* Y major top->bottom, right->left masked, AND NOT dest. */
        case 0:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor << shfCnt;
                pxBit = (pxBit << shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    if( shfCnt == 2 )
                    {
                        pxBit = 0x03;
                        pxColor = lclPenColor >> 6;
                    }
                    else
                    {
                        pxBit = 0x0f;
                        pxColor = lclPenColor >> 4;
                    }
                }
            }
        break;
        /* Y major top->bottom, left->right masked, AND NOT dest. */
        /* Y major top->bottom, vertical masked, AND NOT dest. */
        case 1:
        case 2:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

                pxColor = pxColor >> shfCnt;
                pxBit = (pxBit >> shfCnt);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = shiftMask;
                    pxColor = lclPenColor;
                }
            }
        break;

        /* X major top->bottom, left->right masked, AND NOT dest. */
        /* Diagonal top->bottom, left->right masked, AND NOT dest. */
        /* Horizontal left->right masked, AND NOT dest. */
        case 3:
        case 4:
        case 5:
            pxColor = pxColor >> shfCnt;
            pxBit = (pxBit >> shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = shiftMask;
                pxColor = lclPenColor;
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;
        /* X major top->bottom, right->left masked, AND NOT dest. */
        /* Diagonal top->bottom, right->left masked, AND NOT dest. */
        case 6:
        case 7:
            pxColor = pxColor << shfCnt;
            pxBit = (pxBit << shfCnt);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                if( shfCnt == 2 )
                {
                    pxBit = 0x03;
                    pxColor = lclPenColor >> 6;
                }
                else
                {
                    pxBit = 0x0f;
                    pxColor = lclPenColor >> 4;
                }
            }

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;

        }
    }
}

#endif /* INCLUDE_2_4_BIT */


