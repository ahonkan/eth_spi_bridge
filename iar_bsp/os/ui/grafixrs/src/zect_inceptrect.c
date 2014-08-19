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
*  zect_inceptrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation functions InceptRect and its supporting functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  INCEPT
*  InceptRect
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
*    INCEPT
*
* DESCRIPTION
*
*    Local function INCEPT returns TRUE value if there is an intersection 
*    between two rectangular otherwise it returns FALSE.
*
* INPUTS
*
*    INT32 Min1  - Value to compare with Min2. Should be less than Min2.
*                  
*
*    INT32 Max1  - For test = 0: Xmax if Max2 > Max1
*                  For test = 1: Ymax if Max2 < Max1
*
*    INT32 Min2  - For test = 0: Xmin
*                  For test = 1: Ymin
*
*    INT32 Max2  - For test = 0: Xmax if Max2 < Max1
*                  For test = 1: Ymax if Max2 > Max1
*
*    rect *dstR  - Pointer to the resulting destination rectangle - Xmin,Ymin,Xmax,Ymax.
*
*    INT32 test  - Set to 1 to compare y coordinates.
*                  Set to 0 to compare x coordinates.
*
* OUTPUTS
*
*    INT32      - Returns TRUE if the rectangles intersect.
*                 Returns FALSE if not.
*
***************************************************************************/
INT32 INCEPT( INT32 Min1 , INT32 Max1, INT32 Min2, INT32 Max2 , rect *dstR , INT32 test)
{
    INT32 value = 1;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (Min1 > Min2) || (Min2 > Max1) )
    {
        /* There is no interception */
        value = 0; 
    }
    else
    {
        if( test == 0 )
        {
            /* X coordination points */
            dstR->Xmin = Min2;
            dstR->Xmax = Max2;
            if( Max2 > Max1)
            {
                dstR->Xmax = Max1;
            }
        }
        else
        {
            /* Y coordination points */
            dstR->Ymin = Min2;
            dstR->Ymax = Max2;
            if( Max2 > Max1 )
            {
                dstR->Ymax = Max1;
            }
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    InceptRect
*
* DESCRIPTION
*
*    Function InceptRect calculates the rectangle DstR that is the intersection
*    of two input source rectangles (Rect1 & Rect2), and RETURNs TRUE if they intersect
*    or FALSE if they do not.  Rectangles that "touch" on the same line or point
*    are considered as interesting.
*   
*    If the two source rectangles do not intersect, the destination is left
*    unchanged.
*
* INPUTS
*
*    rect *Rect1 - Pointer to the first rectangle.
*
*    rect *Rect2 - Pointer to the second rectangle.
*
*    rect *dstR  - Pointer to the destination rectangle.
*
* OUTPUTS
*
*    INT32      - Returns TRUE if the rectangle intersect.
*                 Returns FALSE if not.
*
***************************************************************************/
INT32 InceptRect( rect *Rect1, rect *Rect2, rect *dstR)
{
    INT32 value;
    rect tmpR;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check for X intersection and if FALSE switch value */
    value = INCEPT(Rect1->Xmin,Rect1->Xmax,Rect2->Xmin,Rect2->Xmax, &tmpR ,0 );
    
    /* Exchange position between two rectangles */
    if( value == 0 )
    {
        value = INCEPT(Rect2->Xmin,Rect2->Xmax,Rect1->Xmin,Rect1->Xmax, &tmpR ,0 );
    }

    if( value != 0 )
    {
        /* Check for Y intersection and if FALSE switch value */
        value = INCEPT(Rect1->Ymin,Rect1->Ymax,Rect2->Ymin,Rect2->Ymax, &tmpR ,1);

        /* Exchange position between two rectangles */
        if( value == 0 )
        {
            value = INCEPT(Rect2->Ymin,Rect2->Ymax,Rect1->Ymin,Rect1->Ymax, &tmpR ,1);
        }

        /* Return FALSE value if still Error existing, there is no intersection */
        /* Also two rectangles touch both X and Y coordination is FALSE       */
        if( value == 1 )
        {
            /* set intersection rectangle */
            *dstR = tmpR; 
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}
