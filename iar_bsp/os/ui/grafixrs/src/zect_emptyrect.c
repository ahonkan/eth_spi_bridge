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
*  zect_emptyrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - EmptyRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  EmptyRect
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
*    EmptyRect
*
* DESCRIPTION
*
*    Function EmptyRect tests the rectangle pointed to by argR to see if it
*    contains any area (Xmax > Xmin and Ymax > Ymin).
*
* INPUTS
*
*    rect *argR - Pointer to the rectangle to empty.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
INT32 EmptyRect(rect *argR)
{
    /* if rectangle is not empty, return FALSE */
    INT32 value = 0; 

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (argR->Xmax <= argR->Xmin) || (argR->Ymax <= argR->Ymin) )
    {
        /* rectangle is empty, return TRUE */
        value = 1; 
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);     
}
