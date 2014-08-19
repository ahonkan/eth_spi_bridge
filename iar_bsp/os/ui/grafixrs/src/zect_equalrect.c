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
*  zect_equalrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation functions - EqualRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  EqualRect
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
*    EqualRect
*
* DESCRIPTION
*
*    Function EqualRect determines whether rectangle Rect1 is exactly the same
*    as rectangle Rect2 and RETURNs a TRUE if it is or FALSE if is not.
*
* INPUTS
*
*    rect *Rect1 - Pointer to the first rectangle.
*
*    rect *Rect2 - Pointer to the second rectangle.
*
* OUTPUTS
*
*    INT32       - Returns TRUE if same.
*                - Returns FALSE if not the same.
*
***************************************************************************/
INT32 EqualRect( rect *Rect1 , rect *Rect2)
{
    INT32 value = 0;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if(   (Rect1->Xmin == Rect2->Xmin) && (Rect1->Xmax == Rect2->Xmax)
        &&(Rect1->Ymin == Rect2->Ymin) && (Rect1->Ymax == Rect2->Ymax) )
    {
        value = 1;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (value);
}
