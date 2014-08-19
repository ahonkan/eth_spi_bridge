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
*  zect_screenrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - ScreenRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ScreenRect
*
* DEPENDENCIES
*
*  rs_base.h
*  zect.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/zect.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    ScreenRect
*
* DESCRIPTION
*
*    Function ScreenRect returns a rectangle set to the bounding limits
*    of the current grafPort.
*
* INPUTS
*
*    rect *SCRNRECT - Pointer to the resulting rectangle of the grafPort.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ScreenRect(rect *SCRNRECT)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( theGrafPort.portFlags & pfVirtual )
    {
        /* return virtual rect */
        *SCRNRECT = theGrafPort.portVirt;
    }
    else
    {
        *SCRNRECT = theGrafPort.portRect;
    }

    /* Return to user mode */
    NU_USER_MODE();
}
