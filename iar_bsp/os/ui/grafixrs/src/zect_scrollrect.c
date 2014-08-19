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
*  zect_scrollrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - ScrollRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ScrollRect
*
* DEPENDENCIES
*
*  rs_base.h
*  zect.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/zect.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    ScrollRect
*
* DESCRIPTION
*
*    Function ScrollRect moves the images bits contained in rectangle "R" a 
*    distance (DX,DY) within the current port.  Bits that are shifted beyond the 
*    current clipping limits are lost.  The port's background pattern (.bkPat) is 
*    used to fill the area voided by the rectangle move.
*
*    The blit is performed by calling the self 2 self blitter, the background 
*    rect(s) are likewise filled by direct primitive calls.
*
* INPUTS
*
*    rect *areaR  - Pointer to the image bits rectangle.
*
*    INT32 valDX - Delta x.
*
*    INT32 valDY - Delta y.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ScrollRect( rect *areaR, INT32 valDX, INT32 valDY)
{
    /* DX */
    INT32 ddx;          

    /* DY */
    INT32 ddy;          

    /* Clipping rectangle */
    rect cR;            
    rect tempAreaR;

    /* number of background fills required */
    INT32 voids;        

    /* source/dest rectangles */
    struct _rectlist1   
    {
        rect srcR;
        rect dR;
    } srcdstRect;

    /* void rectangles */
    struct _rectlist2   
    {
        rect Rect1;
        rect Rect2;
    } voidRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    tempAreaR = *areaR;
    cR = ViewClip;

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GR(tempAreaR, &srcdstRect.srcR, 0);
    }
    else
    {
        /* update global rectangle */
        srcdstRect.srcR = tempAreaR; 
    }

    /* Check to see how much is off the bitmap. If it's all off, leave */
    if( (InceptRect(&srcdstRect.srcR, &cR, &srcdstRect.dR)) )
    {
        /* srcR = dR */
        srcdstRect.srcR = srcdstRect.dR;    

        /* Convert deltas to global */
        ddx = valDX;                        
        ddy = valDY;

        if( !(theGrafPort.portFlags & pfUpper) )
        {
            /* if lower origin,direction is reversed */
            ddy = -ddy; 
        }

        if( theGrafPort.portFlags & pfVirtual )
        {
            /* Virtual To Global size */
            V2GSIZE(ddx, ddy, &ddx, &ddy); 
        }

        voids = ShiftRect(&srcdstRect.dR, ddx, ddy, &voidRect.Rect1, &voidRect.Rect2);
        if( voids != 0 )
        {

            /* grafBlit.blitRop  = zREPz */
            grafBlit.blitRop  = 0;                  

            /* blitRec.blitList = &blitData */
            grafBlit.blitList = (SIGNED) &srcdstRect; 

            /* Do the blit. By definition, uses the current grafPort & self2self blit */
            grafBlit.blitSmap = grafBlit.blitDmap;
            (grafBlit.blitSmap->prBlitSS)(&grafBlit);

            /* doing background pattern */
            grafBlit.blitPat  = theGrafPort.bkPat;     

            /* blitRec.blitList = &blitData */
            grafBlit.blitList = (SIGNED) &voidRect; 

            /* fill the void(s) */
            grafBlit.blitCnt  = voids;              

            (grafBlit.blitDmap->prFill)(&grafBlit);

            /* restore blitrcd */
            grafBlit.blitRop = theGrafPort.pnMode;
            grafBlit.blitPat = theGrafPort.pnPat;
            grafBlit.blitList = (SIGNED) &grafBlist;
            grafBlit.blitCnt = 1;
        }
    }

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}
