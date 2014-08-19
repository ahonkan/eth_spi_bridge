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
*  polygons_mappoly.c
*
* DESCRIPTION
*
*  Contains polygon support function, MapPoly.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  MapPoly
*
* DEPENDENCIES
*
*  rs_base.h
*  globalrsv.h
*  polygons.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/globalrsv.h"
#include "ui/polygons.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    MapPoly
*
* DESCRIPTION
*
*    Function MapPoly maps the coordinate points from the source rectangle, srcR,
*    to the destination rectangle, dstR.
*
* INPUTS
*
*    INT32 POLYCNT     - Poly count.
*
*    polyHead *POLYHDR - Pointer to the polyHead structure.
*
*    point *POLYPTS    - Pointer to the points.
*
*    rect *srcR        - Pointer to the source rectangle.
*
*    rect *dstR)       - Pointer to the destination rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MapPoly(INT32 POLYCNT, polyHead *POLYHDR, point *POLYPTS, rect *srcR, rect *dstR)
{
    /* size of polygon header */
    INT32 polyHeadSize;   
    INT32 i;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    polyHeadSize = sizeof(polyHead);
    for( ; POLYCNT > 0; POLYCNT--)
    {
        for( i = POLYHDR->polyBgn; i <= (INT32) POLYHDR->polyEnd; i++)
        {
            MapPt(&POLYPTS[i], srcR, dstR);
        }
        
        POLYHDR = POLYHDR + polyHeadSize;
    }

    /* Return to user mode */
    NU_USER_MODE();

}

