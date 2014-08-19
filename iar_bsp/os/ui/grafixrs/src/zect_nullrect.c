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
*  zect_nullrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - NullRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  NullRect
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
*    NullRect
*
* DESCRIPTION
*
*    Function NullRect sets the rectangle pointed to by argR to have dimensions
*    of the center point of the rectangle, such that it contains no area.
*
* INPUTS
*
*    rect *argR - Pointer to the rectangle to NULL.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID NullRect(rect *argR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* compute average of Xs */
    argR->Xmin = (argR->Xmin + argR->Xmax) / 2;
    argR->Xmax = argR->Xmin;

    /* compute average of Ys */
    argR->Ymin = (argR->Ymin + argR->Ymax) / 2; 
    argR->Ymax = argR->Ymin;

    /* Return to user mode */
    NU_USER_MODE();
}
