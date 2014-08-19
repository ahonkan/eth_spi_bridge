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
*  lined.c                                                      
*
* DESCRIPTION
*
*  API line draw function: RS_Line_Draw
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Line_Draw
*
* DEPENDENCIES
*
*  rs_base.h
*  rectd.h
*  lined.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rectd.h"
#include "ui/lined.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    RS_Line_Draw
*
* DESCRIPTION
*
*    The RS API Function RS_Line_Draw is used to draw lines.
*    NOTE: The graphics port must already be set up and it should be global.  
*
* INPUTS
*
*    INT16 pointCount - point count.
*
*    point *linePts   - Pointer to the points to use for the line manipulation.
*
* OUTPUTS
*
*    STATUS - Returns NU_SUCCESS if successful.
*           - Returns is non-zero if not successful.
*
***************************************************************************/
STATUS RS_Line_Draw( INT16 pointCount, point *linePts)
{
    INT16 Done               = NU_FALSE;
    INT16 JumpDoLastSegment  = NU_FALSE;
    INT16 JumpThinDoneNoSkip = NU_FALSE;
    INT16 JumpDrawLastSeg    = NU_FALSE;
    INT16 JumpThinDone       = NU_FALSE;
    INT16 JumpDrawThin       = NU_FALSE;
    INT16 JumpDoSquare       = NU_FALSE;
    STATUS status            = ~NU_SUCCESS;
    INT32 absCNT;        
    INT32 i                  = 0;
    INT32 TempX;
    INT32 TempY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    lineExecPntr = (VOID (*)()) lineExecIDV;

    while( !Done )
    {
        /* What kind of line are we handling? */
        if( JumpDrawThin == NU_TRUE || !(theGrafPort.pnFlags & pnSizeFlg) )
        {
            /* Thin line does not care about re-drawing of points that cross lines,
               but it does care that vertices only get drawn for one of the two entering
               lines (not twice, that is), so we'll have to go through the default blitRcd
               and use skipLast.  Note that this is NOT the same as calling LineTo with
               the end point backed off, as THAT would draw a different line. */
            JumpDrawThin = NU_FALSE;  

            /* don't draw the last point */
            grafBlist.skipStat = SkipLast;  

            /*we'll draw just one segment at a time */
            grafBlit.blitCnt = 1;   

            /* get the start coordinates of the */
            TempX = linePts[0].X;

            /* first line segment */
            TempY = linePts[0].Y;

            if( globalLevel > 0 )
            {
                /* convert from user to blit rcd */
                U2GP(TempX, TempY, &grafBlist.Xmin, &grafBlist.Ymin, 1);
            }
            else
            {
                /* set them into blit rcd */
                grafBlist.Xmin = TempX;
                grafBlist.Ymin = TempY;
            }

            /* Calculate absolute value of the count of vertices in polyline */
            /* nothing to do */
            if( pointCount == 0 )
            {
                status = NU_SUCCESS;
                Done   = NU_TRUE;
            }

            if( !Done )
            {
                /* need to close polygon (connect last point to first point) */
                if (pointCount < 0 )
                {
                    /* negate and add one extra close point */
                    absCNT = 1 - pointCount;
                }
                else
                {
                    absCNT = pointCount;
                }

                /* calculate number of vectors = point count - 1;
                   subtract one more since the last vector is handled specially */
                absCNT = absCNT - 2;
                if( absCNT == 0 )
                {
                    /* only one vector to draw */
                    JumpDoLastSegment = NU_TRUE; 
                }

                if( !JumpDoLastSegment )
                {
                    if( absCNT < 0 )
                    {
                        /* only one point to draw */
                        JumpThinDoneNoSkip = NU_TRUE; 
                    }

                    if( !JumpDoLastSegment && !JumpThinDoneNoSkip )
                    {
                        /* advance to the next polyline vertex */
                        for( i = 1; i <= absCNT; i++)
                        {
                            /* set the end coordinates of this line */
                            TempX = linePts[i].X;
                            TempY = linePts[i].Y;

                            if( globalLevel > 0 )
                            {
                                /* convert from user to blit rcd */
                                U2GP(TempX, TempY, &grafBlist.Xmax, &grafBlist.Ymax, 1);
                            }
                            else
                            {
                                /* set them into blit rcd */
                                grafBlist.Xmax = TempX;
                                grafBlist.Ymax = TempY;
                            }

                            /* call the current line routine with the default blitRcd */
                            lineExecPntr(&grafBlit);

                            /* set the start coordinates of next */
                            grafBlist.Xmin = grafBlist.Xmax;
                            grafBlist.Ymin = grafBlist.Ymax;
                        }

                    } /* if( !JumpDoLastSegment && !JumpThinDoneNoSkip ) */

                } /* if( !JumpDoLastSegment ) */

                JumpDoLastSegment = NU_FALSE;

                if( !JumpThinDoneNoSkip )
                {
                    /* If the original count was < 0, we need to close the polygon, otherwise we need to
                       draw the last segment without skipping the last point. */
                    if( pointCount >= 0 )
                    {
                        /* close polygon? */
                        JumpDrawLastSeg = NU_TRUE; 
                    }

                    if( !JumpDrawLastSeg )
                    {
                        /* yes, close the polygon */
                        i = 0;          

                        /* draw the segment with SkipLast */
                        JumpThinDone = NU_TRUE; 
                    }
                } /* if( !JumpThinDoneNoSkip ) */

                JumpDrawLastSeg = NU_FALSE;

                if( !JumpThinDoneNoSkip && !JumpThinDone )
                {
                    /* draw the last segment of the polyline, including the last point */
                    /* advance to the final vertex in the polyline */
                    i = absCNT + 1;

                    JumpThinDoneNoSkip = NU_FALSE;
                }

                if( !JumpThinDone )
                {
                    /* draw the last point, this once */
                    grafBlist.skipStat = NoSkip; 
                }

                JumpThinDone = NU_FALSE;

                /* set the end coordinates of the last */
                TempX = linePts[i].X;
                TempY = linePts[i].Y;

                if( globalLevel > 0 )
                {
                    /* convert from user to blit rcd */
                    U2GP(TempX, TempY, &grafBlist.Xmax, &grafBlist.Ymax, 1);
                }
                else
                {
                    /* set them into blit rcd */
                    grafBlist.Xmax = TempX;
                    grafBlist.Ymax = TempY;
                }

                /* call the current line routine with the default blitRcd */
                lineExecPntr(&grafBlit);
                status = NU_SUCCESS;
                Done   = NU_TRUE;
            }
        } /* if (!(grafPort.pnFlags & pnSizeFlg)) */


        /* Wide line may be either square or oval pen; figure out which. */
        if( !Done )
        {
            /* square or oval pen? */
            if( theGrafPort.pnFlags & pnShapeFlg )
            {
                while( !JumpDoSquare && status != NU_SUCCESS )
                {
                    JumpDoSquare = NU_FALSE;

                    /* Square pen, supporting no cap styles and no join styles. Dashing is
                       supported, but does not draw each pixel once and only once, so vertices and
                       self-intersection aren't handled properly for dashed XOR and the like. */
                       
#ifdef  DASHED_LINE_SUPPORT
                       
                    if( theGrafPort.pnFlags & pnDashFlg )
                    {
                        /* is the line dashed? */
                        JumpDrawThin = NU_TRUE; 
                        status = NU_SUCCESS;
                    }
                    else
                        
#endif  /* DASHED_LINE_SUPPORT */
                        
                    {
                        /* Note that skipLast is ignored by dashing square pen wide lines */

                        /* Special square pen solid or patterned poly line
                           - dashes ignored at this point (should never reach this point with dashing selected). */
                        lineExecPntr = (VOID (*)()) lineSqPolyIDV;

                        /* call the square pen polyline */
                        lineExecPntr( linePts, pointCount, 0);
                        status = NU_SUCCESS;
                        Done   = NU_TRUE;
                        JumpDoSquare = NU_TRUE; 
                    }

                    if( !Done )
                    {
                        /* oval pen, supporting all cap styles, joins, and patterns */
                        if( pointCount < 0 )
                        {
                            /* if this a framepoly do it with square pen */
                            JumpDoSquare = NU_TRUE; 
                            break; 
                        }

                    } /* if( !Done ) */

                } /* while( !DoSquare ) */

            } /* if( grafPort.pnFlags & pnShapeFlg ) */

            if( !Done && !JumpDrawThin )
            {
                lineExecPntr = (VOID (*)()) lineOvPolyIDV;

                /* call the oval pen polyline */
                lineExecPntr(pointCount, linePts);
                status = NU_SUCCESS;
                Done   = NU_TRUE;

            } /* if( !Done && !JumpDrawThin ) */

        } /* if( !Done ) */

    } /* while( !Done ) */

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    /* Done */
    return(status);
}
