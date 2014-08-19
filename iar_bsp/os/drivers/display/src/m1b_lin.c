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
*  m1b_lin.c
*
* DESCRIPTION
*
*  This file contains the thin line drawing functions for 1 bit memory
*  or Display (VGA or LCD).
*
* DATA STRUCTURES
*
*  Variables required are in regclipv.h.
*
* FUNCTIONS
*
*  M1BL_ThinLine1Bit
*  M1BL_M1BURTable
*  M1BL_M1BMOTable
*  M1BL_M1BUXTable
*  M1BL_M1BMATable
*  M1BL_M1BMO_NDTable
*  M1BL_M1BMA_NDTable
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

/***************************************************************************
* FUNCTION
*
*    M1BL_ThinLine1Bit
*
* DESCRIPTION
*
*    Is a special case optimization for thin line drawing to 1 bit memory
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
VOID M1BL_ThinLine1Bit(blitRcd *lineRec)
{
    lineS *listPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_TRUE;
    INT16 done           = NU_FALSE;

    /* Table used to NOT the source color according to raster op. */
    UINT8 FlipTable[16] = {0,    0,    0,    0x01,
                           0x01, 0x01, 0x01, 0,
                           0,    0,    0,    0,
                           0,    0x01, 0,    0x01};

    /* Table used to force the source color to 0x01 according to raster op. */
    UINT8 ForceTableOr[16] = {0,    0,    0,    0,
                              0,    0,    0,    0,
                              0,    0,    0,    0,
                              0x01, 0,    0x01, 0};

    /* Table used to force the source color to 0 according to raster op. */
    UINT8 ForceTableAnd[16] = {0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01,
                               0,    0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01};

    /* set up the rectangular/region clip info */
    if( CLIP_Set_Up_Clip(lineRec, &cRect, blitMayOverlap, isLine))
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
            /* no--valid coordinates are guaranteed at a higher level */
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
        listPtr     = (lineS *) lineRec->blitList;
        rectCnt     =  lineRec->blitCnt;
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
                COLOR_CONVERT((UINT8)lineRec->blitBack, lclColr);

                lclPenColor = lclColr;
            }
        }
        else
        {
            UINT8 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT((UINT8)lineRec->blitFore, lclColr);

            /* pattern 1 is the only other possibility */
            lclPenColor = lclColr;
        }

        if( !done )
        {
            if( lclPenColor != 0)
            {
                /* force color 0 or 1 */
                lclPenColor = 1;
            }

            /* we don't care about transparency */
            dstClass = dstClass & 0x0f;

            /* NOT the color if appropriate */
            lclPenColor = lclPenColor ^ FlipTable[dstClass];

            /* make the color 0x01 if appropriate */
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

            rowTablePtr[1] = (INT32 *) dstBmap->mapTable[0];

            /* look up the optimization routine */
            switch (dstClass)
            {
            /* Memory non-transparent */
            case 0:     /* zREPz   :           src           */
            case 4:     /* zNREPz  :        (NOT src)        */
            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                optPtr = &M1BL_M1BURTable;
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
                optPtr = &M1BL_M1BMOTable;
                break;

            case 2:     /* zXORz   :       src XOR dst       */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 14:    /* zINVERTz:        (NOT dst)        */
                optPtr = &M1BL_M1BUXTable;
                break;

            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M1BL_M1BMATable;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
                optPtr = &M1BL_M1BMO_NDTable;
                break;

            case 10:    /* zNOPz   :           dst <NOP>     */
                optPtr = &SCREENS_Nop;
                break;

            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M1BL_M1BMA_NDTable;
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
                nuResume(dstBmap);
            }
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
            ShowCursor();
#endif

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
*    M1BL_M1BURTable
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
VOID M1BL_M1BURTable(VOID)
{
    INT32 i;
    UINT8 pxBit;
#ifdef BIG_ENDIAN_GRFX
    pxBit      = (UINT8) (0x80 >> (dRect.Xmin & 0x07));
#else
    pxBit      = (UINT8) (0x01 << (dRect.Xmin & 0x07));
#endif
    dstBgnByte = dRect.Xmin >> 3;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;

        if( lclPenColor )
        {
            *dstPtr |= pxBit;
        }
        else
        {
            *dstPtr = *dstPtr & ~pxBit;
        }

        /* appropriate for the line direction */
        switch (lineDir)
        {
        case 0:
            /* Y major top->bottom, right->left replace. */
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif
            }
            break;
        case 1:
            /* Vertical top->bottom replace. */
            rowTablePtr[0]++;
            break;

        case 2:
            /* Y major top->bottom, left->right replace. */
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif
            }
            break;
        case 3:
            /* Diagonal top->bottom, left->right replace. */
            rowTablePtr[0]++;

#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x80;
            }
#else
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x01;
            }
#endif
            break;
        case 4:
            /* X major top->bottom, left->right replace. */

#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x80;
            }
#else
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x01;
            }
