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
*  pens.c                                                       
*
* DESCRIPTION
*
*  This file contains Pen related functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PENS_rsOvalLineGeneric
*  PenMode
*  BackColor
*
* DEPENDENCIES
*
*  rs_base.h
*  pens.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/pens.h"

/***************************************************************************
* FUNCTION
*
*    PENS_rsOvalLineGeneric
*
* DESCRIPTION
*
*    Function PENS_rsOvalLineGeneric is the generic line call handler for oval lines.  Converts 
*    blit record call to a polyline call.
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
VOID PENS_rsOvalLineGeneric(VOID)
{
    point tmpPoints[2];
    VOID (*ovPLPntr)();

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* already in global */
    globalLevel--;  

    /* set up the two points from the default blit list and call oval polyline */
    tmpPoints[0].X = grafBlist.Xmin;
    tmpPoints[0].Y = grafBlist.Ymin;
    tmpPoints[1].X = grafBlist.Xmax;
    tmpPoints[1].Y = grafBlist.Ymax;
    ovPLPntr       = (VOID (*)()) lineOvPolyIDV;
    ovPLPntr(2, &tmpPoints[0]);

    /* restore globalLevel */
    globalLevel++;  

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    PenMode
*
* DESCRIPTION
*
*    Function PENMODE sets the current rsPort's transfer mode for line drawing
*    operations, fills and blits.
*
* INPUTS
*
*    INT32 Mode - Mode for drawing.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PenMode(INT32 Mode ) 
{
    INT16 ErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( Mode > 31 )
    {
        ErrValue = c_RasterOp + c_BadRasOp;
        
        /* report error */
        nuGrafErr(ErrValue, __LINE__, __FILE__); 
        return;
    }
    theGrafPort.pnMode  = Mode;
    grafBlit.blitRop = Mode;
    thePort->pnMode  = Mode;

    /* Return to user mode */
    NU_USER_MODE();

    return;
}

/***************************************************************************
* FUNCTION
*
*    BackColor
*
* DESCRIPTION
*
*    Function BackColor sets the background color of the current rsPort 
*    (thePort.bkColor) to the specified COLOR value.
*
* INPUTS
*
*    INT32 argColor - Background color.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID BackColor(INT32 argColor)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    theGrafPort.bkColor  = argColor;
    grafBlit.blitBack = argColor;
    thePort->bkColor  = argColor;

    /* Return to user mode */
    NU_USER_MODE();

}
