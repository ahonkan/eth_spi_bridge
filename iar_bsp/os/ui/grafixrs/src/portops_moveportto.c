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
*  portops_moveportto.c                                                    
*
* DESCRIPTION
*
*  Port operation function - MovePortTo.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  MovePortTo
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
*    MovePortTo
*
* DESCRIPTION
*
*    Function MovePortTo moves the position of the current grafPort to a new 
*    location on the bitmap.  Note that MovePortTo does not immediately affect the 
*    screen image.  It only affects the location of subsequent write operations to 
*    the port.  If you are actually moving the location of an existing visible 
*    port, you will need to use both procedures CopyBlit() to move the visible port 
*    image, and MovePortTo to set the new port location for subsequent write 
*    operations.
*
*    gblLEFT and gblTOP define the top-left corner of the grafPort on the
*    bitmap in global (pixel) coordinates (are independent of the
*    port's local coordinate system and local axis orientations).  gblLEFT
*    and gblTOP define global X and Y coordinates from the bitmap's
*    top-left corner position.
*
*    Any existing regions used in this port will have to be offset manually.
*
* INPUTS
*
*    INT32 gblLEFT - X value of top-left corner of the grafPort on the
*                    bitmap in global (pixel) coordinates.
*
*    INT32 gblTOP  - Y value of top-left corner of the grafPort on the
*                    bitmap in global (pixel) coordinates.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MovePortTo(INT32 gblLEFT, INT32 gblTOP)
{
    rect tmpRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set user port */
    thePort->portOrgn.X = gblLEFT;

    /* set shadow port */
    theGrafPort.portOrgn.X = gblLEFT;

    thePort->portOrgn.Y = gblTOP;
    theGrafPort.portOrgn.Y = gblTOP;

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
