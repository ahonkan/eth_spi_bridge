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
*  portops_portsize.c                                                    
*
* DESCRIPTION
*
*  Port operation function - PortSize.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PortSize
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  portops.h
*  fonti.h
*  rs_api.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/portops.h"
#include "ui/fonti.h"
#include "ui/rs_api.h"

/***************************************************************************
* FUNCTION
*
*    PortSize
*
* DESCRIPTION
*
*    Function PortSize changes the size of the current grafPort rectangle limits 
*    (portRect).  When changing the port size, the port origin-corner (upperleft/ 
*    lowerleft) remains at the same same position and the opposite corner is 
*    adjusted to the specified width and height dimensions.
*
*    If the port origin is "upperleft" (+Y values increasing from top to bottom), 
*    the top left corner of the port remains at same position and the bottom right 
*    corner is adjusted accordingly.
*
*    If the port origin is "lowerleft" (+Y values increasing from bottom to top), 
*    the bottom left corner of the port remains at the same position and the top 
*    right corner is adjusted.
*
* INPUTS
*
*    INT32 psWDX - Width.
*
*    INT32 psHTY - Height.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PortSize(INT32 psWDX,  INT32 psHTY)
{
    INT16   grafErrValue;
    SIGNED  oldDY;
    SIGNED  tempXY;
    rect    tmpRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( psWDX <= 0 )
    {
        /* invalid width */
        psWDX = 1;
        grafErrValue = c_PortSize + c_BadSize;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }

    if( psHTY <= 0 )
    {
        /* invalid height */
        psHTY = 1;
        grafErrValue = c_PortSize + c_BadSize;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }

    oldDY = thePort-> portRect.Ymax - thePort->portRect.Ymin;

    tempXY = thePort->portRect.Xmin + psWDX;
    if( tempXY > 32767 )
    {
        /* overflow */
        grafErrValue = c_PortSize + c_OfloPt;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        /* update user port */
        thePort->portRect.Xmax = tempXY;

        /* update shadow port */
        theGrafPort.portRect.Xmax = tempXY;

        tempXY = thePort->portRect.Ymin + psHTY;
        if( tempXY > 32767 )
        {
            /* overflow */
            grafErrValue = c_PortSize + c_OfloPt;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        }
        else
        {
            /* update user port */
            thePort->portRect.Ymax = tempXY;

            /* update shadow port */
            theGrafPort.portRect.Ymax = tempXY;
        }
    }

    if( !(thePort->portFlags & pfUpper) )
    {
        /* lower left origin */
        tempXY -= thePort->portRect.Ymin;
        oldDY -= tempXY;
        thePort->portOrgn.Y += oldDY;
        theGrafPort.portOrgn.Y += oldDY;
    }

    /* update coordinate xform data */
    COORDS_rsGblCoord();

    /* convert to global and check port clip */
    COORDS_rsGblClip(thePort, &tmpRect);
    ViewClip = tmpRect;

    /* Return to user mode */
    NU_USER_MODE();
}
