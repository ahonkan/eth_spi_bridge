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
*  zect_maprect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - MapRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  MapRect
*
* DEPENDENCIES
*
*  rs_base.h
*  zect.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/zect.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    MapRect
*
* DESCRIPTION
*
*    Function MapRect maps the specified rectangle, R, within the source 
*    rectangle (srcR) to a similarly located rectangle within the destination 
*    rectangle (dstR).
*
* INPUTS
*
*    rect *R    - Pointer to the specified rectangle.
*
*    rect *srcR - Pointer to the source rectangle.
*
*    rect *dstR - Pointer to the destination rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MapRect( rect *R , rect *srcR , rect *dstR)
{
    point Tem;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    Tem.X = R->Xmin;
    Tem.Y = R->Ymin;

    MapPt(&Tem, srcR, dstR);

    Tem.X = R->Xmax;
    Tem.Y = R->Ymax;

    MapPt(&Tem, srcR, dstR);

    /* Return to user mode */
    NU_USER_MODE();
}
