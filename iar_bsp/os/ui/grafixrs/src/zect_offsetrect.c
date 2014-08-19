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
*  zect_offsetrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - OffsetRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  OffsetRect
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
*    OffsetRect
*
* DESCRIPTION
*
*    Function OffsetRect moves the specified rectangle by adding the delta-X
*    (DX) value to the rectangle 'R' X coordinates, and delta-Y (DY) values to the
*    rectangle 'R' Y coordinates.  The rectangle retains its shape and size, it is
*    simply offset to a different position on the coordinate plane.
*
* INPUTS
*
*    rect *R     - Pointer to the specified rectangle.
*
*    INT32 dltX - Change to rectangle starting x position.
*
*    INT32 dltY - Change to rectangle starting y position.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID OffsetRect( rect *R , INT32 dltX , INT32 dltY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    R->Xmin = R->Xmin + dltX;
    R->Xmax = R->Xmax + dltX;
    R->Ymin = R->Ymin + dltY;
    R->Ymax = R->Ymax + dltY;

    /* Return to user mode */
    NU_USER_MODE();
}
