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
*  zect_centerrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation functions - CenterRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  CenterRect
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
*    CenterRect
*
* DESCRIPTION
*
*    Function CenterRect returns rectangle R such that it area is centered about
*    point PT, and is WIDTHX wide by HEIGHTY high.
*
* INPUTS
*
*    point *PT     - Pointer to the center point.
*
*    INT32 widthX  - Width.
*
*    INT32 heightY - Height.
*
*    rect *dstR    - Pointer to the resulting rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID CenterRect( point *PT, INT32 widthX, INT32 heightY, rect *dstR)
{
    INT32 Value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( widthX < 0 )
    {
        Value = c_CenterRe + c_BadSize;
        nuGrafErr( (INT16)Value, __LINE__, __FILE__); 
        widthX = 0;
    }   
    if( heightY < 0 )
    {
        Value = c_CenterRe + c_BadSize;
        nuGrafErr( (INT16)Value, __LINE__, __FILE__); 
        heightY = 0;
    }

    Value = widthX >> 1;
    dstR->Xmin = PT->X - Value;
    dstR->Xmax = PT->X + Value;

    Value = heightY >> 1;

    dstR->Ymin = PT->Y - Value;
    dstR->Ymax = PT->Y + Value;

    /* Return to user mode */
    NU_USER_MODE();
}
