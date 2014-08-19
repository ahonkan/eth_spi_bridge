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
*  pens_dashstyle.c
*
* DESCRIPTION
*
*  This file contains Pen related function, DashStyle.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  DashStyle
*
* DEPENDENCIES
*
*  rs_base.h
*  pens.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/pens.h"

/***************************************************************************
* FUNCTION
*
*    DashStyle
*
* DESCRIPTION
*
*    Function DashStyle selects either an on-off or a double dash pen style.
*
* INPUTS
*
*    INT32 DASHSTYL - The pen's style.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DashStyle(INT32 DASHSTYL)
{
    
#ifdef  DASHED_LINE_SUPPORT
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( DASHSTYL == dashDouble )
    {
        theGrafPort.pnFlags |= pnDashStyle;
    }
    else
    {
        theGrafPort.pnFlags &= ~pnDashStyle;
    }

    /* set the flags in the real port */
    thePort->pnFlags = theGrafPort.pnFlags;

    /* Return to user mode */
    NU_USER_MODE();

#else

    NU_UNUSED_PARAM(DASHSTYL);
    
#endif  /* DASHED_LINE_SUPPORT */
    
}
