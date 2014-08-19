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
*  inon_ptinflood.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether point is in flood area or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtInFlood
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
*    PtInFlood
*
* DESCRIPTION
*
*    PtInFlood simply replaces the filler with INON_rsRectInList, 
*    which detects whether there is a match or not. This allows
*    perfect coincidence with the drawing routine, since the drawer 
*    routine is actually used.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    point *seedPt   - Pointer to the seed point.
*
*    rect *fpRect    - Pointer to the fpRect.
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is in the flood area.
*                    - Returns FALSE if the point is not in flood area.
*
***************************************************************************/
INT32 PtInFlood(point *fpTESTPT, point *seedPt, rect *fpRect, INT32 sizX, INT32 sizY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, fpRect, sizX, sizY);

    if( PtRslt )
    {
        VectSetup();

        RS_OptionalFiller( FLOOD, seedPt->X, seedPt->Y, -1, fpRect);

        VectRestore();
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
