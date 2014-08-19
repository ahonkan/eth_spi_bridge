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
*  line.c                                                       
*
* DESCRIPTION
*
*  This file contains API - level line drawing functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  LineTo
*  LINE_rsOvalPolyLines
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  line.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/line.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    LineTo
*
* DESCRIPTION
*
*    Function LineTo draws a "LineStyle" line from the current "pen" position
*    to the specified (X,Y) screen coordinate using the predefined "RasterOp"
*    write operation.  Line specifications which extend beyond the current
*    viewport limits are automatically clipped appropriately.  The current "pen"
*    position is updated to the specified (X,Y) screen coordinates.
*
* INPUTS
*
*    INT32 argX - End X coordinate of line to draw.
*
*    INT32 argY - End Y coordinate of line to draw.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID LineTo( INT32 valX, INT32 valY)
{
    INT32 previousX;
    INT32 previousY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    lineExecPntr = (VOID (*)()) lineExecIDV;

    /* update shadow port */
    theGrafPort.pnLoc.X = valX;
    theGrafPort.pnLoc.Y = valY;

    /* update user port */
    thePort->pnLoc.X = valX;
    thePort->pnLoc.Y = valY;

    /* save previous point for this line */
    previousX = LocX;
    previousY = LocY;

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GP(valX, valY, &LocX, &LocY, 1);
    }
    else
    {
        /* update global pen location */
        LocX = valX;
        LocY = valY;
    }

    /* if pen hidden, we are done */
    if( theGrafPort.pnLevel >= 0 )
    {
        /* set up default blitRcd for this line */
        grafBlist.Xmin     = previousX;
        grafBlist.Ymin     = previousY;
        grafBlist.Xmax     = LocX;
        grafBlist.Ymax     = LocY;

        /* draw with current end cap style */
        grafBlist.skipStat = theGrafPort.pnCap; 

        /* call the current line routine with the default blitRcd */
        lineExecPntr(&grafBlit);
    }

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    LINE_rsOvalPolyLines
*
* DESCRIPTION
*
*    Oval poly lines are constructed via regions. Regions defining the line, end
*    caps, and joins are captured then painted via PaintRegion(). The line portion
*    is constructed by a polygon whose vertices are computed by finding the points
*    on a circle defining the round pen at angles perpendicular to the line
*    eliminating from the center of the circle.
*
*    Current Restrictions:
*     1) lines are forced to round, using the larger of the two pen dimensions
*     2) Dashes not supported
*     3) capSquare not supported - does capFlat instead  
*     4) joinMiter not supported - does joinBevel no matter what miterlimit  
*     5) doesn't xor correctly, use region capture to do this.
*
* INPUTS
*
*    INT16 numpoints - The number of points to draw.
*
*    point *points   - Pointer to the points to draw.
* 
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID LINE_rsOvalPolyLines(INT16 numpoints, point *points)
{
    INT16 Done = NU_FALSE;
    INT32 halfmin;
    INT32 halfmax;
    INT32 ptcount;
    INT32 ptcount1;
    rect  bb1;
    rect  bb2;
    INT32 capstyle;
    INT32 joinstyle;
    INT32 lineangle;
    INT32 angleP90;
    INT32 angleM90;
    point lpoly[5];
    point lastpoly[5];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* must have at least two points */
    if( numpoints < 2 )
    {
        Done = NU_TRUE;
    }
    
    /* if (grafPort.pnLevel < 0), exit */
    if( !Done && theGrafPort.pnLevel >= 0 )
    {
        /* pickup cap and join style */
        capstyle = theGrafPort.pnCap;
        joinstyle = theGrafPort.pnJoin;

        /* compute pen dimensions, force to round pen */
        if( theGrafPort.pnSize.Y > theGrafPort.pnSize.X )
        {
            halfmin = theGrafPort.pnSize.Y >> 1;
            halfmax = (theGrafPort.pnSize.Y + 1) >> 1;
        }
        else
        {
            halfmin = theGrafPort.pnSize.X >> 1;
            halfmax = (theGrafPort.pnSize.X + 1) >> 1;
        }

        /* decrement the index value for later */
        numpoints--;    
        for( ptcount=0; ptcount<numpoints; ptcount++ )
        {
            /* compute bounding box for this point */
            bb1.Xmin = points[ptcount].X - halfmin;
            bb1.Xmax = points[ptcount].X + halfmax;
            bb1.Ymin = points[ptcount].Y - halfmin;
            bb1.Ymax = points[ptcount].Y + halfmax;

            /* compute bounding box for next point */
            ptcount1 = ptcount + 1;
            bb2.Xmin = points[ptcount1].X - halfmin;
            bb2.Xmax = points[ptcount1].X + halfmax;
            bb2.Ymin = points[ptcount1].Y - halfmin;
            bb2.Ymax = points[ptcount1].Y + halfmax;

            /* compute polygon describing line with flat caps */

            /* compute angle of line eliminating from this vertex */
            lineangle = (INT32) PtToAngle(&bb1, &points[ptcount1]);

            /* compute angles +- 90 from this angle */
            angleP90 = lineangle + 900;
            angleM90 = lineangle - 900;

            /* compute points on oval at those angles - 
               these should be the tangent points */
            OvalPt(&bb1, angleP90, &lpoly[0]);
            OvalPt(&bb1, angleM90, &lpoly[3]);

            /* compute angle of line eliminating from next vertex */
            lineangle = (INT32) PtToAngle(&bb2, &points[ptcount]);

            /* compute angles +- 90 from this angle */
            angleP90 = lineangle + 900;
            angleM90 = lineangle - 900;

            /* compute points on oval at those angles - 
               these should be the tangent points */
            OvalPt(&bb2, angleP90, &lpoly[2]);
            OvalPt(&bb2, angleM90, &lpoly[1]);

            /* draw the polygon */
            FillPolygon(&lpoly[0], 4, coordModeOrigin, convex);

            /* if this is the first point, cap it, otherwise join it */
            if( ptcount == 0 )
            {
                if( capstyle == capRound )
                {
                    RS_Oval_Draw( PAINT, &bb1, -1);
                }
            }
            else
            {
                switch (joinstyle)
                {
                case joinRound:
                    RS_Oval_Draw( PAINT, &bb1, -1);
                    break;
                case joinMiter:
                case joinBevel:
                    /* fill a polygon connecting the last flat cap to the new */
                    lastpoly[1].X = lpoly[0].X;
                    lastpoly[1].Y = lpoly[0].Y;
                    lastpoly[3].X = lpoly[3].X;
                    lastpoly[3].Y = lpoly[3].Y;
                    FillPolygon(&lastpoly[0], 4, coordModeOrigin, convex);
                    break;
                }
            }

            /* remember the end of this segment as "last" one */
            lastpoly[0].X = lpoly[1].X;
            lastpoly[0].Y = lpoly[1].Y;
            lastpoly[2].X = lpoly[2].X;
            lastpoly[2].Y = lpoly[2].Y;
        }

        /* cap the last point */
        if( capstyle == capRound )
        {
            RS_Oval_Draw( PAINT, &bb2, -1);
        }
    }

    if( !Done )
    {
        /* update user pen location */
        theGrafPort.pnLoc.X = points[numpoints].X;
        theGrafPort.pnLoc.Y = points[numpoints].Y;
        thePort->pnLoc.X = points[numpoints].X;
        thePort->pnLoc.Y = points[numpoints].Y;

        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GP(points[numpoints].X, points[numpoints].Y, &LocX, &LocY, 1);
        }
        else
        {
            /* update global pen location */
            LocX = points[numpoints].X;
            LocY = points[numpoints].Y;
        }
    } /* if( !Done ) */

    /* Return to user mode */
    NU_USER_MODE();
}