#endif

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
            break;
        case 5:
            /* Low-level line drawing optimizations. */
            /* Unmasked low-level optimizations.     */
            /* Horizontal left->right replace.       */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x80;
            }
#else
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x01;
            }
#endif
            break;
        case 6:
            /* Diagonal top->bottom, right->left replace. */
            rowTablePtr[0]++;

#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x01;
            }
#else
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x80;
            }
#endif
            break;
        case 7:
            /* X major top->bottom, right->left replace. */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x01;
            }
#else
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x80;
            }
#endif

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
            break;
        default:
            break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BL_M1BMOTable
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
VOID M1BL_M1BMOTable(VOID)
{
    INT16  done = NU_FALSE;
    INT32  i;
    UINT8  pxBit;

    if( lclPenColor == 0)
    {
        /* nothing to do */
        done = NU_TRUE;
    }

    if(!done)
    {
#ifdef BIG_ENDIAN_GRFX
        pxBit      = (UINT8) (0x80 >> (dRect.Xmin & 0x07));
#else
        pxBit      = (UINT8) (0x01 << (dRect.Xmin & 0x07));
#endif
        dstBgnByte = dRect.Xmin >> 3;

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
             dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
            *dstPtr = *dstPtr | pxBit;

            /* appropriate for the line direction */
            switch (lineDir)
            {
            case 0: /* Y major top->bottom, right->left masked, OR. */
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                    pxBit = (pxBit << 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        pxBit = 0x01;
                    }
#else
                    pxBit = (pxBit >> 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        pxBit = 0x80;
                    }
#endif
                }
                break;
            case 1: /* Y major top->bottom, left->right masked, OR. */
            case 2: /* Y major top->bottom, vertical masked, OR. */
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                    pxBit = (pxBit >> 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = 0x80;
                    }
#else
                    pxBit = (pxBit << 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = 0x01;
                    }
#endif
                }
                break;
            case 3: /* X major top->bottom, left->right masked, OR. */
            case 4: /* Diagonal top->bottom, left->right masked, OR. */
            case 5: /* Horizontal left->right masked, OR. */
#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
                break;
            case 6: /* X major top->bottom, right->left masked, OR. */
            case 7: /* Diagonal top->bottom, right->left masked, OR. */
#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
                break;
            default:
                break;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BL_M1BUXTable
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
VOID M1BL_M1BUXTable(VOID)
{
    INT16  done = NU_FALSE;
    INT32  i;
    UINT8  pxBit;

    if(lclPenColor == 0)
    {
        /* nothing to do */
        done = NU_TRUE;
    }

    if ( !done )
    {
#ifdef BIG_ENDIAN_GRFX
        pxBit      = (UINT8) (0x80 >> (dRect.Xmin & 0x07));
#else
        pxBit      = (UINT8) (0x01 << (dRect.Xmin & 0x07));
#endif
        dstBgnByte = dRect.Xmin >> 3;

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            /* appropriate for the line direction */
            switch (lineDir)
            {
            case 0: /* Y major top->bottom, right->left XOR. */
                dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                    pxBit = (pxBit << 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        pxBit = 0x01;
                    }
#else
                    pxBit = (pxBit >> 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        pxBit = 0x80;
                    }
#endif

                }
                break;

            case 1: /* Vertical top->bottom XOR. */
                 dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

                rowTablePtr[0]++;
                break;

            case 2: /* Y major top->bottom, left->right XOR. */
                 dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                    pxBit = (pxBit >> 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = 0x80;
                    }
#else
                    pxBit = (pxBit << 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = 0x01;
                    }
#endif
                }
                break;

            case 3: /* Diagonal top->bottom, left->right XOR. */
                 dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

                rowTablePtr[0]++;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif
                break;

            case 4: /* X major top->bottom, left->right XOR. */
                 dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
                break;

            case 5: /*  Horizontal left->right XOR. */
                *dstPtr ^= pxBit;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif

                break;

            case 6: /* Diagonal top->bottom, right->left XOR. */
                 dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

                rowTablePtr[0]++;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif
                break;
            case 7: /* X major top->bottom, right->left XOR. */
                 dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
                *dstPtr = *dstPtr ^ pxBit;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if(pxBit == 0)
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
                break;

            default:
                break;

            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BL_M1BMATable
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
VOID M1BL_M1BMATable(VOID)
{
    INT16  done = NU_FALSE;
    INT32  i;
    UINT8  pxBit;

    if( lclPenColor == 1)
    {
        /* nothing to do */
        done = NU_TRUE;
    }

    if( !done )
    {
#ifdef BIG_ENDIAN_GRFX
        pxBit      = (UINT8) (0x80 >> (dRect.Xmin & 0x07));
#else
        pxBit      = (UINT8) (0x01 << (dRect.Xmin & 0x07));
#endif
        dstBgnByte = dRect.Xmin >> 3;

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
             dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
            *dstPtr = *dstPtr & ~pxBit;

            /* appropriate for the line direction */
            switch (lineDir)
            {
            case 0:  /* Y major top->bottom, right->left masked, AND. */
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                    pxBit = (pxBit << 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        pxBit = 0x01;
                    }
#else
                    pxBit = (pxBit >> 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte--;
                        pxBit = 0x80;
                    }
#endif
                }
                break;

            case 1: /* Y major top->bottom, left->right masked, AND. */
            case 2: /* Y major top->bottom, vertical masked, AND. */

                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                    pxBit = (pxBit >> 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = 0x80;
                    }
#else
                    pxBit = (pxBit << 1);
                    if( pxBit == 0 )
                    {
                        dstBgnByte++;
                        pxBit = 0x01;
                    }
#endif
                }
                break;

            case 3: /* X major top->bottom, left->right masked, AND. */
            case 4: /* Diagonal top->bottom, left->right masked, AND. */
            case 5: /* Horizontal left->right masked, AND. */

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
                break;

            case 6: /* X major top->bottom, right->left masked, AND. */
            case 7: /* Diagonal top->bottom, right->left masked, AND. */

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    rowTablePtr[0]++;
                }
                break;

            default:
                break;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BL_M1BMO_NDTable
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
VOID M1BL_M1BMO_NDTable(VOID)
{
    INT32 i;
    UINT8 pxBit;

#ifdef BIG_ENDIAN_GRFX
    pxBit      = (UINT8) (0x80 >> (dRect.Xmin & 0x07));
#else
    pxBit      = (UINT8) (0x01 << (dRect.Xmin & 0x07));
#endif
    dstBgnByte = dRect.Xmin >> 3;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
         dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
        *dstPtr = *dstPtr ^ pxBit;

        if( lclPenColor )
        {
            *dstPtr = *dstPtr | pxBit;
        }

        /* appropriate for the line direction */
        switch (lineDir)
        {
        case 0: /* Y major top->bottom, right->left masked, OR NOT dest. */
           rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif
            }
            break;

        case 1: /* Y major top->bottom, left->right masked, OR NOT dest. */
        case 2: /* Y major top->bottom, vertical masked, OR NOT dest. */
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif
            }
            break;

        case 3: /* Diagonal top->bottom, left->right masked, OR NOT dest. */
        case 4: /* Horizontal left->right masked, OR NOT dest. */
        case 5: /* X major top->bottom, left->right masked, OR NOT dest. */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x80;
            }
#else
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x01;
            }
