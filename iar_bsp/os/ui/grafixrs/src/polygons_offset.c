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
*  polygons_offset.c
*
* DESCRIPTION
*
*  Contains polygon support function, OffsetPoly.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  OffsetPoly
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
*    OffsetPoly
*
* DESCRIPTION
*
*    Function OffsetPoly offsets the specified polygon(s) a tDX, tDY distance
*    from their current location.
*
* INPUTS
*
*    INT32 POLYCNT     - Count.
*
*    polyHead *POLYHDR - Pointer to the polyHead structure.
*
*    point *POLYPTS    - Pointer to the points.
*
*    INT32 tDX         - X difference.
*
*    INT32 tDY         - Y difference.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID OffsetPoly(INT32 POLYCNT, polyHead *POLYHDR, point *POLYPTS, INT32 tDX, INT32 tDY)
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
            POLYPTS[i].X = POLYPTS[i].X + tDX;
            POLYPTS[i].Y = POLYPTS[i].Y + tDY;
        }
        
        POLYHDR = POLYHDR + polyHeadSize;
    }

    /* Return to user mode */
    NU_USER_MODE();

}
