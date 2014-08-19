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
*  m24b_lin.c
*
* DESCRIPTION
*
*  This file contains the thin line drawing functions for 24 bit memory
*  or Display (VGA or LCD).
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M24BL_ThinLine24Bit
*  M16BL_M24BURTable
*  M24BL_M24BMRTable
*  M24BL_M24BMOTable
*  M24BL_M24BUXTable
*  M24BL_M24BMXTable
*  M24BL_M24BMATable
*  M24BL_M24BMO_NDTable
*  M24BL_M24BMA_NDTable
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

/***************************************************************************
* FUNCTION
*
*    M24BL_ThinLine24Bit
*
* DESCRIPTION
*
*    Is a special case optimization for thin line drawing to 24 bit memory
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
VOID  M24BL_ThinLine24Bit(blitRcd *lineRec)
{
    lineS *listPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_TRUE;
    INT16 done           = NU_FALSE;

    /* Table used to NOT the source color according to raster op. */
    UINT8  FlipTable[16] = {0,    0,    0,    0xff,
                            0xff, 0xff, 0xff, 0,
                            0,    0,    0,    0,
                            0,    0xff, 0,    0xff};

    /* Table used to force the source color to 0xff according to raster op. */
    UINT8  ForceTableOr[16] = {0,    0,    0,    0,
                               0,    0,    0,    0,
                               0,    0,    0,    0,
                               0xff, 0,    0xff, 0};

    /* Table used to force the source color to 0 according to raster op. */
    UINT8  ForceTableAnd[16] = {0xff, 0xff, 0xff, 0xff,
                                0xff, 0xff, 0xff, 0xff,
                                0,    0xff, 0xff, 0xff,
                                0xff, 0xff, 0xff, 0xff};

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
        listPtr = (lineS *) lineRec->blitList;
        rectCnt     =  lineRec->blitCnt;
        lclPortMask =  lineRec->blitMask;
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
                UINT32 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(lineRec->blitBack, lclColr);

                lclPenColorR = (UINT8) (lclColr >> 16);
                lclPenColorG = (UINT8) ((lclColr >> 8) & 0xff);
                lclPenColorB = (UINT8) (lclColr & 0xff);
            }
        }
        else
        {
            UINT32 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(lineRec->blitFore, lclColr);

            /* pattern 1 is the only other possibility */
            lclPenColorR = (UINT8) (lclColr >> 16);
            lclPenColorG = (UINT8) ((lclColr >> 8) & 0xff);
            lclPenColorB = (UINT8) (lclColr & 0xff);
        }

        if( !done )
        {
            /* we don't care about transparency */
            if(dstClass <= 15)
            {
                /* NOT the color if appropriate */
                lclPenColorR = lclPenColorR ^ FlipTable[dstClass];
                lclPenColorG = lclPenColorG ^ FlipTable[dstClass];
                lclPenColorB = lclPenColorB ^ FlipTable[dstClass];

                /* make the color 0xff if appropriate */
                lclPenColorR = lclPenColorR | ForceTableOr[dstClass];
                lclPenColorG = lclPenColorG | ForceTableOr[dstClass];
                lclPenColorB = lclPenColorB | ForceTableOr[dstClass];

                /* make the color 0 if appropriate */
                lclPenColorR = lclPenColorR & ForceTableAnd[dstClass];
                lclPenColorG = lclPenColorG & ForceTableAnd[dstClass];
                lclPenColorB = lclPenColorB & ForceTableAnd[dstClass];
            }

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

            rowTablePtr[1] = (INT32  *) dstBmap->mapTable[0];

            /* look up the optimization routine */
            switch (dstClass)
            {
            /* LCD non-transparent */
            case 0:     /* zREPz   :           src           */
            case 4:     /* zNREPz  :        (NOT src)        */
            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                if (lclPortMask == 0xff)
                {
                    /* unmasked */
                    optPtr = &M24BL_M24BURTable;
                }
                else
                {
                    /* masked */
                    optPtr = &M24BL_M24BMRTable;
                }
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
                optPtr = &M24BL_M24BMOTable;
                break;

            case 2:     /* zXORz   :       src XOR dst       */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 14:    /* zINVERTz:        (NOT dst)        */
                if (lclPortMask == 0xff)
                {
                    /* unmasked */
                    optPtr = &M24BL_M24BUXTable;
                }
                else
                {
                    /* masked */
                    optPtr = &M24BL_M24BMXTable;
                }
                break;

            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M24BL_M24BMATable;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
                optPtr = &M24BL_M24BMO_NDTable;
                break;

            case 10:    /* zNOPz   :           dst <NOP>     */
                optPtr = &SCREENS_Nop;
                break;

            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M24BL_M24BMA_NDTable;
                break;
            case 32:
                optPtr = &M24BL_M24BMRTable;
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
*    M24BL_M24BURTable
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
VOID M24BL_M24BURTable(VOID)
{
    INT32 i;

    if( lineDir == 5 )
    {
        /* Low-level line drawing optimizations. */
        /* Unmasked low-level optimizations. */
        /* Horizontal left->right replace. */
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            *dstPtr++ = lclPenColorR;
            *dstPtr++ = lclPenColorG;
            *dstPtr++ = lclPenColorB;
        }

    }
    else
    {
        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

            *dstPtr++ = lclPenColorR;
            *dstPtr++ = lclPenColorG;
            *dstPtr   = lclPenColorB;

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
                    dRect.Xmin--;
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
                    dRect.Xmin++;
                }
            break;

            /* Diagonal top->bottom, left->right replace. */
            case 3:
                rowTablePtr[0]++;
                dRect.Xmin++;
            break;

            /* X major top->bottom, left->right replace. */
            case 4:
                dRect.Xmin++;

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
                dRect.Xmin--;
            break;

            /* X major top->bottom, right->left replace. */
            case 7:

                dRect.Xmin--;

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
*    M24BL_M24BMRTable
*
* DESCRIPTION
*
*    Masked replace table.
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
VOID M24BL_M24BMRTable(VOID)
{
    INT32 i;
#ifdef  GLOBAL_ALPHA_SUPPORT
    UINT8 *lclDstPtr;
#endif  /* GLOBAL_ALPHA_SUPPORT */

    /* set 1 bits to select the dest rather than the source */
    lclPortMask = ~lclPortMask;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

#ifdef  GLOBAL_ALPHA_SUPPORT

        lclDstPtr = dstPtr;
        
        if (grafBlit.blitRop == xAVGx)
        {
            *lclDstPtr = (((1-alpha_level) * lclPenColorR) + (alpha_level * (*lclDstPtr)));
            lclDstPtr++;

            *lclDstPtr = (((1-alpha_level) * lclPenColorG) + (alpha_level * (*lclDstPtr)));
            lclDstPtr++;

            *lclDstPtr = (((1-alpha_level) * lclPenColorB) + (alpha_level * (*lclDstPtr)));
        }

        else
            
#endif  /* GLOBAL_ALPHA_SUPPORT */

        {
            *dstPtr = (UINT8)((((*dstPtr ^ lclPenColorB) & lclPortMask) ^ lclPenColorR));
            dstPtr++;
            *dstPtr = (UINT8)((((*dstPtr ^ lclPenColorG) & lclPortMask) ^ lclPenColorG));
            dstPtr++;
            *dstPtr = (UINT8)((((*dstPtr ^ lclPenColorR) & lclPortMask) ^ lclPenColorB));
        }

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, replace. */
        case 0:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                dRect.Xmin--;
            }
        break;

        /* Masked low-level optimizations. */
        /* Y major top->bottom, left->right masked, replace. */
        /* Y major top->bottom, vertical masked, replace. */
        case 1:
        case 2:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                dRect.Xmin++;
            }
        break;

        /* X major top->bottom, left->right masked, replace. */
        /* Diagonal top->bottom, left->right masked, replace. */
        /* Horizontal left->right masked, replace. */
        case 3:
        case 4:
        case 5:
            dRect.Xmin++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;
        /* X major top->bottom, right->left masked, replace. */
        /* Diagonal top->bottom, right->left masked, replace. */
        case 6:
        case 7:
            dRect.Xmin--;

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

    /* restore the original setting */
    lclPortMask = ~lclPortMask;
}

