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
*  zect_initrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - InitRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  InitRect
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
*   InitRect
*
* DESCRIPTION
*
*   Function InitRect calls SetRect to create a rectangle using x and y 
*   coordinates and width and height, instead of 2 sets of x/y coordinates.
*
* INPUTS
*
*   rect *theRect - Pointer to rectangle.
*   INT32 x - Left horizontal coordinate.
*   INT32 y - Top vertical coordinate.
*   INT32 w - Width of rectangle.
*   INT32 h - Height of rectangle.
*
* OUTPUTS
*
*   None.
*
***************************************************************************/
VOID InitRect(rect *theRect, INT32 xStart, INT32 yStart, INT32 width, INT32 height)
{
    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Call SetRect */
    SetRect(theRect, xStart, yStart, xStart + width, yStart + height);

    /* Return to user mode */
    NU_USER_MODE();
}
