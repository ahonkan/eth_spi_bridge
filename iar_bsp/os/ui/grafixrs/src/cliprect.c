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
*  cliprect.c                                                  
*
* DESCRIPTION
*
*  This file contains the ClipRect function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ClipRect
*
* DEPENDENCIES
*
*  rs_base.h
*  coords.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/coords.h"

/***************************************************************************
* FUNCTION
*
*    ClipRect
*
* DESCRIPTION
*
*    Function ClipRect sets the clipping limits of the current rsPort to the
*    rectangle specified by cRECT. Rectangle cRECT is defined in the current ports
*    coordinate system.  The port clip rectangle is maintained in LOCAL
*    coordinates.
*
*    If cRECT is a null pointer, then the clip rect is disabled (set to the bitmap
*    limits).

*    Function CloseBitmap deallocates memory assigned to the grafMap.  It
*    returns 0 if successful or a GrafError style error code (and posts an error)
*    if not.
*
* INPUTS
*
*    rect *cRect - pointer to clipping rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ClipRect( rect *cRect)
{
    rect tmpRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( cRect == NU_NULL )
    {
        /* null pointer - make it bitmap limits */
        theGrafPort.portFlags = theGrafPort.portFlags & ~pfRecClip;
        thePort->portFlags = thePort->portFlags & ~pfRecClip;
    }
    else
    {
        tmpRect = *cRect;
        if( theGrafPort.portFlags & pfVirtual )
        {
            /* convert virtual rect to local */
            Vir2LclRect(&tmpRect);
        }

        /* set shadow port */
        theGrafPort.portClip = tmpRect;
        theGrafPort.portFlags |= pfRecClip;

        /* set user port */
        thePort->portClip   = tmpRect;
        thePort->portFlags |= pfRecClip;
    }

    /* convert to global and check port clip */
    COORDS_rsGblClip(thePort, &tmpRect);
    ViewClip = tmpRect;

    /* Return to user mode */
    NU_USER_MODE();

}
