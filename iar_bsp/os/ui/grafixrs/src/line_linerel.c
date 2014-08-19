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
*  line_linerel.c
*
* DESCRIPTION
*
*  This file contains API - level line drawing function LineRel.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  LineRel
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  line.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/line.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    LineRel
*
* DESCRIPTION
*
*    Function LineRel draws a "LineStyle" line from the current "pen" position
*    a specified distance (DX,DY) using the predefined "RasterOp"
*    write operation.  Line specifications which extend beyond the current
*    viewport limits are automatically clipped appropriately.  The current "pen"
*    position is updated to the ending screen coordinates.
*
* INPUTS
*
*    INT32 argDX - Change in end X coordinate of line to draw.
*
*    INT32 argDY - Change in end Y coordinate of line to draw.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID LineRel(INT32 valDX, INT32 valDY )
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Pickup and increment */
    theGrafPort.pnLoc.X += valDX;
    theGrafPort.pnLoc.Y += valDY;

    LineTo(theGrafPort.pnLoc.X, theGrafPort.pnLoc.Y);

    /* Return to user mode */
    NU_USER_MODE();
}
