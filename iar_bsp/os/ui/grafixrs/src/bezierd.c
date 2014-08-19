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
**************************************************************************

**************************************************************************
*
* FILE NAME                                                         
*
*  bezierd.c                                                    
*
* DESCRIPTION
*
*  All Bezier draw functions.The main API for BEZIER is included  
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Bezier_Draw           - API call
*  BEZD_BezierDepth         - support function for API
*  BEZD_BezierDepthRecurse  - support function for API
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  bezierd.h
*  gfxstack.h
*
***************************************************************************/
#include <stdlib.h>

#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/bezierd.h"
#include "ui/gfxstack.h"

/* pointer into bezPts array */
static point *ptrBEZPTS;       

/*****************************************************************************
* FUNCTION
*
*    RS_Bezier_Draw
*
* DESCRIPTION
*
*    Function is for drawing BEZIER's.  The API allows you to PAINT, FRAME, ERASE
*    INVERT, and FILL an BEZIER.
*
* INPUTS
*
*    ObjectAction action - This would be the action that would be performed
*                          on the object, this will be a list of actions in 
*                          an enumerated data type.  
*       ** Extra Function for Bezier is Poly.
*
*        EX: FRAME  = 0,
*            PAINT  = 1,
*            FILL   = 2,
*            ERASE  = 3,
*            INVERT = 4,  
*            POLY   = 5  (only used by BEZIER) 
*
*    point *bezier_points - Pointer to the points to use for the bezier manipulation.
*
*    INT32 point_count    - This is point count.
*
*    INT32 patt           - Fill pattern structure that contains 32 default values 
*                         So the value is 0 to 31. -1 if not used. This can be user Defined
*
*    Then based off of what is passed into the function you will then send it to the "action"
*    function that going to be used.
*    
*    NOTE: The graphics port must already be set up and it should be global.  
*
* OUTPUTS
*
*    STATUS is return either NU_SUCCESS for passing and ~NU_SUCCESS if failure. 
*
****************************************************************************/
STATUS RS_Bezier_Draw( ObjectAction action, point *bezier_points, INT32 point_count, INT32 patt)
{
    STATUS status = ~NU_SUCCESS;

    /* number of interpolation points */
    INT32   num_bezier_points;       

    /* pointer for interpolated point array */
    point *ptr_bezier_points;        

    /* work variables */
    INT32   i;              
    INT32   j;

    /* temporary bezier control */
    point temp_pointer[4];          
    INT32   bezier_depth;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();
    
    /* to remove paradigm warning */
    (VOID)status;

    /* is pnLevel visible? */
    if( theGrafPort.pnLevel < 0 )
    {
        /* Do nothing get out */
        status = NU_SUCCESS;
    }
    else
    {
        /* starting point */
        num_bezier_points = 1;   

        /* Compute number of points for bezier approximation */
        for( i = 0; i < (point_count - 1); i += 3)
        {
            /* Process one curve at a time */
            for( j = 0; j < 4; j++)
            { 
                /* copy and convert control points to global */
                if( globalLevel > 0 )
                {
                    U2GP( bezier_points[i + j].X, bezier_points[i + j].Y, &temp_pointer[j].X, &temp_pointer[j].Y, 1);
                }
                else
                {
                    temp_pointer[j]= bezier_points[i + j];
                }
            }

            bezier_depth = BEZD_BezierDepth((point *)&temp_pointer);
            num_bezier_points += (1 << bezier_depth);
        }

        /* Allocate memory to point approximation array (add space for one
           additional point in case we need to close) */
        if( action == FRAME )
        {
            status = GRAFIX_Allocation( &System_Memory, (VOID**) &ptr_bezier_points,
                                         ( (num_bezier_points + 1) << 4), 0 );
        }
        else
        {
            status = GRAFIX_Allocation( &System_Memory, (VOID **) &ptr_bezier_points,
                                         (num_bezier_points << 4), 0 );
        }

        /* Compute the bezier approximation and draw */
        if( status == NU_SUCCESS )
        {
            /* ptrBEZPTS is global used by BEZD_BezierDepthRecurse() below */
            /* pick up pointer to output */
            ptrBEZPTS = ptr_bezier_points; 

            /* Generate point list for bezier approximation. */
            for( i = 0; i < (point_count - 1); i += 3)
            {
                /* Process one curve at a time */
                for( j = 0; j < 4; j++)
                { 
                    /* copy and convert control points to global */
                    if( globalLevel > 0 )
                    {
                        U2GP(bezier_points[i + j].X, bezier_points[i + j].Y, &temp_pointer[j].X, &temp_pointer[j].Y, 1);
                    }
                    else
                    {
                        temp_pointer[j]= bezier_points[i + j];
                    }
                }

                /* store the starting point for the first curve */
                if( i == 0 )
                {
                    *ptrBEZPTS++ = temp_pointer[0];
                }

                /* Calculate the recursion depth (numPts = 2^bezier_depth) */
                bezier_depth = BEZD_BezierDepth( (point *)&temp_pointer );
                if( bezier_depth  == 0 )
                {
                    /* if bezier_depth is zero, this section is a straight line */
                    *ptr_bezier_points++ = temp_pointer[3];
                }
                else
                {
                    /* calculate the approximation points */
                    BEZD_BezierDepthRecurse( (INT32) (temp_pointer[0].X << 16), (INT32) (temp_pointer[0].Y << 16),
                              (INT32) (temp_pointer[1].X << 16), (INT32) (temp_pointer[1].Y << 16),
                              (INT32) (temp_pointer[2].X << 16), (INT32) (temp_pointer[2].Y << 16),
                              (INT32) (temp_pointer[3].X << 16), (INT32) (temp_pointer[3].Y << 16),
                                bezier_depth);
                }
            }

            if( action == FRAME )
            {
                /* Does the last point close to the first? */
                if( (ptr_bezier_points[0].X != ptr_bezier_points[num_bezier_points-1].X)
                    ||
                    (ptr_bezier_points[0].Y != ptr_bezier_points[num_bezier_points-1].Y))
                {
                    /* no, add final point to close to first */
                    ptr_bezier_points[num_bezier_points].X = ptr_bezier_points[0].X;
                    ptr_bezier_points[num_bezier_points].Y = ptr_bezier_points[0].Y;
                    num_bezier_points++;
                }
            }
            globalLevel--;

            switch(action)
            {
                case FRAME:
                case PAINT:
                case POLY:
                    break;
                
                case ERASE:
                    /* set up default blit record */ 
                    grafBlit.blitRop = zREPz;
                    grafBlit.blitPat = theGrafPort.bkPat;
                    break;

                case INVERT:
                    /* set up default blit record */
                    grafBlit.blitRop = zINVERTz;
                    break;

                case FILL:
                    /* set up default blit record */ 
                    grafBlit.blitRop = zREPz;
                    grafBlit.blitPat = (patt & 0x1f);
                    break;
				case TRANS:
					grafBlit.blitRop = xAVGx;
					break;

                default:
                    status = NU_INVALID_DRAW_OP;
                    break;
            }

            /* Draw the bezier curve(s) */
            if( status != NU_INVALID_DRAW_OP )
            {
                if( (action != FRAME) && (action != POLY) )
                {
                    FillPolygon(ptr_bezier_points, num_bezier_points, coordAbs, shapeComplex);
                }
                else
                {
                    RS_Line_Draw( (INT16) num_bezier_points, (point *)ptr_bezier_points);
                }

                if( (action == ERASE) || (action == FILL) )
                {   
                    /* restore blit record */
                    grafBlit.blitPat = theGrafPort.pnPat;
                }
                else if( (action == ERASE) || (action == FILL) || (action == INVERT) )
                {
                    grafBlit.blitRop = theGrafPort.pnMode;
                }

                status = NU_SUCCESS;
            }

            globalLevel++;

            /* Free the point array */
            GRAFIX_Deallocation(ptr_bezier_points);

        } /* if (status == NU_SUCCESS) */ 

    } 

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
    
    return(status);
}


