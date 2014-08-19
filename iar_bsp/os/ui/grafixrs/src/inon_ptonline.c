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
*  inon_ptonline.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether a point is on the given line or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtOnLine
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
*    PtOnLine
*
* DESCRIPTION
*
*    Function PtOnLine determines whether the specified point, fpTESTPT, is
*    on the line segment between PT1 & PT2, and returns TRUE if it is or FALSE
*    if it is not.
*
*    PtOnLine treats the line as a rect structure. This allows the standard PtInRect
*    function set to do trivial rejection and handle the setup and exit. Notice the
*    trivial clip check is different for lines than the other ptin/on code,
*    because of the quadrant correction stuff.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    point *PT1      - Pointer to the first point of the line.
*
*    point *PT2      - Pointer to the second point of the line.
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is on the line.
*                    - Returns FALSE if the point is not on the line.
*
***************************************************************************/
INT32 PtOnLine(point *fpTESTPT, point *PT1, point *PT2, INT32 sizX, INT32 sizY)
{
    INT32 penX; /* place to stash current pen location */
    INT32 penY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, 0, sizX, sizY);
    VectSetup();

    /* preserve current pen location */
    penX = theGrafPort.pnLoc.X;
    penY = theGrafPort.pnLoc.Y;

    /* draw it */
    MoveTo(PT1->X, PT1->Y); 
    LineTo(PT2->X, PT2->Y);

    /* restore pen location */
    MoveTo(penX, penY);
    VectRestore();

    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
