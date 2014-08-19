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
*  m16b_lin.c
*
* DESCRIPTION
*
*  This file contains the thin line drawing functions for 16 bit memory
*  or Display (VGA or LCD).
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  M16BL_ThinLine16Bit
*  M16BL_M16BURTable
*  M16BL_M16BMRTable
*  M16BL_M16BMOTable
*  M16BL_M16BUXTable
*  M16BL_M16BMXTable
*  M16BL_M16BMATable
*  M16BL_M16BMO_NDTable
*  M16BL_M16BMA_NDTable
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

#ifdef  GLOBAL_ALPHA_SUPPORT
extern UINT32 lclpnColr16;
#endif /* GLOBAL_ALPHA_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    M16BL_ThinLine16Bit
*
* DESCRIPTION
*
*    Is a special case optimization for thin line drawing to 16 bit memory
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
VOID M16BL_ThinLine16Bit(blitRcd *lineRec)
{
    lineS *listPtr;
    INT32 blitMayOverlap = NU_FALSE;
    INT32 isLine         = NU_TRUE;
    INT16 done           = NU_FALSE;

    /* Table used to NOT the source color according to raster op. */
    UINT16 FlipTable16[16] = {0,      0,      0,      0xffff,
                              0xffff, 0xffff, 0xffff, 0,
                              0,      0,      0,      0,
                              0,      0xffff, 0,      0xffff};

    /* Table used to force the source color to 0xffff according to raster op. */
    UINT16 ForceTableOr16[16] = {0,      0,    0,      0,
                                 0,      0,    0,      0,
                                 0,      0,    0,      0,
                                 0xffff, 0,    0xffff, 0};

    /* Table used to force the source color to 0 according to raster op. */
    UINT16 ForceTableAnd16[16] = {0xffff, 0xffff, 0xffff, 0xffff,
                                  0xffff, 0xffff, 0xffff, 0xffff,
                                  0,      0xffff, 0xffff, 0xffff,
                                  0xffff, 0xffff, 0xffff, 0xffff};

    /* set up the rectangular/region clip info */
    if( CLIP_Set_Up_Clip(lineRec, &cRect, blitMayOverlap, isLine) )
    {
        done = NU_TRUE;
    }
    if( !done )
    {
        
#ifndef NO_REGION_CLIP
                
        /* do we need to worry about clipping at all*/
        if( clipToRegionFlag == 0 )
                
#endif  /* NO_REGION_CLIP */
                
        {
            /* no--valid coordinates are guaranteed at a higher level */
            /* the clipping code expects an old-style clip rect */
            cRect.Xmax--;
            cRect.Ymax--;

            if ( cRect.Xmin != 0)
            {
                cRect.Xmin++;
            }

            if ( cRect.Ymin != 0)
            {
                cRect.Ymin++;
            }

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
        rectCnt     = lineRec->blitCnt;
        lclPortMask = lineRec->blitMask;
        dstClass    = lineRec->blitRop;

        /* pattern 0? */
        if( lineRec->blitPat == 0)
        {
            /* yes, use background color */
            if(dstClass & 0x10)
            {
                /* done if transparent */
                done = NU_TRUE;
            }

            if(!done )
            {
                UINT32 lclColr;

                /* Convert the color to target format. */
                COLOR_CONVERT(lineRec->blitBack, lclColr);

                lclPenColor16 = (UINT16)lclColr;
            }
        }
        else
        {
            UINT32 lclColr;

            /* Convert the color to target format. */
            COLOR_CONVERT(lineRec->blitFore, lclColr);

            /* pattern 1 is the only other possibility */
            lclPenColor16 = (UINT16)lclColr;
        }

        if( !done )
        {

            /* we don't care about transparency */

            if (dstClass <= 15)
            {

                /* NOT the color if appropriate */
                lclPenColor16 = lclPenColor16 ^ FlipTable16[dstClass];

                /* make the color 0xff if appropriate */
                lclPenColor16 = lclPenColor16 | ForceTableOr16[dstClass];

                /* make the color 0 if appropriate */
                lclPenColor16 = lclPenColor16 & ForceTableAnd16[dstClass];
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

            rowTablePtr[1] = (INT32 *)dstBmap->mapTable[0];

            /* look up the optimization routine */
            switch (dstClass)
            {
            /* LCD non-transparent */
            case 0:     /* zREPz   :           src           */
            case 4:     /* zNREPz  :        (NOT src)        */
            case 8:     /* zCLEARz :           0's           */
            case 12:    /* zSETz   :           1's           */
                if( lclPortMask == 0xffff)
                {
                    /* unmasked */
                    optPtr = &M16BL_M16BURTable;
                }
                else
                {
                    /* masked */
                    optPtr = &M16BL_M16BMRTable;
                }
                break;

            case 1:     /* zORz    :       src OR  dst       */
            case 5:     /* zNORz   :    (NOT src) OR  dst    */
                optPtr = &M16BL_M16BMOTable;
                break;

            case 2:     /* zXORz   :       src XOR dst       */
            case 6:     /* zNXORz  :    (NOT src) XOR dst    */
            case 14:    /* zINVERTz:        (NOT dst)        */
                if( lclPortMask == 0xffff)
                {
                    /* unmasked */
                    optPtr = &M16BL_M16BUXTable;
                }
                else
                {
                    /* masked */
                    optPtr = &M16BL_M16BMXTable;
                }
                break;

            case 3:     /* zNANDz  :    (NOT src) AND dst    */
            case 7:     /* zANDz   :       src AND dst       */
                optPtr = &M16BL_M16BMATable;
                break;

            case 9:     /* zORNz   :     src OR  (NOT dst)   */
            case 13:    /* zNORNz  : (NOT src) OR  (NOT dst) */
                optPtr = &M16BL_M16BMO_NDTable;
                break;

            case 10:    /* zNOPz   :           dst <NOP>     */
                optPtr = &SCREENS_Nop;
                break;

            case 11:    /* zANDNz  :     src AND (NOT dst)   */
            case 15:    /* zNANDNz : (NOT src) AND (NOT dst) */
                optPtr = &M16BL_M16BMA_NDTable;
                break;
            }

#ifndef NO_REGION_CLIP
                
            if( clipToRegionFlag != 0 )
            {
                /* handle region clipping in a separate loop */
                LineDrawer = &CLIP_ClipAndDrawEntry;
                CLIP_Line_Clip_Region( rectCnt, listPtr);
                nuResume( dstBmap );
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

                    drawStat = (signed char)listPtr->flStat;
                    listPtr++;

                     /* clip and draw the line */
                    CLIP_ClipAndDrawEntry();
                }
                nuResume(dstBmap);
            }
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
            ShowCursor();
#endif
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BURTable
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
VOID M16BL_M16BURTable(VOID)
{
    INT32 i;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    if( lineDir == 5 )
    {
        /* Low-level line drawing optimizations. */
        /* Unmasked low-level optimizations. */
        /* Horizontal left->right replace. */
#ifndef SMART_LCD
        dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
#endif /* SMART_LCD */
        for( i = 0; i <= majorAxisLengthM1; i++)
        {
#ifdef SMART_LCD
            /* Call the function to get the pixel. */
            SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + i, dRect.Ymin, &pixelValue);
            pixelValue = lclPenColor16;
            /* Call the function to set the pixel. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + i, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
            *dstPtr16 = lclPenColor16;
             dstPtr16++;
#endif /* SMART_LCD */
        }
    }
    else
    {
        for( i = 0; i <= majorAxisLengthM1; i++)
       {
#ifdef SMART_LCD
            /* Call the function to get the pixel. */
            SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
            pixelValue = lclPenColor16;
            /* Call the function to set the pixel. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
             dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
            *dstPtr16 = lclPenColor16;
#endif /* SMART_LCD */

            /* appropriate for the line direction */
            switch (lineDir)
            {

            /* Y major top->bottom, right->left replace. */
            case 0:
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
                break;

            /* Y major top->bottom, left->right replace. */
            case 2:
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                    dRect.Ymin++;
#else /* SMART_LCD */
                    rowTablePtr[0]++;
#endif /* SMART_LCD */
                }
            break;

            /* Diagonal top->bottom, right->left replace. */
            case 6:
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                    dRect.Ymin++;
#else /* SMART_LCD */
                    rowTablePtr[0]++;
#endif /* SMART_LCD */
                }
            break;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BMRTable
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
VOID M16BL_M16BMRTable(VOID)
{
    INT32 i;
    
#ifdef  GLOBAL_ALPHA_SUPPORT
        
    UINT16 pnColrR, pnColrG, pnColrB,lclRed,lclGreen,lclBlue;
    UINT16 tempcolor, TransR, TransB, TransG;

#endif  /* GLOBAL_ALPHA_SUPPORT */

#ifdef SMART_LCD
    UINT32 pixelValue;
#elif defined(GLOBAL_ALPHA_SUPPORT)
    UINT16 *lclDstPtr;
#endif /* SMART_LCD */

    /* set 1 bits to select the dest rather than the source */
    lclPortMask = ~lclPortMask;

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
#else /* SMART_LCD */
        dstPtr16  = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
#ifdef  GLOBAL_ALPHA_SUPPORT
        lclDstPtr = dstPtr16;
#endif /* GLOBAL_ALPHA_SUPPORT */
#endif /* SMART_LCD */
        
#ifdef  GLOBAL_ALPHA_SUPPORT
        
        if (grafBlit.blitRop == xAVGx)
        {
#ifdef CM565
            {
                /* set up local colors */
                pnColrR =(UINT16)(lclpnColr16 >> 11);
                pnColrG = (UINT16)((lclpnColr16 >> 5) & 0x3f);
                pnColrB = (UINT16)(lclpnColr16  & 0x1f);

#ifdef SMART_LCD
                lclRed = (UINT16)(pixelValue >> 11);
                lclGreen =(UINT16) ((pixelValue >> 5) & 0x3f);
                lclBlue = (UINT16)(pixelValue & 0x1f);
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

#ifdef SMART_LCD
                lclRed = (UINT16)((pixelValue >> 10) & 0x1f);
                lclGreen = (UINT16)((pixelValue >> 5) & 0x1f);
                lclBlue = (UINT16)(pixelValue & 0x1f);
#else /* SMART_LCD */
                lclRed = (UINT16)((*lclDstPtr >> 10) & 0x1f);
                lclGreen = (UINT16)((*lclDstPtr >> 5) & 0x1f);
                lclBlue = (UINT16)(*lclDstPtr & 0x1f);
#endif /* SMART_LCD */
            }
#endif

            TransR = (((1-alpha_level) * pnColrR) + (alpha_level * (lclRed)));
        
            TransG = (((1-alpha_level) * pnColrG) + (alpha_level * (lclGreen)));
        
            TransB = (((1-alpha_level) * pnColrB) + (alpha_level * (lclBlue)));
#ifdef CM565
            tempcolor = (TransR << 11);
            tempcolor = tempcolor | (TransG << 5);

#else

#ifdef SMART_LCD
            pixelValue = pixelValue >> 15;
            tempcolor = pixelValue << 15;
#else /* SMART_LCD */
            *lclDstPtr = *lclDstPtr >> 15;
            tempcolor = *lclDstPtr << 15;
#endif /* SMART_LCD */
            tempcolor = (TransR << 10);
            tempcolor = tempcolor | (TransG << 5);
#endif
            tempcolor = tempcolor | (TransB & 0x1f);

#ifdef SMART_LCD
            /* Call the function to set the pixel. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, tempcolor);
#else /* SMART_LCD */
            *dstPtr16 = tempcolor;
#endif /* SMART_LCD */
        }
        else

#endif  /* GLOBAL_ALPHA_SUPPORT */
            
        {
#ifdef SMART_LCD
            pixelValue = (((pixelValue ^ lclPenColor16) & lclPortMask) ^ lclPenColor16);
            /* Call the function to set the pixel. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
            *dstPtr16 = (UINT16)(( ( (*dstPtr16 ^ lclPenColor16) & lclPortMask) ^ lclPenColor16));
#endif /* SMART_LCD */
        }

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, replace. */
        case 0:
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
*    M16BL_M16BMOTable
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
VOID M16BL_M16BMOTable(VOID)
{
    INT32 i;
    UINT16 lclMask;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    /* we'll leave all masked-off bits unchanged */
    lclMask = (UINT16)(lclPortMask & lclPenColor16);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
        pixelValue = pixelValue|lclMask;
        /* Call the function to set the pixel. */
        SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
         dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
        *dstPtr16 = (*dstPtr16 | lclMask);
#endif /* SMART_LCD */

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, OR. */
        case 0:
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
            }
        break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BUXTable
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
VOID M16BL_M16BUXTable(VOID)
{
    INT32 i;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    if( lineDir == 5 )
    {
        /*  Horizontal left->right XOR. */
#ifndef SMART_LCD
        dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
#endif /* SMART_LCD */
        for( i = 0; i <= majorAxisLengthM1; i++)
        {
#ifdef SMART_LCD
            /* Call the function to get the pixel. */
            SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin + i, dRect.Ymin, &pixelValue);
            pixelValue = pixelValue ^ lclPenColor16;
            /* Call the function to set the pixel. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin + i, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
            *dstPtr16 = *dstPtr16 ^ lclPenColor16;
             dstPtr16++;
#endif /* SMART_LCD */
        }
    }
    else
    {
        for( i = 0; i <= majorAxisLengthM1; i++)
        {
#ifdef SMART_LCD
            /* Call the function to get the pixel. */
            SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
            pixelValue = pixelValue ^ lclPenColor16;
            /* Call the function to set the pixel. */
            SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
             dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
            *dstPtr16 = *dstPtr16 ^ lclPenColor16;
#endif /* SMART_LCD */

            /* appropriate for the line direction */
            switch (lineDir)
            {

            /* Y major top->bottom, right->left XOR. */
            case 0:
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
            break;

            /* Y major top->bottom, left->right XOR. */
            case 2:
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                    dRect.Ymin++;
#else /* SMART_LCD */
                    rowTablePtr[0]++;
#endif /* SMART_LCD */
                }
            break;

            /* Diagonal top->bottom, right->left XOR. */
            case 6:
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                    dRect.Ymin++;
#else /* SMART_LCD */
                    rowTablePtr[0]++;
#endif /* SMART_LCD */
                }
            break;
            }
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BMXTable
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
VOID M16BL_M16BMXTable(VOID)
{
    INT32 i;
    UINT16 lclMask;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    /* we'll leave all masked-off bits unchanged */
    lclMask = (UINT16)(lclPortMask & lclPenColor16);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
        pixelValue = pixelValue ^ lclMask;
        /* Call the function to set the pixel. */
        SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
         dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
        *dstPtr16 = (*dstPtr16 ^ lclMask);
#endif /* SMART_LCD */

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, XOR. */
        case 0:
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
            }
        break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BMATable
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
VOID M16BL_M16BMATable(VOID)
{
    INT32 i;
    UINT16 lclMask;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    /* we'll leave all masked-off bits unchanged */
    lclMask = (UINT16)((~lclPortMask) | lclPenColor16);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
        pixelValue = pixelValue & lclMask;
        /* Call the function to set the pixel. */
        SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
         dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
        *dstPtr16 = (*dstPtr16 & lclMask);
#endif /* SMART_LCD */

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, AND. */
        case 0:
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
            dRect.Xmin++;

            errTermL += errTermAdjUpL;
            if( errTermL >= 0 )
            {
                /* minor axis advance */
                errTermL -= errTermAdjDownL;
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
            }
        break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BMO_NDTable
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
VOID M16BL_M16BMO_NDTable(VOID)
{
    INT32 i;
    UINT16 lclMask;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    /* we'll leave all masked-off bits unchanged */
    lclMask = (UINT16)(lclPortMask & lclPenColor16);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
        pixelValue = (pixelValue ^ lclPortMask) | lclMask;
        /* Call the function to set the pixel. */
        SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
         dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
        *dstPtr16 = (UINT16)((*dstPtr16 ^ lclPortMask) | lclMask);
#endif /* SMART_LCD */

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, OR NOT dest. */
        case 0:
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
            }
        break;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    M16BL_M16BMA_NDTable
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
VOID M16BL_M16BMA_NDTable(VOID)
{
    INT32 i;
    UINT16 lclMask;
#ifdef SMART_LCD
    UINT32 pixelValue;
#endif /* SMART_LCD */

    /* we'll leave all masked-off bits unchanged */
    lclMask = (UINT16)((~lclPortMask) | lclPenColor16);

    for( i = 0; i <= majorAxisLengthM1; i++)
    {
#ifdef SMART_LCD
        /* Call the function to get the pixel. */
        SCREENI_Display_Device.display_get_pixel_hook(dRect.Xmin, dRect.Ymin, &pixelValue);
        pixelValue = (pixelValue ^ lclPortMask) & lclMask;
        /* Call the function to set the pixel. */
        SCREENI_Display_Device.display_set_pixel_hook(dRect.Xmin, dRect.Ymin, (UINT16)pixelValue);
#else /* SMART_LCD */
         dstPtr16 = (UINT16 *) *rowTablePtr[0] + dRect.Xmin;
        *dstPtr16 = (UINT16)((*dstPtr16 ^ lclPortMask) & lclMask);
#endif /* SMART_LCD */

        /* appropriate for the line direction */
        switch (lineDir)
        {

        /* Y major top->bottom, right->left masked, AND NOT dest. */
        case 0:
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
            dRect.Ymin++;
#else /* SMART_LCD */
            rowTablePtr[0]++;
#endif /* SMART_LCD */

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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
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
#ifdef SMART_LCD
                dRect.Ymin++;
#else /* SMART_LCD */
                rowTablePtr[0]++;
#endif /* SMART_LCD */
            }
        break;
        }
    }
}

#endif /* #ifdef INCLUDE_16_BIT */