/***************************************************************************
* FUNCTION
*
*    M24BL_M24BMOTable
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
VOID  M24BL_M24BMOTable(VOID )
{
    INT32 i;
    UINT8 lclMaskR, lclMaskG, lclMaskB;

    /* we'll leave all masked-off bits unchanged */
    lclMaskR = (UINT8)(lclPortMask & lclPenColorR);
    lclMaskG = (UINT8)(lclPortMask & lclPenColorG);
    lclMaskB = (UINT8)(lclPortMask & lclPenColorB);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

        *dstPtr = (*dstPtr | lclMaskR);
        dstPtr++;
        *dstPtr = (*dstPtr | lclMaskG);
        dstPtr++;
        *dstPtr = (*dstPtr | lclMaskB);

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
                dRect.Xmin--;
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
                dRect.Xmin++;
            }
        break;

        /* X major top->bottom, left->right masked, OR. */
        /* Diagonal top->bottom, left->right masked, OR. */
        /* Horizontal left->right masked, OR. */
        case 3:
        case 4:
        case 5:
            dRect.Xmin++;

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
            dRect.Xmin--;

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
*    M24BL_M24BUXTable
*
* DESCRIPTION
*
*    The XOR Table
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
VOID  M24BL_M24BUXTable(VOID )
{
    INT32 i;

    if( lineDir == 5 )
    {
        /*  Horizontal left->right XOR. */
        dstPtr = (UINT8  *) *rowTablePtr[0] + (3 * dRect.Xmin);

        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            *dstPtr = *dstPtr ^ lclPenColorR;
            dstPtr++;
            *dstPtr = *dstPtr ^ lclPenColorG;
            dstPtr++;
            *dstPtr = *dstPtr ^ lclPenColorB;
            dstPtr++;
        }
    }
    else
    {
        for( i = 0; i <= majorAxisLengthM1; i++)
        {
            dstPtr = (UINT8  *) *rowTablePtr[0] + (3 * dRect.Xmin);

            *dstPtr = *dstPtr ^ lclPenColorR;
            dstPtr++;
            *dstPtr = *dstPtr ^ lclPenColorG;
            dstPtr++;
            *dstPtr = *dstPtr ^ lclPenColorB;

            /* appropriate for the line direction */
            switch (lineDir)
            {
            /* Y major top->bottom, right->left XOR. */
            case 0:
                rowTablePtr[0]++;

                errTermL += errTermAdjUpL;
                if( errTermL >= 0 )
                {
                    /* minor axis advance */
                    errTermL -= errTermAdjDownL;
                    dRect.Xmin--;
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
                    dRect.Xmin++;
                }

            break;

            /* Diagonal top->bottom, left->right XOR. */
            case 3:
                rowTablePtr[0]++;
                dRect.Xmin++;
            break;

            /* X major top->bottom, left->right XOR. */
            case 4:
                dRect.Xmin++;

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
                dRect.Xmin--;
            break;

            /* X major top->bottom, right->left XOR. */
            case 7:
                dRect.Xmin--;

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
*    M24BL_M24BMXTable
*
* DESCRIPTION
*
*    The Masked XOR Table
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
VOID M24BL_M24BMXTable(VOID)
{
    INT32 i;
    UINT8 lclMaskR, lclMaskG, lclMaskB;

    /* we'll leave all masked-off bits unchanged */
    lclMaskR = (UINT8)(lclPortMask & lclPenColorR);
    lclMaskG = (UINT8)(lclPortMask & lclPenColorG);
    lclMaskB = (UINT8)(lclPortMask & lclPenColorB);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

        *dstPtr = (*dstPtr ^ lclMaskR);
        dstPtr++;
        *dstPtr = (*dstPtr ^ lclMaskG);
        dstPtr++;
        *dstPtr = (*dstPtr ^ lclMaskB);

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, XOR. */
        case 0:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                dRect.Xmin--;
            }
        break;

        /* Y major top->bottom, left->right masked, XOR. */
        /* Y major top->bottom, vertical masked, XOR. */
        case 1:
        case 2:
            rowTablePtr[0]++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                dRect.Xmin++;
            }
        break;
        /* X major top->bottom, left->right masked, XOR. */
        /* Diagonal top->bottom, left->right masked, XOR. */
        /* Horizontal left->right masked, XOR. */
        case 3:
        case 4:
        case 5:
            dRect.Xmin++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
                rowTablePtr[0]++;
            }
        break;

        /* X major top->bottom, right->left masked, XOR. */
        /* Diagonal top->bottom, right->left masked, XOR. */
        case 6:
        case 7:
            dRect.Xmin--;

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
*    M24BL_M24BMATable
*
* DESCRIPTION
*
*    The Masked AND Table
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
VOID  M24BL_M24BMATable(VOID )
{
    INT32 i;
    UINT8 lclMaskR, lclMaskG, lclMaskB;

    /* we'll leave all masked-off bits unchanged */
    lclMaskR = (UINT8)(lclPortMask & lclPenColorR);
    lclMaskG = (UINT8)(lclPortMask & lclPenColorG);
    lclMaskB = (UINT8)(lclPortMask & lclPenColorB);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

        *dstPtr = (*dstPtr & lclMaskR);
        dstPtr++;
        *dstPtr = (*dstPtr & lclMaskG);
        dstPtr++;
        *dstPtr = (*dstPtr & lclMaskB);

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
                dRect.Xmin--;
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
                dRect.Xmin++;
            }

        break;

        /* X major top->bottom, left->right masked, AND. */
        /* Diagonal top->bottom, left->right masked, AND. */
        /* Horizontal left->right masked, AND. */
        case 3:
        case 4:
        case 5:
            dRect.Xmin++;

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
            dRect.Xmin--;

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
*    M24BL_M24BMO_NDTable
*
* DESCRIPTION
*
*    The Masked OR NOT Destination Table
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
VOID M24BL_M24BMO_NDTable(VOID)
{
    INT32 i;
    UINT8 lclMaskR, lclMaskG, lclMaskB;

    /* we'll leave all masked-off bits unchanged */
    lclMaskR = (UINT8)(lclPortMask & lclPenColorR);
    lclMaskG = (UINT8)(lclPortMask & lclPenColorG);
    lclMaskB = (UINT8)(lclPortMask & lclPenColorB);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

        *dstPtr = (UINT8)((*dstPtr ^ lclPortMask) | lclMaskR);
        dstPtr++;
        *dstPtr = (UINT8)((*dstPtr ^ lclPortMask) | lclMaskG);
        dstPtr++;
        *dstPtr = (UINT8)((*dstPtr ^ lclPortMask) | lclMaskB);

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
                dRect.Xmin--;
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
                dRect.Xmin++;
            }

        break;

        /* X major top->bottom, left->right masked, OR NOT dest. */
        /* Diagonal top->bottom, left->right masked, OR NOT dest. */
        /* Horizontal left->right masked, OR NOT dest. */
        case 3:
        case 4:
        case 5:
            dRect.Xmin++;

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
            dRect.Xmin--;

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
*    M24BL_M24BMA_NDTable
*
* DESCRIPTION
*
*    The Masked AND NOT Destination Table
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
VOID  M24BL_M24BMA_NDTable(VOID )
{
    INT32 i;
    UINT8 lclMaskR, lclMaskG, lclMaskB;

    /* we'll leave all masked-off bits unchanged */
    lclMaskR = (UINT8)(lclPortMask & lclPenColorR);
    lclMaskG = (UINT8)(lclPortMask & lclPenColorG);
    lclMaskB = (UINT8)(lclPortMask & lclPenColorB);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
        dstPtr = (UINT8 *) *rowTablePtr[0] + (3 * dRect.Xmin);

        *dstPtr = (UINT8)((*dstPtr ^ lclPortMask) & lclMaskR);
        dstPtr++;
        *dstPtr = (UINT8)((*dstPtr ^ lclPortMask) & lclMaskG);
        dstPtr++;
        *dstPtr = (UINT8)((*dstPtr ^ lclPortMask) & lclMaskB);

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
                dRect.Xmin--;
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
                dRect.Xmin++;
            }
        break;

        /* X major top->bottom, left->right masked, AND NOT dest. */
        /* Diagonal top->bottom, left->right masked, AND NOT dest. */
        /* Horizontal left->right masked, AND NOT dest. */
        case 3:
        case 4:
        case 5:
            dRect.Xmin++;

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
            dRect.Xmin--;

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

#endif /* #ifdef INCLUDE_24_BIT */

