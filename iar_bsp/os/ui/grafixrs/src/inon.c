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
*  inon.c                                                       
*
* DESCRIPTION
*
*  Contains functions that check locations of points and rectangles.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtInRect
*  VectSetup
*  VectRestore
*  INON_rsRectInList
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  inon.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/inon.h"

/* The test rectangle */
static  struct _rect PtTstR;    

static  VOID  (*origFiller)();
static  INT32 orgPenFlags;

/***************************************************************************
* FUNCTION
*
*    PtInRect
*
* DESCRIPTION
*
*    Function PtInRect determines whether the specified point, fpTESTPT, with
*    added sizX and sizY, is in the specified rectangle argRect, and returns a
*    TRUE if it is or FALSE if is not.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    rect *argRect   - Pointer to the rectangle. 
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is in the rectangle.
*                    - Returns FALSE if the point is not in the rectangle.
*
***************************************************************************/
INT32 PtInRect(point *fpTESTPT, rect *argRect, INT32 sizX, INT32 sizY)
{
    /* temp trivial check rect */
    rect trivRect;  
    rect tempR;
    INT32 tempX;
    INT32 tempY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* initialize test result */
    PtRslt = 0; 

    /* Set up test rectangle. In order to match the pen, we follow the same
       rectangle fill rules. This means that an even sized pen will have
       the "extra" size at the right and maximum sides. */

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GP(fpTESTPT->X, fpTESTPT->Y, &tempX, &tempY, 1);
    }
    else
    {
        tempX = fpTESTPT->X;
        tempY = fpTESTPT->Y;

        /* Thin lines still count as a pixel sized line, so adjust fudge accordingly */
        if( sizX < 1 )
        {
            sizX = 1;
        }
        if( sizY < 1 )
        {
            sizY = 1;
        }
    }

    /* for consistency, do exactly what CenterRect does */
    PtTstR.Xmin = tempX - (sizX >> 1);
    PtTstR.Ymin = tempY - (sizY >> 1);
    PtTstR.Xmax = PtTstR.Xmin + sizX;
    PtTstR.Ymax = PtTstR.Ymin + sizY;

    if( argRect == 0 )
    {
        /* exit if no trivial check */
        PtRslt = 0; 
    }

    else
    {
        tempR = *argRect;
        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(tempR, &trivRect, 0);
        }
        else
        {
            trivRect = tempR;
        }

        /* we are checking the test rect "area" */
        PtTstR.Xmax--;  
        
        /* so decrement Xmax and Ymax */
        PtTstR.Ymax--;  

        if( (PtTstR.Xmax >= trivRect.Xmin) && (PtTstR.Xmin < trivRect.Xmax) &&
            (PtTstR.Ymax >= trivRect.Ymin) && (PtTstR.Ymin < trivRect.Ymax) )
        {
            /* put back Xmax and Ymax to the */
            PtTstR.Xmax++;  
            
            /* undecremented test rect */
            PtTstR.Ymax++;  
            
            /* set true flag */
            PtRslt = 1;     
        }

    } /* else */

    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}

/***************************************************************************
* FUNCTION
*
*    VectSetup
*
* DESCRIPTION
*
*    Function VectSetup is an internal function that sets the primitive vectors.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID VectSetup(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* save orig filler pointer */
    origFiller = theGrafPort.portMap->prFill;

    /* new filler */
    theGrafPort.portMap->prFill = (VOID (*)()) INON_rsRectInList;

    /* save orig pen flags */
    orgPenFlags = theGrafPort.pnFlags;
    if( !(theGrafPort.pnFlags & pnSizeFlg) )
    {
        /* Force lines to go wide so that they will call filler */
        /* modifies shadow port ONLY! */
        theGrafPort.pnSize.X = 1;  
        theGrafPort.pnSize.Y = 1;
        theGrafPort.pnFlags |= pnSizeFlg;
        SETLINESTYLE(theGrafPort.pnFlags);
    }

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    VectRestore
*
* DESCRIPTION
*
*    Function VectRestore is an internal function that restores the original 
*    primitive vectors.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID VectRestore(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( !(orgPenFlags & pnSizeFlg) )
    {
        /* Force lines to go wide so that they will call filler */
        /* modifies shadow port ONLY! */
        theGrafPort.pnSize.X = 0;  
        theGrafPort.pnSize.Y = 0;
        theGrafPort.pnFlags = orgPenFlags;
        SETLINESTYLE(theGrafPort.pnFlags);
    }

    /* new filler */
    theGrafPort.portMap->prFill = origFiller;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    INON_rsRectInList
*
* DESCRIPTION
*
*    This function replaces the filler during "PtIn/On" type operations.  It
*    obtains the rectangle to test against from grafdata, and places the result
*    there. Since a given function may call the filler multiple times, the filler
*    will increment the return value upon a "hit". A 0 is used to indicate the
*    starting value of "not in list". Should any part of the rectangle be in the
*    list, a hit is tallied. Notice that the test logic is backwards, ie it's
*    more likely to NOT be in the rect, so we try to stop testing soonest.
*
* INPUTS
*
*    blitRcd *FillRcd - FillRcd.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID INON_rsRectInList(blitRcd *FillRcd)
{
    INT32 numRect;
    rect *pntrRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* check hit counter; return if already found one */
    if( PtRslt != 1 ) 
    {
        /* how many rects in list */
        numRect = FillRcd->blitCnt; 
    
        /* get pointer to rect list */
        pntrRect = (rect *) FillRcd->blitList;

        /* skip if list is empty */
        while( numRect > 0 ) 
        {
            numRect--;
            if( (pntrRect->Ymin < PtTstR.Ymax) && (pntrRect->Ymax > PtTstR.Ymin) &&
                (pntrRect->Xmin < PtTstR.Xmax) && (pntrRect->Xmax > PtTstR.Xmin) )
            {   
                /* set hit counter */
                PtRslt = 1; 
                break; 
            }
            
            /* check next rectangle in list */
            pntrRect++; 
        }
    } /* if( PtRslt != 1 ) */

    /* Return to user mode */
    NU_USER_MODE();

}
