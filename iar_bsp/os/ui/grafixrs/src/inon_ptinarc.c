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
*  inon_ptinarc.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether a point is in the given arc or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtInArc
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
*    PtInArc
*
* DESCRIPTION
*
*    Function PtInArc simply replaces the filler with INON_rsRectInList, which
*    detects whether there is a match or not. This allows perfect coincidence with
*    the drawing routine, because the drawer routine is actually used.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    rect *fpRect    - Pointer to the fpRect.
*
*    INT16 bgn       - Arc begin angle.
*
*    INT16 arcangle     - Arc angle.
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is in the arc.
*                    - Returns FALSE if the point is not in the arc.
*
***************************************************************************/
INT32 PtInArc(point *fpTESTPT, rect *fpRect, INT16 bgn, INT16 arcangle, INT32 sizX, INT32 sizY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, fpRect, sizX, sizY);
    if( PtRslt )
    {
        VectSetup();
        RS_Arc_Draw( FILL, fpRect, bgn, arcangle, 1);
        VectRestore();
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
