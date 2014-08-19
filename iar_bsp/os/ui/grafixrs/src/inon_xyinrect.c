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
*  inon_xyinrect.c                                                       
*
* DESCRIPTION
*
*  Contains functions that check whether a point is in rectangle or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  XYInRect
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  inon.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/inon.h"



/***************************************************************************
* FUNCTION
*
*    XYInRect
*
* DESCRIPTION
*
*    Function XYInRect determines whether the position X,Y is included within the
*    specified rectangle argRect, and returns a TRUE if it is or FALSE if is not.
*
* INPUTS
*
*    INT32 valX - X value of the point to check.
*
*    INT32 valY - Y value of the point to check.
*
*    rect *varR - Pointer to the rectangle to check in.
*
* OUTPUTS
*
*    INT32      - Returns TRUE if the XY point is in the rectangle.
*               - Returns FALSE if the XY point is not in the rectangle.
*
***************************************************************************/
INT32 XYInRect(INT32 valX, INT32 valY, rect *varR)
{
    INT32 value = 0;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (valX >= varR->Xmin) && (valX <= varR->Xmax) &&
        (valY >= varR->Ymin) && (valY <= varR->Ymax) )
    {
        value = 1;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}
