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
*  polygons_fillrule.c
*
* DESCRIPTION
*
*  Contains polygon support function, FillRule.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  FillRule
*
* DEPENDENCIES
*
*  rs_base.h
*  globalrsv.h
*  polygons.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/globalrsv.h"
#include "ui/polygons.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    FillRule
*
* DESCRIPTION
*
*    Function FillRule sets the port's current portFlags.pfFillRule bit to
*    to the specified POLYRULE setting.
*
* INPUTS
*
*    INT32 POLYRULE - POLYRULE setting.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID FillRule(INT32 POLYRULE)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    theGrafPort.portFlags = theGrafPort.portFlags & ~pfFillRule;

    if( POLYRULE != 0 )
    {
        theGrafPort.portFlags |= pfFillRule;
    }

    /* update user port */
    thePort->portFlags = theGrafPort.portFlags;

    /* Return to user mode */
    NU_USER_MODE();

}
