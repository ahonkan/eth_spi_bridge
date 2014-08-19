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
*  zect_duprect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - DupRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  DupRect
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
*    DupRect
*
* DESCRIPTION
*
*    Function DupRect copies the contents of the source rectangle, srcR, to the
*    destination rectangle, dstR.
*
* INPUTS
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
VOID DupRect( rect *srcR , rect *dstR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Duplicate all value by restore them to Destination structure */
    dstR->Xmin = srcR->Xmin;
    dstR->Xmax = srcR->Xmax;
    dstR->Ymin = srcR->Ymin;
    dstR->Ymax = srcR->Ymax;

    /* Return to user mode */
    NU_USER_MODE();
}
