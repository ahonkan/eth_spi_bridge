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
*  rects.c                                                      
*
* DESCRIPTION
*
*  Contains the API rectangle support functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  EqRectHeight
*  EqRectWidth
*  RectHeight
*  RectWidth
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  edges.h
*  rects.h
*  globalrsv.h
*
***************************************************************************/

#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/edges.h"
#include "ui/rects.h"
#include "ui/globalrsv.h"

/***************************************************************************
* FUNCTION
*
*   EqRectHeight
*
* DESCRIPTION
*
*    Function EqRectHeight sees if the height of two rects are equal.
*
* INPUTS
*
*    rect *r  - Pointer to a rectangle.
*    rect *r1 - Pointer to the other rectangle.
*
* OUTPUTS
*
*    INT32 - Returns 1 if height are the same.
*
***************************************************************************/
INT32 EqRectHeight(rect *r, rect *r1)
{
    INT32       ret_val = 0;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    if( (r->Ymax - r->Ymin) == (r1->Ymax - r1->Ymin) )
    {
        ret_val = 1; 
    }

    /* Return to user mode */
    NU_USER_MODE();

    return ret_val;
}

/***************************************************************************
* FUNCTION
*
*   EqRectWidth
*
* DESCRIPTION
*
*    Function EqRectWidth sees if the width of two rects are equal.
*
* INPUTS
*
*    rect *r  - Pointer to a rectangle.
*    rect *r1 - Pointer to the other rectangle.
*
* OUTPUTS
*
*    INT32 - Returns 1 if width are the same.
*
***************************************************************************/
INT32 EqRectWidth(rect *r, rect *r1)
{
    INT32       ret_val = 0;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    

    if( (r->Xmax - r->Xmin) == (r1->Xmax - r1->Xmin) )
    {
        ret_val = 1; 
    }

    /* Return to user mode */
    NU_USER_MODE();

    return ret_val;
}

/***************************************************************************
* FUNCTION
*
*   RectHeight
*
* DESCRIPTION
*
*    Function RectHeight returns the height of a rectangle.
*
* INPUTS
*
*    rect *r     - Pointer to the rectangle.
*
* OUTPUTS
*
*    INT32 - Returns the height of a rectangle.
*
***************************************************************************/
INT32 RectHeight(rect *r)
{
    INT32 height;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Get the height of the rectangle */
    height = r->Ymax - r->Ymin;

    /* Return to user mode */
    NU_USER_MODE();

    return height;
}

/***************************************************************************
* FUNCTION
*
*   RectWidth
*
* DESCRIPTION
*
*    Function RectWidth returns the width of a rectangle.
*
* INPUTS
*
*    rect *r     - Pointer to the rectangle.
*
* OUTPUTS
*
*    INT32 - Returns the width of a rectangle.
*
***************************************************************************/
INT32 RectWidth(rect *r)
{
    INT32 width;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Get the width of the rectangle */
    width = r->Xmax - r->Xmin; 

    /* Return to user mode */
    NU_USER_MODE();

    return width;
}
