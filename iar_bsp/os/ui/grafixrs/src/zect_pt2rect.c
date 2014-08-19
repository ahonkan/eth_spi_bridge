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
*  zect_pt2rect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - Pt2Rect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  Pt2Rect
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
*    Pt2Rect
*
* DESCRIPTION
*
*    Function Pt2Rect creates a rectangle R with the smallest rectangle which 
*    contains the two points PT1 & PT2.
*
* INPUTS
*
*    point *PT1 - Pointer to the first point.
*
*    point *PT2 - Pointer to the second point.
*
*    rect *R    - Pointer to the resulting rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Pt2Rect( point *PT1, point *PT2, rect *R)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( PT2->X < PT1->X )
    {
        R->Xmin = PT2->X;
        R->Xmax = PT1->X;
    }
    else
    {
        R->Xmin = PT1->X;
        R->Xmax = PT2->X;
    }


    if( PT2->Y < PT1->Y )
    {
        R->Ymin = PT2->Y;
        R->Ymax = PT1->Y;
    }
    else
    {
        R->Ymin = PT1->Y;
        R->Ymax = PT2->Y;
    }

    /* Return to user mode */
    NU_USER_MODE();
}
