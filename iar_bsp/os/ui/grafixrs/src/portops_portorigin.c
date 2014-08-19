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
*  portops_portorigin.c                                                    
*
* DESCRIPTION
*
*  Port operation function - PortOrigin.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PortOrigin
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
*    PortOrigin
*
* DESCRIPTION
*
*    Function PortOrigin sets the ports "origin position" in either of two 
*    locations: upper-left corner (poTB = upperLeft), or lower-left corner  
*    (poTB = lowerLeft).
*
*    If poTB is set to upperLeft (1), the port origin is set to upper-left  
*    with +y values increasing from top to bottom.  
*
*    If poTB is set to lowerLeft (0), the port origin is set to lower-left  
*    with +y values increasing from bottom to top.  (In both cases X values 
*    increase from left to right.)
*
* INPUTS
*
*    INT32 poTB - upperLeft (1), the port origin is set to upper-left.
*                 lowerLeft (0), the port origin is set to lower-left.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PortOrigin(INT32 poTB)
{
    rect tmpRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( poTB == 0 )
    {
        /* lower left */
        thePort-> portFlags = thePort-> portFlags & ~pfUpper;
        theGrafPort.portFlags  = theGrafPort.portFlags & ~pfUpper;
    }
    else
    {
        /* upper left */
        thePort-> portFlags |= pfUpper;
        theGrafPort.portFlags  |= pfUpper;
    }

    /* update coordinate xform data */
    COORDS_rsGblCoord();

    /* convert to global and check port clip */
    COORDS_rsGblClip(thePort, &tmpRect);
    ViewClip = tmpRect;

    /* update locX,Y */
    COORDS_rsGblPenCoord();

    /* Return to user mode */
    NU_USER_MODE();
}
