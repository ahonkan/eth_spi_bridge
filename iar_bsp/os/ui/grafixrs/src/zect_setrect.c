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
*  zect_setrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - SetRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  SetRect
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
*    SetRect
*
* DESCRIPTION
*
*    Function SetRect stores the specified X1, Y1, X2, Y2 coordinates into the
*    specified rectangle R.
*
* INPUTS
*
*    rect *R  - Pointer to the rectangle.
*
*    INT32 X1 - Coordinate X1.
*
*    INT32 Y1 - Coordinate Y1.
*
*    INT32 X2 - Coordinate X2.
*
*    INT32 Y2 - Coordinate Y2.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetRect( rect *R, INT32 X1, INT32 Y1, INT32 X2, INT32 Y2)
{
    INT32 TemPt ;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( X2 < X1 )
    {
        TemPt = X1;
        X1  = X2;
        X2  = TemPt;
    }

    if( Y2 < Y1 )
    {
        /* Swap points */
        TemPt = Y1;
        Y1    = Y2;
        Y2    = TemPt;
    }

    /* Restore the value back to rect *R after calculation */
    R->Xmin = X1;
    R->Xmax = X2;
    R->Ymin = Y1;
    R->Ymax = Y2;

    /* Return to user mode */
    NU_USER_MODE();
}