/*****************************************************************************
* FUNCTION
*
*    BEZD_BezierDepth
*
* DESCRIPTION
*
*    Function that does the recursion "depth" for 
*    de'Casteljau approximation of a Bezier curve.
*    
*    The number of points generated for a given depth is:
*
*      numBezPts = 2^bezDepth (+ 1, for starting point)
*
* INPUTS
*
*    point *points_in_curve - points on the bezier curve
*
* OUTPUTS
*
*    Returns "depth" for de'Casteljau approximation of a Bezier curve.  
*    else  returns zero (0) if the control and handle points
*    are equal (the curve is a straight line). 
*
****************************************************************************/
INT32 BEZD_BezierDepth(point *points_in_curve)
{
    /* temporary control points */
    point   temp_c_1;         
    point   temp_c_2;
    point   temp_h_1;
    point   temp_h_2;

    /* max delta between ctrl1 & ctrl2 */
    INT32   max_delta_c1_c2;  

    /* max delta between ctrl1 & hand1 */
    INT32   max_delta_c1_h1;  

    /* max delta between ctrl2 & hand2 */
    INT32   max_delta_c2_h2;  

    /* max delta of deltas */
    INT32   max_delta;   

    /* depth to return */
    INT32   depth;      

    /* get input data */
    temp_c_1 = *points_in_curve++;
    temp_h_1 = *points_in_curve++;
    temp_h_2 = *points_in_curve++;
    temp_c_2 = *points_in_curve;

    /* Compute deltas between control points and handles */
    max_delta_c1_c2 = __max( abs(temp_c_1.X - temp_c_2.X), abs(temp_c_1.Y - temp_c_2.Y));
    max_delta_c1_h1 = __max( abs(temp_c_1.X - temp_h_1.X), abs(temp_c_1.Y - temp_h_1.Y));
    max_delta_c2_h2 = __max( abs(temp_c_2.X - temp_h_2.X), abs(temp_c_2.Y - temp_h_2.Y)); 

    /* If the handles are equal (or close) to the control points,
       treat the bezier as a straight line. */
    if( (max_delta_c1_h1 < 4 ) && (max_delta_c2_h2 < 4 ) )
    {
        depth = 0;
    }
    else
    {
        /* determine the maximum delta */
        max_delta = __max( max_delta_c1_c2, max_delta_c1_h1);
        max_delta = __max( max_delta,  max_delta_c2_h2);

        if( max_delta < 8 )
        {
            /* 2 segments,  3 points */
            depth = 1; 
        }
        else if (max_delta < 16)
        {
            /*  4 segments,  5 points */
            depth = 2;  
        }
        else if (max_delta < 32)
        {
            /*  8 segments,  9 points */
            depth = 3;  
        }
        else if (max_delta < 128)
        {
            /* 16 segments, 17 points */
            depth = 4;  
        }
        else if (max_delta < 512)
        {
            /* 32 segments, 33 points */
            depth = 5;  
        }
        else
        {
            /* 64 segments, 65 points */
            depth = 6;  
        }
    }

    return(depth);
}

