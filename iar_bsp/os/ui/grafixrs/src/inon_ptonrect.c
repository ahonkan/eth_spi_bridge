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
*  inon_ptonrect.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether a point is on the given rectangle or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtOnRect
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
*    PtOnRect
*
* DESCRIPTION
*
*    Function PtOnRect determines whether the specified point, fpTESTPT, is
*    outside (FALSE), or on (TRUE) the rect fpRect with line width of sizX, sizY.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    rect *argRect   - Pointer to the rectangle. 
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is on the rectangle.
*                    - Returns FALSE if the point is not on the rectangle.
*
***************************************************************************/
INT32 PtOnRect(point *fpTESTPT, rect *fpRect, INT32 sizX, INT32 sizY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, 0, sizX, sizY);
    VectSetup();

    RS_Rectangle_Draw( FRAME, fpRect, -1, 0 , 0);
    VectRestore();
    
    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
