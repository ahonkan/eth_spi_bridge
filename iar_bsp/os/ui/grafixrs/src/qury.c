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
*  qury.c                                                       
*
* DESCRIPTION
*
*  Query functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  QueryPosn
*  QueryX
*  QueryY
*
* DEPENDENCIES
*
*  rs_base.h
*  qury.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/qury.h"

/***************************************************************************
* FUNCTION
*
*    QueryPosn
*
* DESCRIPTION
*
*    Function QueryPosn returns the X and Y (ScrnX,ScrnY) screen coordinates of
*    the current "pen" position.
*
* INPUTS
*
*    INT32 * X - Pointer to X pen position.
*
*    INT32 * Y - Pointer to X pen position.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID QueryPosn( INT32 * X, INT32 * Y)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    *X = theGrafPort.pnLoc.X;
    *Y = theGrafPort.pnLoc.Y;
    
    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    QueryX
*
* DESCRIPTION
*
*    Function QueryX RETURNs the X screen coordinate of the current "pen" position.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns the X pen position.
*
***************************************************************************/
INT32 QueryX(VOID)
{
    INT32 value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    value = theGrafPort.pnLoc.X;

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    QueryY
*
* DESCRIPTION
*
*    Function QueryY RETURNs the Y screen coordinate of the current "pen" position.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns the Y pen position.
*
***************************************************************************/
INT32 QueryY(VOID)
{
    INT32 value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    value = theGrafPort.pnLoc.Y;

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}