/*****************************************************************************
* FUNCTION
*
*    BEZD_BezierDepthRecurse
*
* DESCRIPTION
*
*    Calculates the de'Casteljau construction points by recursively 
*    subdividing the Bezier curve by its control points. 
*
*    NOTE: Calculates the de'Casteljau construction points so the Bezier
*          can be subdivided into 2 parts (left, then right) by recursive calls
*          to this routine. Recursion is broken off when depth, decremented once
*          for each recursion level, becomes 0.  This is the finest level of
*          subdivision; the right-most point on the small subdivided Bezier is
*          also a point on the original Bezier, so we load it into global array
*          BezPts[] (thru ptrBezPts which points into the array).
*
* INPUTS
*
*    INT32 point0_x - point0 x coordinate
*    INT32 point0_y - point0 y coordinate
*    INT32 point1_x - point1 x coordinate
*    INT32 point1_y - point1 y coordinate
*    INT32 point2_x - point2 x coordinate
*    INT32 point2_y - point2 y coordinate
*    INT32 point3_x - point3 x coordinate
*    INT32 point3_y - point3 y coordinate
*    INT32 depth - depth of the curve
*
* OUTPUTS
*
*    None
*
****************************************************************************/
VOID BEZD_BezierDepthRecurse(INT32 point0_x, INT32 point0_y, INT32 point1_x, INT32 point1_y, 
                             INT32 point2_x, INT32 point2_y, INT32 point3_x, INT32 point3_y, 
                             INT32 depth)
{
    INT32 midway_q0_x,midway_q0_y;
    INT32 midway_q1_x,midway_q1_y;
    INT32 midway_q2_x,midway_q2_y;
    INT32 midway_r0_x,midway_r0_y;
    INT32 midway_r1_x,midway_r1_y;
    INT32 midway_s0_x,midway_s0_y;

    /* depth == 0 means we are at the finest subdivision level: store the
       point into global array and return, breaking off recursion. */ 
    if( depth == 0 )
    {
        ptrBEZPTS->X = (point3_x >> 16);
        ptrBEZPTS->Y = (point3_y >> 16);

        ptrBEZPTS++;
    }
    else
    {
        /* Calculate de Casteljau construction points as averages of
           previous points (ie., midway points). */
        /* q's are midway between 4 incoming control and handle points. */
        midway_q0_x = (point0_x + point1_x) >> 1;
        midway_q0_y = (point0_y + point1_y) >> 1;
        midway_q1_x = (point1_x + point2_x) >> 1;
        midway_q1_y = (point1_y + point2_y) >> 1;
        midway_q2_x = (point2_x + point3_x) >> 1;
        midway_q2_y = (point2_y + point3_y) >> 1;

        /* r's are midway between 3 q's. */
        midway_r0_x = (midway_q0_x + midway_q1_x) >> 1;
        midway_r0_y = (midway_q0_y + midway_q1_y) >> 1;
        midway_r1_x = (midway_q1_x + midway_q2_x) >> 1;
        midway_r1_y = (midway_q1_y + midway_q2_y) >> 1;

        /* s0 is midway between 2 r's and is in middle of original Bez. */
        midway_s0_x = (midway_r0_x + midway_r1_x) >> 1;
        midway_s0_y = (midway_r0_y + midway_r1_y) >> 1;

        /* Decrement depth; subdivide incoming Bez into 2 parts: left, then right. */
        BEZD_BezierDepthRecurse(point0_x, point0_y, midway_q0_x, midway_q0_y, midway_r0_x, midway_r0_y, midway_s0_x, midway_s0_y, --depth);
        BEZD_BezierDepthRecurse(midway_s0_x, midway_s0_y, midway_r1_x, midway_r1_y, midway_q2_x, midway_q2_y, point3_x, point3_y,   depth);
    }

}

/* Check for __max function */
#ifndef __max
INT32 __max(INT32 val1, INT32 val2)
{
    if (val1 > val2) return val1;
    return val2;
}
#endif


