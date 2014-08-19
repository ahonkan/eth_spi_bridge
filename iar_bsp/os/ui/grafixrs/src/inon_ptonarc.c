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
*  inon_ptonarc.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether a point is on the given arc or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtOnArc
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
*    PtOnArc
*
* DESCRIPTION
*
*    Function PtOnArc determines whether the specified point, fpTESTPT,
*    is outside (FALSE), or on (TRUE) the arc inscribed in fpRect with line
*    width of sizX, sizY beginning at bgnANG degrees and continuing for
*    arcANG degrees.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    rect *fpRect    - Pointer to the fpRect.
*
*    INT16 bgn       - Arc begin angle.
*
*    INT16 angle     - Arc angle.
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is on the arc.
*                    - Returns FALSE if the point is not on the arc.
*
***************************************************************************/
INT32 PtOnArc(point *fpTESTPT, rect *fpRect, INT16 bgnANG, INT16 arcANG, INT32 sizX, INT32 sizY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, 0, sizX, sizY);
    VectSetup();

    RS_Arc_Draw( FRAME, fpRect,  bgnANG, arcANG, -1);
    VectRestore();
    
    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
