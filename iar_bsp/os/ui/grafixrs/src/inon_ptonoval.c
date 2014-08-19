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
*  inon_ptonoval.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether a point is on the given oval or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtOnOval
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
*    PtOnOval
*
* DESCRIPTION
*
*    Function PtOnOval determines whether the specified point, fpTESTPT, is outside (FALSE), 
*    or on (TRUE) the oval inscribed in fpRect with line width of sizX, sizY.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    rect *fpRect    - Pointer to the rectangle. 
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is on the oval.
*                    - Returns FALSE if the point is not on the oval.
*
***************************************************************************/
INT32 PtOnOval(point *fpTESTPT, rect *fpRect, INT32 sizX, INT32 sizY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, 0, sizX, sizY);
    VectSetup();

    RS_Oval_Draw( FRAME, fpRect, -1);
    VectRestore();

    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
