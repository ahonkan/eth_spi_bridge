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
*  zect_insetrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - InsetRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  InsetRect
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
*    InsetRect
*
* DESCRIPTION
*
*    Function InsetRect shrinks or expands the specified rectangle 'R'.
*    The left and right sides are moved toward the center by the amount specified
*    by DX; the top and bottom are moved to the center by the amount specified
*    DY.  If DX or DY are negative the appropriate pair of sides are moved
*    outward instead of inward.  The effect of INSETRECT is to alter the size
*    of rectangle 'R' by R*DX horizontally and 2*DY vertically, with the rectangle
*    center remaining centered at the same position.
*
* INPUTS
*
*    rect *R     - Pointer to the specified rectangle.
*
*    INT32 dltX - Change to rectangle width.
*
*    INT32 dltY - Change to rectangle height.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID InsetRect( rect *R , INT32 dltX , INT32 dltY)
{

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Restore the Value of X and Y with proper size */
    R->Xmin = R->Xmin + dltX;
    R->Xmax = R->Xmax - dltX; 
    R->Ymin = R->Ymin + dltY;
    R->Ymax = R->Ymax - dltY;

    /* Return to user mode */
    NU_USER_MODE();
}
