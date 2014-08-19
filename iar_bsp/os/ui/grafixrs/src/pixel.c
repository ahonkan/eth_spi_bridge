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
*  pixel.c                                                      
*
* DESCRIPTION
*
*  Contains the GetPixel and SetPixel functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  GetPixel
*  SetPixel
*
* DEPENDENCIES
*
*  rs_base.h
*  pixel.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/pixel.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    GetPixel
*
* DESCRIPTION
*
*    Function GetPixel returns the pixel value at the specified X,Y local
*    coordinate position.
*
* INPUTS
*
*    INT32 argX - X local coordinate position.
*
*    INT32 argY - Y local coordinate position.
*
* OUTPUTS
*
*    SIGNED - Returns the value of the pixel.
*
***************************************************************************/
SIGNED GetPixel( INT32 argX, INT32 argY)
{
    INT32 localX;
    INT32 localY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GP(argX, argY, &localX, &localY, 1);
    }
    else
    {
        /* update local pen location */
        localX = argX;
        localY = argY;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return ( (theGrafPort.portMap->prGetPx)(localX, localY, theGrafPort.portMap) );
}

/***************************************************************************
* FUNCTION
*
*    SetPixel
*
* DESCRIPTION
*
*    Function SetPixel sets the specified X,Y local coordinate position using
*    the current foreground pen color pnColor, pnMode, and pnPat.
*
* INPUTS
*
*    INT32 argX - X local coordinate position.
*
*    INT32 argY - Y local coordinate position.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetPixel( INT32 argX, INT32 argY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Get the screen semaphore */
    GFX_GetScreenSemaphore();

    if( theGrafPort.pnLevel >= 0 )
    {
        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GP(argX, argY, &LocX, &LocY, 1);
        }
        else
        {
            /* update global pen location */
            LocX = argX;
            LocY = argY;
        }

        grafBlist.Xmin = LocX;
        grafBlist.Ymin = LocY;

        /* set primitive can only handle replace, solid */
        if( ((grafBlit.blitRop & 0x0f) == 0) && (grafBlit.blitPat <= 1) )
        {
            (grafBlit.blitDmap->prSetPx)(&grafBlit);
        }
        else
        {
            /* use filler to set the pixel */
            /* build 1 pixel rect */
            grafBlist.Xmax = LocX + 1; 
            grafBlist.Ymax = LocY + 1;

            (grafBlit.blitDmap->prFill)(&grafBlit);
        }
    }

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}
