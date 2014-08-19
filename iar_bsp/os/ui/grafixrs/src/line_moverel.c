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
*  line_moverel.c
*
* DESCRIPTION
*
*  This file contains API - level line drawing function, MoveRel.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  MoveRel
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
*    MoveRel
*
* DESCRIPTION
*
*    Function MoveRel moves the current "pen" position the specified (DX,DY)
*    distance from the current screen position.
*
* INPUTS
*
*    INT32 argDX - Change in X coordinate to move to.
*
*    INT32 argDY - Change in Y coordinate to move to.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MoveRel(INT32 argDX, INT32 argDY )
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Pickup and increment */
    theGrafPort.pnLoc.X += argDX;
    theGrafPort.pnLoc.Y += argDY;

    MoveTo(theGrafPort.pnLoc.X, theGrafPort.pnLoc.Y);

    /* Return to user mode */
    NU_USER_MODE();
}
