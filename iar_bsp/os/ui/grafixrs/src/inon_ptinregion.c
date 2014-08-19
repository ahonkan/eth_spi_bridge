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
*  inon_ptinregion.c                                                       
*
* DESCRIPTION
*
*  Contains functions that check whether a point or rectangle is in the region.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtInRegion
*  RectInRegion
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
*    PtInRegion
*
* DESCRIPTION
*
*    Function PtInRegion checks to see if a rectangle of size pxwid by pxht
*    centered about point pt.
*        False = does not intersect region
*        True  = intersects region
*
* INPUTS
*
*    point *pt   - Pointer to the point to check.
*
*    region *rgn - Pointer to the region.
*
*    INT32 pxwid - Width about point.
*
*    INT32 pxht  - Height about the point.
*
* OUTPUTS
*
*    INT32       - Returns TRUE if the point is in the region.
*                - Returns FALSE if the point is not in the region.
*
***************************************************************************/
INT32 PtInRegion( point *pt, region *rgn, INT32 pxwid, INT32 pxht)
{
    rect temRect;
    INT16 value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set up test rectangle. In order to match the pen, we
    follow the same rectangle fill rules. This means that an 
    even sized pen will have the "extra" size at the right
    and maximum sides */
    if( pxwid < 1 )
    {
        pxwid = 1;
    }
    if( pxht < 1 )
    {
        pxht = 1;
    }

    temRect.Xmin = pt->X - (pxwid >> 1);
    temRect.Ymin = pt->Y - (pxht >> 1);
    temRect.Xmax = temRect.Xmin + pxwid;
    temRect.Ymax = temRect.Ymin + pxht;

    value = RectInRegion(&temRect, rgn);

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    RectInRegion
*
* DESCRIPTION
*
*    Function RectInRegion checks to see if a rectangle;
*       False = does not intersect region
*       True  = intersects region
*
* INPUTS
*
*    point *pt   - Pointer to the point to check.
*
*    region *rgn - Pointer to the region.
*
*    INT32 pxwid - Width about point.
*
*    INT32 pxht  - Height about the point.
*
* OUTPUTS
*
*    INT32       - Returns TRUE if the point does intersect the region.
*                - Returns FALSE if the point does not intersect the region.
*
***************************************************************************/
INT32 RectInRegion( rect *r , region *rgn)
{
    INT32    value = 0;

    rect temRect;
    INT16 numRects;
    rect *rgnRectPtr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {
        U2GR(*r, &temRect,0);
    }
    else
    {
        temRect = *r;
    }

    /* Global value and do trivial check on region bound box */
    temRect.Xmax--;
    temRect.Ymax--;

    if (!((temRect.Xmax <  rgn->rgnRect.Xmin)
       || (temRect.Xmin >= rgn->rgnRect.Xmax)
       || (temRect.Ymax <  rgn->rgnRect.Ymin)
       || (temRect.Ymin >= rgn->rgnRect.Ymax)))
    {
        temRect.Xmax++;
        temRect.Ymax++;

        /* Check each rect in region  */
        numRects = (INT16) (rgn->rgnListEnd - rgn->rgnList);
        rgnRectPtr = rgn->rgnList;

        while( numRects-- >= 0 )
        {   
            if(   (temRect.Ymax >= rgnRectPtr->Ymin)
                && (temRect.Ymin <= rgnRectPtr->Ymax)
                && (temRect.Xmax >= rgnRectPtr->Xmin)
                && (temRect.Xmin <= rgnRectPtr->Xmax) )
            {
                value = 1;
                break; 
            }
            rgnRectPtr++;
        }

    } /* else */

    /* Return to user mode */
    NU_USER_MODE();

    return (value);
}