#endif

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
            break;

        case 6: /* X major top->bottom, right->left masked, OR NOT dest. */
        case 7: /* Diagonal top->bottom, right->left masked, OR NOT dest. */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x01;
            }
#else
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x80;
            }
#endif

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
            break;

        default:
            break;

        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M1BL_M1BMA_NDTable
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
VOID M1BL_M1BMA_NDTable(VOID)
{
    INT32  i;
    UINT8 pxBit;


#ifdef BIG_ENDIAN_GRFX
    pxBit      = (UINT8) (0x80 >> (dRect.Xmin & 0x07));
#else
    pxBit      = (UINT8) (0x01 << (dRect.Xmin & 0x07));
#endif
    dstBgnByte = dRect.Xmin >> 3;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
         dstPtr = (UINT8 *) *rowTablePtr[0] + dstBgnByte;
        *dstPtr = *dstPtr ^ pxBit;
        if( !(lclPenColor) )
        {
            *dstPtr = *dstPtr & ~pxBit;
        }

        /* appropriate for the line direction */
        switch (lineDir)
        {
        case 0: /* Y major top->bottom, right->left masked, AND NOT dest. */
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x01;
                }
#else
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte--;
                    pxBit = 0x80;
                }
#endif
            }
            break;

        case 1: /* Y major top->bottom, left->right masked, AND NOT dest. */
        case 2: /* Y major top->bottom, vertical masked, AND NOT dest. */
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;

#ifdef BIG_ENDIAN_GRFX
                pxBit = (pxBit >> 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x80;
                }
#else
                pxBit = (pxBit << 1);
                if( pxBit == 0 )
                {
                    dstBgnByte++;
                    pxBit = 0x01;
                }
#endif
            }
            break;

        case 3: /* Diagonal top->bottom, left->right masked, AND NOT dest. */
        case 4: /* Horizontal left->right masked, AND NOT dest. */
        case 5: /* X major top->bottom, left->right masked, AND NOT dest. */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x80;
            }
#else
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte++;
                pxBit = 0x01;
            }
#endif

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
            break;
        case 6: /* X major top->bottom, right->left masked, AND NOT dest. */
        case 7: /* Diagonal top->bottom, right->left masked, AND NOT dest. */
#ifdef BIG_ENDIAN_GRFX
            pxBit = (pxBit << 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x01;
            }
#else
            pxBit = (pxBit >> 1);
            if( pxBit == 0 )
            {
                dstBgnByte--;
                pxBit = 0x80;
            }
#endif

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
            break;

        default:
            break;

        }
    }
}

#endif /* INCLUDE_1_BIT */

