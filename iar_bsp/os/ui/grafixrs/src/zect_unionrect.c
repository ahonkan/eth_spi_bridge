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
*  zect_unionrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - UnionRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  UnionRect
*
* DEPENDENCIES
*
*  rs_base.h
*  zect.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/zect.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    UnionRect
*
* DESCRIPTION
*
*    Function UnionRect calculates and RETURNs the smallest rectangle "DstR"
*    which includes both input rectangles Rect1 and Rect2.
*
* INPUTS
*
*    rect *Rect1 - Pointer to the first rectangle.
*
*    rect *Rect2 - Pointer to the second rectangle.
*
*    rect *dstR  - Pointer to the resulting destination rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID UnionRect( rect *Rect1, rect *Rect2, rect *dstR)
{

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Assign the X coordinate values between two rectangles
       to the destination rectangle */
    if( Rect2->Xmin < Rect1->Xmin )
    {
        dstR->Xmin = Rect2->Xmin;
    }
    else 
    {
        dstR->Xmin = Rect1->Xmin;
    }

    if( Rect2->Xmax > Rect1->Xmax )
    {
        dstR->Xmax = Rect2->Xmax;
    }
    else
    {
        dstR->Xmax = Rect1->Xmax;
    }

    /* Assign the Y coordinate values between two rectangles
       to the destination rectangle */
    if( Rect2->Ymin < Rect1->Ymin )
    {
        dstR->Ymin = Rect2->Ymin;
    }
    else
    {
        dstR->Ymin = Rect1->Ymin;
    }

    if( Rect2->Ymax > Rect1->Ymax )
    {
        dstR->Ymax = Rect2->Ymax;
    }
    else
    {
        dstR->Ymax = Rect1->Ymax;
    }

    /* Return to user mode */
    NU_USER_MODE();

}
