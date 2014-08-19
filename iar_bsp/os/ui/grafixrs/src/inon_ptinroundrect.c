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
*  inon_ptinroundrect.c                                                       
*
* DESCRIPTION
*
*  Contains function that checks whether a point is in the given round 
*  rectangle or not.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtInRoundRect
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
*    PtInRoundRect
*
* DESCRIPTION
*
*    Function PtInRoundRect simply replaces the filler with INON_rsRectInList, which
*    detects whether there is a match or not. This allows perfect coincidence with
*    the drawing routine, because the drawer routine is actually used.
*
* INPUTS
*
*    point *fpTESTPT - Pointer to the point.
*
*    rect *argRect   - Pointer to the rectangle. 
*
*    rect *fpRect    - Pointer to the fpRect.
*
*    INT32 diaX      - Rounded rectangle X diameter. 
*
*    INT32 diaY      - Rounded rectangle Y diameter.
*
*    INT32 sizX      - Added X size to rect.
*
*    INT32 sizY      - Added Y size to rect.
*
* OUTPUTS
*
*    INT32           - Returns TRUE if the point is in the rectangle.
*                    - Returns FALSE if the point is not in the rectangle.
*
***************************************************************************/
INT32 PtInRoundRect(point *fpTESTPT, rect *fpRect, INT32 diaX, INT32 diaY, INT32 sizX, INT32 sizY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PtRslt = PtInRect(fpTESTPT, fpRect, sizX, sizY);
    if( PtRslt )
    {
        VectSetup();
        RS_Rectangle_Draw( FILL, fpRect, 1, diaX, diaY);
        VectRestore();
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
