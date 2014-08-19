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
*  line_moveto.c
*
* DESCRIPTION
*
*  This file contains API - level line drawing function, MoveTo.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  MoveTo
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
*    MoveTo
*
* DESCRIPTION
*
*    Function MOVETO moves the current "pen" position to the specified (X,Y)
*    screen coordinate.
*
* INPUTS
*
*    INT32 argX - X coordinate to move to.
*
*    INT32 argY - Y coordinate to move to.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MoveTo( INT32 argX, INT32 argY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* update shadow port */
    theGrafPort.pnLoc.X   = argX;
    theGrafPort.pnLoc.Y   = argY;

#ifdef  DASHED_LINE_SUPPORT
    
    /* reset the dash sequence to the start */
    theGrafPort.pnDashCnt = 0;    

#endif  /* DASHED_LINE_SUPPORT */
    
    /* update user port */
    thePort->pnLoc.X   = argX;
    thePort->pnLoc.Y   = argY;

#ifdef  DASHED_LINE_SUPPORT

    thePort->pnDashCnt = 0;

#endif  /* DASHED_LINE_SUPPORT */
    
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

    /* Return to user mode */
    NU_USER_MODE();
}

