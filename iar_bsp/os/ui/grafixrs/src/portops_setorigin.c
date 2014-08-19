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
*  portops_setorigin.c                                                    
*
* DESCRIPTION
*
*  Port operation function - SetOrigin.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  SetOrigin
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
*    SetOrigin
*
* DESCRIPTION
*
*    Function SetOrigin sets the origin of the local coordinate system for
*    the current grafPort. SetOrigin() does not affect the immediate screen
*    image, nor the location and size of the grafPort; it affects where subsequent
*    writing operations appear within the grafPort.  SetOrigin updates the
*    coordinates of the portRect and clipRect limits.  Dependent the the port
*    origin location, upperleft or lowerleft, the SetOrigin SOX,SOY values define
*    the respective local coordinate values.  For example:
*
*    If the port origin is set as upperleft, SOX,SOY defines the local top-left 
*    corner coordinate of the grafPort.
*
*    If the port origin is set as lowerleft, SOX,SOY defines the local bottom-left
*    corner coordinate of the grafPort.
*
* INPUTS
*
*    INT32 SOX - Defines the local left-top coordinate.
*
*    INT32 SOY - Defines the local left-bottom coordinate.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetOrigin(INT32 SOX, INT32 SOY)
{
    INT32 wthX;
    INT32 hgtY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    wthX = thePort->portRect.Xmax - thePort->portRect.Xmin + SOX;
    hgtY = thePort->portRect.Ymax - thePort->portRect.Ymin + SOY;

    thePort->portRect.Xmin = SOX;
    thePort->portRect.Xmax = wthX;
    thePort->portRect.Ymin = SOY;
    thePort->portRect.Ymax = hgtY;

    theGrafPort.portRect.Xmin = SOX;
    theGrafPort.portRect.Xmax = wthX;
    theGrafPort.portRect.Ymin = SOY;
    theGrafPort.portRect.Ymax = hgtY;
                    ;
    wthX = thePort->portClip.Xmax - thePort->portClip.Xmin + SOX;
    hgtY = thePort->portClip.Ymax - thePort->portClip.Ymin + SOY;

    thePort->portClip.Xmin = SOX;
    thePort->portClip.Xmax = wthX;
    thePort->portClip.Ymin = SOY;
    thePort->portClip.Ymax = hgtY;

    theGrafPort.portClip.Xmin = SOX;
    theGrafPort.portClip.Xmax = wthX;
    theGrafPort.portClip.Ymin = SOY;
    theGrafPort.portClip.Ymax = hgtY;

    /* update coordinate xform data */
    COORDS_rsGblCoord();

    /* update locX,Y */
    COORDS_rsGblPenCoord();

    /* Return to user mode */
    NU_USER_MODE();
}
