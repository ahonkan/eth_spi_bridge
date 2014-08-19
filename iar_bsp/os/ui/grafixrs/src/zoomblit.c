/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
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
*   zoomblit.c                                                  
*
* DESCRIPTION
*
*  This file contains the ZoomBlit function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ZoomBlit
*
* DEPENDENCIES
*
*  rs_base.h
*  zoomblit.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/zoomblit.h"
#include "ui/gfxstack.h"

extern SIGNED (*prGetPx)();

extern rsPort *CreateBitmap(INT32 aMEMTYPE, INT32 aWIDTH, INT32 aHEIGHT);
extern STATUS  CopyBlit( rsPort *srcPORT, rsPort *dstPORT, rect *argSrcR, rect *argDstR );
extern VOID    DestroyBitmap(rsPort *offPort);
extern VOID    GetPort( rsPort **gpPORT);
extern VOID    SetPort( rsPort *portPtr);

/* vector to GetPixel primitive for the destination bitmap */
SIGNED (*PixelPrimitive)(); 

/***************************************************************************
* FUNCTION
*
*    ZoomBlit
*
* DESCRIPTION
*
*    Function ZoomBlit transfers data between two different sized rectangles.
*    As ZoomBlit() uses getPixel & setpixel/fill primitives, different grafMap
*    color depths are OK.
*
*    A fixed point value is used to avoid accumulated coordinate errors, and
*    output coordinates are rounded down when drawn.
*
*    ZoomBlit does 1-n color translation, based on the target grafPort's
*    pen & backColor. As usual, the targets, clipArea, rasterOp, mask
*    etc. are used (& honored)
*
*    When scaling down the destination pixel value is determined by:
*
*    1) color-depth > 256 colors (no palette assumed)
*    The average pixel value (RGB value) of the source rectangle is used 
*
*    2) color-depth <= 256 (palette assumed)
*    The dominate value of the source rectangle is used.
*    If the source ports raster op is one of the transparent raster ops,
*    then the ports background color is never considered dominate, unless
*    it is the only value present in the source rectangle.
*
* INPUTS
*
*    rsPort *srcPORT - Pointer to the source port.
*
*    rsPort *dstPORT - Pointer to the destination port.
*
*    rect *argSrcR     - Pointer to the source rectangle.
*
*    rect *argDstR     - Pointer to the destination rectangle.
*
* OUTPUTS
*
*    INT32             - Returns 0 (always).
*
***************************************************************************/
INT32 ZoomBlit(rsPort *srcPORT ,rsPort *dstPORT, rect *argSrcR, rect *argDstR)
{
    INT16   JumpSprd_NextX = NU_FALSE;
    blitRcd blitRec;

    /* array of color indexes */
    UINT8   colorCnt[256];

    /* R,G, & B shift from word  */
    UINT8   Rshift;       
    UINT8   Gshift;
    UINT8   Bshift;

    /* Down using color or RGB */
    UINT16  ZoomDnVect;   

    /* total source colors */
    INT32   NColors;      

    /* pixel color */
    SIGNED  tmpColor;     

    /* current max color */
    UINT16  maxColor;     

    /* max color count */
    INT16   maxColorCnt;  

    /* red color count */
    INT16   redColorCnt;  

    /* green color count */
    INT16   greenColorCnt;

    /* blue color count */
    INT16   blueColorCnt; 

    /* RGB return value */
    UINT16  RGBrtn;       

    /* R,G, & B mask from word */
    UINT16  Rmask;        
    UINT16  Gmask;
    UINT16  Bmask;

    /* number of pixels in zoom area */
    SIGNED  totalPixels;  

    /* source pen mode */
    INT16   srcMode;      

    /* source width and height */
    INT32   srcDltX;      
    INT32   srcDltY;

    /* target width and height */
    INT32   dstDltX;      
    INT32   dstDltY;

    /* source back color */
    SIGNED  srcBack;      

    /* target pen and back color */
    SIGNED  dstPen;       
    SIGNED  dstBack;

    /* src "macro pixel" width/height */
    float   fSrcDX;       
    float   fSrcDY;

    /* dst "macro pixel" width/height */
    float   fDstDX;       
    float   fDstDY;

    /* float pnt source macro pixel */
    float   fSrcXmin;     
    float   fSrcYmin;
    float   fSrcXmax;
    float   fSrcYmax;

    /* float pnt dest macro pixel */
    float   fDstXmin;     
    float   fDstYmin;
    float   fDstXmax;
    float   fDstYmax;

    /* fixed pnt source macro pixel */
    INT32   SrcXmin;      
    INT32   SrcYmin;
    INT32   SrcXmax;
    INT32   SrcYmax;

    /* fixed pnt dest macro pixel */
    INT32   DstXmin;      
    INT32   DstYmin;
    INT32   DstXmax;
    INT32   DstYmax;

    /* global clip rect for target grafMap */
    rect    dstClipR;      

    /* global rect sizes */
    rect    GsrcRect;      
    rect    GdstRect;

    /* dest rectangle for blitRcd */
    rect    dstBR;         
    INT32   shfCnt;
    INT32   i;

    rsPort *scrnPort;

    rsPort *memPort;

    STATUS  status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* ========== Setup source info =============== */

    /* Save the current port */
    GetPort (&scrnPort);

    /* Create an in-memory bitmap to draw the contents of the */
    /* zoom rectangle */
    memPort = CreateBitmap(cMEMORY, argDstR->Xmax, argDstR->Ymax);
    
    /* Check if in-memory bitmap was created successfully. */
    if (memPort != NU_NULL)
    {
        /* get source rect and convert to global */
        Port2Gbl(argSrcR, srcPORT, &GsrcRect);
    
        /* source pen color */
        srcBack = srcPORT->bkColor; 
    
        /* source pen mode  */
        srcMode = srcPORT->pnMode;  
    
        /* also place grafMap in blitRcd */
        blitRec.blitSmap = srcPORT->portMap;
    
        /* setup vector for Zoom Down to color palette method (default) */
        ZoomDnVect = Zoom_Down_Color;
    
        /* get total number of colors in source bitmap */
        shfCnt = srcPORT->portMap->pixPlanes;
        if( shfCnt == 1 )
        {
            shfCnt = srcPORT->portMap->pixBits;
        }
    
        /* total source colors */
        NColors = (1 << (shfCnt)) - 1;  
        
        /* > 256 color (RGB) ? */
        if( NColors > 255 )
        {
            /* hard code values for R,G, & B mask and shift - will get from bitmap later */
            /* bits 10-14 (msb - not used) */
            Rmask = 0x7C00; 
    
            /* bits  5-9 */
            Gmask = 0x03E0; 
    
            /* bits  0-4 */
            Bmask = 0x001F; 
    
            Rshift = 0x0A;
            Gshift = 0x05;
            Bshift = 0x00;
    
            /* setup vector for Zoom Down to RGB method */
            ZoomDnVect = Zoom_Down_RGB; 
        }
    
        /* ========== Setup destination info ========== */
    
        /* get dest rect and convert to global */
        Port2Gbl(argDstR, dstPORT, &GdstRect);
        
        /* fill out blitRcd based on target grafPort */
        /* assume rect clip */
        blitRec.blitFlags = bfClipRect; 
        
#ifndef NO_REGION_CLIP
        
        /* are regions enabled ? */
        if( dstPORT->portFlags & pfRgnClip )
        {
            /* yes, clip region */
            blitRec.blitFlags |= bfClipRegn;
            blitRec.blitRegn   = dstPORT->portRegion;
        }
    
#endif  /* NO_REGION_CLIP */

        /* set raster op */
        blitRec.blitRop  = dstPORT->pnMode;   
    
        /* set plane mask */
        blitRec.blitMask = dstPORT->portMask; 
    
        /* solid pen */
        blitRec.blitPat  = 1;                 
    
        /* 1 rect */
        blitRec.blitCnt  = 1;               
    
        /* addr of rect */
        blitRec.blitList = (SIGNED) &dstBR; 
    
        /* set dest grafmap */
        blitRec.blitDmap = memPort->portMap;  
    
        /* set colors */
        dstBack = dstPORT->bkColor;           
        dstPen = dstPORT->pnColor;        
    
        /* set pointer to clipR */
        blitRec.blitClip = &dstClipR;         
    
        /* convert to global and check port clip */
        COORDS_rsGblClip(dstPORT, &dstClipR);        
    
        /* ======= Setup rectangles and 'macro pixels' =========== */
    
        /* Get width and height of source and destination rectangle           */
        /* global source width       */
        srcDltX = GsrcRect.Xmax - GsrcRect.Xmin; 
    
        /* global source height      */
        srcDltY = GsrcRect.Ymax - GsrcRect.Ymin; 
    
        /* global destination width  */
        dstDltX = GdstRect.Xmax - GdstRect.Xmin; 
    
        /* global destination height */
        dstDltY = GdstRect.Ymax - GdstRect.Ymin; 
    
        /* set source and destination "macro pixel" variables for X */
        if( srcDltX < dstDltX )
        {
            /* macro pix width = src width / dst width */
            fSrcDX = (float) srcDltX / (float) dstDltX;
    
            /* dst macro pix width = 1.0 */
            fDstDX = 1;
        }
        else
        {
            /* macro pix width = dst width / src width */
            fDstDX = (float) dstDltX / (float) srcDltX;
    
            /* src macro pix width = 1.0 */
            fSrcDX = 1;
        }
    
        /* set source and destination "macro pixel" variables for Y */
        if( srcDltY < dstDltY )
        {
            /* macro pix height = src height / dst height */
            fSrcDY = (float) srcDltY / (float) dstDltY;
    
            /* dst macro pix height = 1.0 */
            fDstDY = 1;
        }
        else
        {
            /* macro pix height = dst height / src height */
            fDstDY = (float) dstDltY / (float) srcDltY;
    
            /* src macro pix height = 1.0 */
            fSrcDY = 1;
        }
    
        /* initialize src and dest Ymin and Ymax */
        SrcYmin  = GsrcRect.Ymin;
        fSrcYmin = (float)SrcYmin;
        fSrcYmax = fSrcYmin + fSrcDY;
        SrcYmax  = (INT32)fSrcYmax; 
    
        DstYmin  = GdstRect.Ymin;
        fDstYmin = (float)DstYmin;
        fDstYmax = fDstYmin + fDstDY;
        DstYmax  = (INT32)fDstYmax; 
    
        /* ============= Actual ZOOM loop ======================= */
    
        /* spread each src "macro pix" row on each dest "macro pix row" */
        while( SrcYmax <= (GsrcRect.Ymax + 1) )
        {
            SrcXmin  = GsrcRect.Xmin;
            fSrcXmin = (float)SrcXmin;
            fSrcXmax = fSrcXmin + fSrcDX;
            SrcXmax  = (INT32)fSrcXmax;
            DstXmin  = GdstRect.Xmin;
            fDstXmin = (float)DstXmin;
            fDstXmax = fDstXmin + fDstDX;
            DstXmax  = (INT32) fDstXmax;
    
            while( SrcXmax <= (GsrcRect.Xmax + 1) )
            {
                /* ZOOM by filling the destination macro pixel with the source color */
    
                /* first thing to check is if we're zooming up or down */
                /* Note: this must be re-calculated each time */
                /* if either the height or width of the source "macro pixel" is
                   greater than 1, we are Zooming Down else, we're Zooming Up */
                if( (SrcXmax - SrcXmin + SrcYmax - SrcYmin) <= 2)
                {
                    /* =================== zoom up ========================== */
                    /* spread 1 source pixel to multiple destination pixels */
                     PixelPrimitive = srcPORT->portMap->prGetPx;
                     tmpColor = PixelPrimitive(SrcXmin, SrcYmin, blitRec.blitSmap);
                  
                }
                else
                {
                    /* =================== zoom down ========================== */
                    switch (ZoomDnVect)
                    {
                    case Zoom_Down_Color:
                        /* Color Zoom Down (monochrome, 16, and 256) tallies each
                           pixel's color in the source "macro pixel", and the color
                           with the most pixels is the predominant color */
    
                        /* clear color array */
                        for (i = 0; i <= NColors; i++)
                        {
                            colorCnt[i] = 0;
                        }
    
                        /* zero out max color count */
                        maxColorCnt = 0;
    
                        /* start at first row */
                        while( SrcYmin <= SrcYmax )
                        {
                            SrcXmin = (INT32) fSrcXmin;
                            
                            /* start at first column */
                            while( SrcXmin <= SrcXmax )
                            {
                                tmpColor = ( (blitRec.blitSmap->prGetPx)
                                    (SrcXmin, SrcYmin, blitRec.blitSmap) ) & 0xff;
    
                                /* inc column */
                                SrcXmin++;
    
                                /* check if src using transparency and color is back color */
                                if( !(srcMode & 0x0010) || (srcBack != tmpColor) )
                                {
                                    /* add to total for that color index */
                                    colorCnt[tmpColor]++;
    
                                    /* is this the new max color */
                                    if( maxColorCnt < (INT16)colorCnt[tmpColor] )
                                    {
                                        maxColorCnt = colorCnt[tmpColor];
                                        maxColor = (UINT16) tmpColor;
                                    }
                                }
                            }
    
                            /* next row */
                            SrcYmin++;
                        }
    
                        /* special check of if count is 0 */
                        if( maxColorCnt == 0 )
                        {
                            /* transparent src, all pixels backcolor */
                            /* set color to source backcolor */
                            tmpColor = srcBack;  
                        }
                        else
                        {
                            /* set color to source  max color */
                            tmpColor = maxColor; 
                        }
                        break;
    
                    case Zoom_Down_RGB:
                        /* RGB Zoom Down (greater than 255) adds all of the Reds,
                           Greens, and Blues for each pixel in the source "macro pixel",
                           and divides by number of pixels in the source "macro pixel"
                           to get the average */
    
                        /* zero Red value count   */
                        redColorCnt   = 0; 
    
                        /* zero Green value count */
                        greenColorCnt = 0; 
    
                        /* zero Blue value count  */
                        blueColorCnt  = 0; 
    
                        /* build the average RGB value from the total Red, Green,
                           and Blues */
                        totalPixels = (1 + (SrcXmax - SrcXmin)) * (1 + (SrcYmax - SrcYmin));
    
                        /* start at first row */
                        while( SrcYmin <= SrcYmax )
                        {
                            SrcXmin = (INT32) fSrcXmin;
    
                            /* start at first column */
                            while( SrcXmin <= SrcXmax )
                            {
                                RGBrtn = (UINT16) ((blitRec.blitSmap->prGetPx)
                                    (SrcXmin, SrcYmin, blitRec.blitSmap));
    
                                /* mask off red bits and add to running total */
                                redColorCnt += ((RGBrtn & Rmask) >> Rshift);
    
                                /* mask off green bits and add to running total */
                                greenColorCnt += ((RGBrtn & Gmask) >> Gshift);
    
                                /* mask off blue bits and add to running total */
                                blueColorCnt += ((RGBrtn & Bmask) >> Bshift);
    
                                /* inc column */
                                SrcXmin++;
                            }
    
                            /* next row */
                            SrcYmin++;
                        }
    
                        /* average red count   */
                        redColorCnt   /= (INT16) totalPixels; 
    
                        /* average green count */
                        greenColorCnt /= (INT16) totalPixels; 
    
                        /* average blue count  */
                        blueColorCnt  /= (INT16) totalPixels; 
                        tmpColor = (redColorCnt << Rshift) | (greenColorCnt <<
                                   Gshift) | (blueColorCnt << Bshift);
                    }
    
                    /* =============== now do fill ==========================*/
    
                    /* is the source is monochrome ? */
                    if( NColors == 1 )
                    {
                        /* color translate by seeing if source bit is off */
    
                        /* back color ? */
                        if( tmpColor > 0 )
                        {
                            tmpColor = dstPen;
                        }
                        else
                        {
                            /* is dest transparent ? */
                            if( blitRec.blitRop & 0x0010 )
                            {
                                /* if so, don't do fill */
                                JumpSprd_NextX = NU_TRUE; 
                            }
                            else
                            {
                                /* fill with dst back color */
                                tmpColor = dstBack;
                            }
                        }
                    }
                }
    
                if( !JumpSprd_NextX )
                {
                    blitRec.blitFore = tmpColor;
    
                    /* get global pixel values and set it into blitRcd */
                    dstBR.Xmin = DstXmin;
                    dstBR.Ymin = DstYmin;
                    dstBR.Xmax = DstXmax;
                    dstBR.Ymax = DstYmax;
    
                    /* draw the output rect */
                    (blitRec.blitDmap->prFill)(&blitRec);
                } /* if( !JumpSprd_NextX ) */
    
                /* add src "macro pix" width */
                fSrcXmin += fSrcDX;     
                SrcXmin = (INT32) fSrcXmin;
                fSrcXmax += fSrcDX;
                SrcXmax = (INT32) fSrcXmax;
    
                /* add dst "macro pix" width */
                fDstXmin += fDstDX;     
                DstXmin = (INT32) fDstXmin;
                fDstXmax += fDstDX;
                DstXmax = (INT32) fDstXmax;
            }
    
            /* add src "macro pix" height */
            fSrcYmin += fSrcDY;     
            SrcYmin = (INT32) fSrcYmin;
            fSrcYmax += fSrcDY;
            SrcYmax = (INT32) fSrcYmax;
    
            /* add dst "macro pix" height */
            fDstYmin += fDstDY;     
            DstYmin = (INT32) fDstYmin;
            fDstYmax += fDstDY;
            DstYmax = (INT32) fDstYmax;
        }
    
        /* Draw the in-memory bitmap to the screen */
        CopyBlit( memPort, dstPORT, &memPort->portRect, &dstPORT->portRect );

        /* destroy the bitmap to free the memory */
        DestroyBitmap( memPort );
    }
    else
    {
        /* TODO: proper status code */
        status = -1;
    }

    /* Set the port back to the original port that was used */
    /* when this function was entered */
    SetPort (scrnPort);

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}
