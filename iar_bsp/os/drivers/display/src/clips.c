/***************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*  clips.c
*
* DESCRIPTION
*
*  This file contains the thin line rect Clipping functions.
*
* DATA STRUCTURES
*
*  Variables required are in globalv.h.
*
* FUNCTIONS
*
*  CLIP_ClipBottomOrRightTocYmaxOrcXmax
*  CLIP_ClipBottomOrLeftTocYmaxOrcXmin
*  CLIP_ClipTopTocYmin
*  CLIP_ClipTopOrLeftTocYminOrcXmin
*  CLIP_ClipTopOrRightTocYminOrcXmax
*  CLIP_ClipAndDrawEntry
*  CLIP_HandleFirstPointNull
*  CLIP_LineClipping
*  CLIP_Blit_Clip_Region
*  CLIP_Fill_Clip_Region
*  CLIP_Set_Up_Clip
*  CLIP_Check_YX_Band_Blit
*  CLIP_Check_YX_Band
*  CLIP_Line_Clip_Region
*  CLIP_Blit_Clip_Region_BT_RL
*  CLIP_Fill_Clip_Region_BT_RL
*  CLIP_Blit_Clip_Region_BT_LR
*  CLIP_Fill_Clip_Region_BT_LR
*  CLIP_Blit_Clip_Region_TB_RL
*  CLIP_Fill_Clip_Region_TB_RL
*
* DEPENDENCIES
*
*  nucleus.h
*  nu_kernel.h
*  nu_drivers.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"

#if         ((DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE))

extern  DISPLAY_DEVICE      SCREENI_Display_Device;

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) || (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef  THIN_LINE_OPTIMIZE

/****************************************************************************
  Line-Clipping Functions:
  Note that errTerm is -deltaMajor, the unadjusted error term for the first point,
  and that errTermAdjDown is 2*deltaMajor, the full adjustment when the minor
  axis moves. This is ideal for bitmapped drawing, where the error term adjust
  up can always be made before drawing each point and the result tested.


 5 | 3 | 4  Line encoding on entry: end coordinate according to table at left,
 ---------  start coordinate from table at left * 9. So upper left to lower
 2 | 0 | 1  right is 5*9+7 = 52.
 ---------
 8 | 6 | 7
****************************************************************************/

/****************************************************************************
  Region-clipping functions. Variables required are in regclipv.h.  Set
  isLine = 1 to disable support for YX banded blitRecs. Set blitMayOverlap = 1
  to enable set-up support for blits within the same bitmap that could
  potentially overlap.  If NO_REGION_CLIP is defined, Set_Up_Clip only sets
  up rect clipping, flagging region clipping as off, and all region-clipping
  routines are effectively no-oped, so region clipping is turned off (and the
  code is shrunk, too). Note that this no-oping relies on clipToRegionFlag
  never being turned on by Set_Up_Clip (*all* region code is not compiled if
  NOT_REGION_CLIP is defined, with only a return), so if NOT_REGION_CLIP is
  defined, clipToRegionFlag should always be left at 0.  Check_YX_Band,
  which doesn't require regions, stays the same.
****************************************************************************/


/***************************************************************************
* FUNCTION
*
*    CLIP_LineClipping
*
* DESCRIPTION
*
*    Checks against clip rectangle and either trivially rejects or clips
*    to the rectangle.  This routine performs capLast by cutting the length
*    and capFirst by advancing the Bresenham's variables by one point, and
*    will reject the line if zero points are left after capping.
*
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    STATUS        :   Returns NU_SUCCESS if successful initialization,
*                      else a negative value is returned.
*
****************************************************************************/
INT32 CLIP_LineClipping(VOID)
{
    INT32        tem;
    INT32        deltaX, deltaY;
    INT32        ZERO = 0;
    signed char  tempStat;
    INT32        status    = NU_SUCCESS;
    INT16        done      = NU_FALSE;
    INT16        roughClip = NU_FALSE;
    INT16        clipped   = NU_FALSE;
    INT32        clipPoints, errPoints;

    tempStat = drawStat;

    /* force the line to run top->bottom */
    if (dRect.Ymin > dRect.Ymax)
    {
        tem         = dRect.Ymin;
        dRect.Ymin  = dRect.Ymax;
        dRect.Ymax  = tem;
        tem         = dRect.Xmin;
        dRect.Xmin  = dRect.Xmax;
        dRect.Xmax  = tem;

         /* cap the other end now */
        tempStat    = -tempStat;

        /* flip the start and end sense of the rough clip index */
        lineEncode  = (lineEncode / 9) + ((lineEncode % 9) * 9);
    }

    deltaX = dRect.Xmax - dRect.Xmin;
    deltaY = dRect.Ymax - dRect.Ymin;
    if ((deltaX > 32767) || deltaY > 32767)
    {
        /* The line ends are too long. Post an error and skip the line.     */
        /* This line will not be draw, it is an error but will not crash    */
        status = -1;
        done   = NU_FALSE;
    };

    if(!done && status == NU_SUCCESS)
    {
        if (deltaX == 0)
        {
            /* vertical line (always top->bottom) */
            lineDir         = 1;

            /* force the minor axis to never advance */
            errTermAdjDownL = 0;
            errTermAdjUpL   = 0;
            errTermL        = -1;
            if ( lineEncode == 0 )
            {
                /* unclipped */
                if (tempStat < ZERO)
                {
                    /* first point null */
                    /* remove the first point*/
                    dRect.Ymin++;
                }
                else if (tempStat > ZERO)
                {
                    /* last point null */
                    /* remove the last point*/
                    dRect.Ymax--;
                }
                done = NU_TRUE;
            }

            if (!done && (lineEncode == 6))
            {
                /* clip bottom only */
                dRect.Ymax = cRect.Ymax;
                if (tempStat < ZERO)
                {
                    /* first point null */
                    /* remove the first point*/
                    dRect.Ymin++;
                }
                done = NU_TRUE;
            }

            if (!done &&  (lineEncode == 33))
            {
                /* clip top and bottom */
                dRect.Ymin = cRect.Ymin;
                dRect.Ymax = cRect.Ymax;
                done       = NU_TRUE;
                /* (First/last point null doesn't matter when clipped) */
            }


            if (!done && (lineEncode != 27))
            {
                status = -2;
                done   = NU_TRUE;
            }

            if(!done)
            {
                /* clip to top */
                dRect.Ymin = cRect.Ymin;
                if (tempStat > 0)
                {
                    /* last point null */
                    /* remove the last point*/
                    dRect.Ymax--;
                }
                done = NU_TRUE;
            }

            if( done && (status == NU_SUCCESS))
            {
                majorAxisLengthM1 = dRect.Ymax - dRect.Ymin;

                if (majorAxisLengthM1 < 0)
                {
                    /* because of capping,
                       line has become zero points long */
                    status = -3;
                }
            }
            /* vertical done */

        }

        if(status == NU_SUCCESS && !done )
        {
            if (deltaX > 0)
            {
                /* line is L->R */
                if (deltaY == 0)
                {
                    /* L->R, horizontal line */
                    lineDir         = 5;

                    /* force the minor axis to never advance */
                    errTermAdjDownL = 0;
                    errTermAdjUpL   = 0;
                    errTermL        = -1;

                    if (lineEncode == 0)
                    {
                        /* unclipped */
                        if (tempStat < ZERO)
                        {
                            /* first point null */
                            /* remove the first point */
                            dRect.Xmin++;
                        }
                        else if (tempStat > ZERO)
                        {
                            /* last point null */
                            /* remove the last point*/
                            dRect.Xmax--;
                        }
                        done = NU_TRUE;
                    }

                    if (!done && (lineEncode == 1))
                    {
                        /* clip right only */
                        dRect.Xmax = cRect.Xmax;
                        if (tempStat < ZERO)
                        {
                            /* first point null */
                            /* remove the first point*/
                            dRect.Xmin++;
                        }
                        done = NU_TRUE;
                    }

                    if (!done && (lineEncode == 19))
                    {
                        /* clip left and right */
                        dRect.Xmin = cRect.Xmin;
                        dRect.Xmax = cRect.Xmax;
                        done       = NU_TRUE;
                        /* (First/last point null don't matter when clipped) */
                    }

                    if (!done && (lineEncode != 18))
                    {
                        /* reject the line */
                        status = -4;
                        done   = NU_TRUE;
                    }

                    if(!done && (status == NU_SUCCESS))
                    {
                        /* clip to left */
                        dRect.Xmin = cRect.Xmin;
                        if (tempStat > 0)
                        {
                            /* last point null */
                            /* remove the last point*/
                            dRect.Xmax--;
                        }

                        done = NU_TRUE;
                    }

                    if( done && status == NU_SUCCESS)
                    {
                        majorAxisLengthM1 = dRect.Xmax - dRect.Xmin;

                        if (majorAxisLengthM1 < 0)
                        {
                           /* because of capping,line has become zero points long */
                           status = -5;
                        }
                    }
                }

                if(!done && (status == NU_SUCCESS))
                {
                    if (deltaX == deltaY)
                    {
                        /* it's a L->R diagonal line */
                        lineDir             = 3;

                        /* either axis can be the major axis */
                        majorAxisLengthM1   = deltaX;
                        errTermAdjDownL     = deltaX << 1;

                        /* initial error term (no fixup needed because we always go top->bottom) */
                        errTermL            = -deltaX;
                        errTermAdjUpL       = deltaY << 1;
                        done                = NU_TRUE;
                        roughClip           = NU_TRUE;
                    }

                    if(!done && !roughClip)
                    {
                        if(deltaX > deltaY)
                        {
                            /* L->R, Xmajor */
                            lineDir = 4;
                        }
                        else
                        {
                            /* L->R, Ymajor */
                            lineDir = 2;
                            tem     = deltaX;
                            deltaX  = deltaY;
                            deltaY  = tem;
                        }
                    }
                    if(!done && !roughClip)
                    {
                        /* this is deltaY if Ymajor */
                        majorAxisLengthM1 = deltaX;
                        errTermAdjDownL   = deltaX << 1;
                        errTermL          = - deltaX;
                        errTermAdjUpL     = deltaY << 1;
                        done              = NU_TRUE;
                        roughClip         = NU_TRUE;
                    }
                }
            }
        }

        if(!done && !roughClip && status == NU_SUCCESS)
        {
            /* line is R->L */
            deltaX = -deltaX;
            if (deltaY == 0)
            {
                /* R->L, horizontal line */
                /* switch endpoints */
                tem             = dRect.Xmin;
                dRect.Xmin      = dRect.Xmax;
                dRect.Xmax      = tem;
                lineEncode      = (lineEncode / 9) + ((lineEncode % 9) * 9);
                tempStat        = -tempStat;
                lineDir         = 5;

                /* force the minor axis to never advance */
                errTermAdjDownL = 0;
                errTermAdjUpL   = 0;
                errTermL        = -1;

                if (lineEncode == 0)
                {
                    /* unclipped */
                    if (tempStat < ZERO)
                    {
                        /* first point null */
                        /* remove the first point*/
                        dRect.Xmin++;
                    }
                    else if (tempStat > ZERO)
                    {
                        /* last point null */
                        /* remove the last point*/
                        dRect.Xmax--;
                    }
                    done = NU_TRUE;
                }

                if (!done && (lineEncode == 1))
                {
                    /* clip right only */
                    dRect.Xmax = cRect.Xmax;
                    if (tempStat < ZERO)
                    {
                        /* first point null */
                        /* remove the first point*/
                        dRect.Xmin++;
                    }
                    done = NU_TRUE;
                }

                if (!done && (lineEncode == 19))
                {
                    /* clip left and right */
                    dRect.Xmin = cRect.Xmin;
                    dRect.Xmax = cRect.Xmax;

                    done       = NU_TRUE;
                    /* (First/last point null don't matter when clipped) */
                }

                if (!done && (lineEncode != 18))
                {
                    /* reject the line */
                    status = -6;
                }

                if( !done && status == NU_SUCCESS)
                {
                    /* clip to left */
                    dRect.Xmin = cRect.Xmin;
                    if (tempStat > 0)
                    {
                        /* last point null */
                        /* remove the last point*/
                        dRect.Xmax--;
                    }
                    done = NU_TRUE;

                }

                if(done  && status == NU_SUCCESS)
                {
                    majorAxisLengthM1 = dRect.Xmax - dRect.Xmin;

                    if (majorAxisLengthM1 < 0)
                    {
                        /* because of capping, line has become zero points long */
                        status = -7;
                    }
                }
            }
        }
        if (!done && (status == NU_SUCCESS) && (deltaX == deltaY))
        {
            /* it's a R->L diagonal line */
            lineDir             = 6;

            /* either axis can be the major axis */
            majorAxisLengthM1   = deltaX;
            errTermAdjDownL     = deltaX << 1;

            /* initial error term (no fixup needed because we always go top->bottom) */
            errTermL            = -deltaX;
            errTermAdjUpL       = deltaY << 1;

            done                = NU_TRUE;
            roughClip           = NU_TRUE;
        }


        if(!done && !roughClip  && (status == NU_SUCCESS))
        {
            if (deltaX > deltaY)
            {
                /* R->L, Xmajor */
                lineDir = 7;
            }
            else
            {
                /* R->L, Ymajor */
                lineDir = 0;
                tem     = deltaX;
                deltaX  = deltaY;
                deltaY  = tem;
            }
        }
    }
    if(status == NU_SUCCESS && !done && !roughClip)
    {
        /* this is deltaY if Ymajor */
        majorAxisLengthM1 = deltaX;
        errTermAdjDownL   = deltaX << 1;
        errTermL          = - deltaX;
        errTermAdjUpL     = deltaY << 1;
    }

    if((done && roughClip && (status == NU_SUCCESS))
        || (!done && !roughClip && (status == NU_SUCCESS)))
    {
        /* Actual clipping is performed starting here, based on
           the rough clip info calculated above. */
        if (lineEncode >= 58)
        {
            /* reject */
            status = -8;
        }

        if(status == NU_SUCCESS)
        {
            done = NU_FALSE;

            switch (10 * (lineEncode / 9) + (lineEncode % 9))
            {
            case 0: /* trivial accept */
                if (tempStat == 0)
                {
                    /* no cap so return */
                    status = NU_SUCCESS;
                    done = NU_TRUE;
                }
                if (!done && tempStat > 0)
                {
                    /* count off the last point we're not going to draw */
                    majorAxisLengthM1--;

                    if (majorAxisLengthM1 < 0)
                    {
                        status = -10;
                    }

                    done = NU_TRUE;
                }

                if(!done)
                {
                    status = CLIP_HandleFirstPointNull();
                }
                break;
            case 1:
                /* | |
                  -----
                   |S|E
                  -----
                   | |  */
                /* Clip 01 */
                if (lineDir >= 3)
                {
                    /* Xmajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = cRect.Xmax - dRect.Xmin;
                    done = NU_TRUE;
                }
                if(!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (cRect.Xmax - dRect.Xmin)
                                         - errTermL - 1) / errTermAdjUpL;
                }

                clipped = NU_TRUE;
            case 2:
                /* | |
                  -----
                  E|S|
                  -----
                   | |  */
                /* Clip 02 */
                if(!clipped)
                {
                    if (lineDir >= 3)
                    {
                        /* Xmajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = dRect.Xmin - cRect.Xmin;
                        done = NU_TRUE;
                    }

                    if(!done)
                    {

                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (dRect.Xmin - cRect.Xmin)
                                            - errTermL - 1) / errTermAdjUpL;
                    }
                    clipped = NU_TRUE;
                }
            case 6:
                /* | |
                  -----
                   |S|
                  -----
                   |E|  */
                /* Clip 06 */
                if(!clipped)
                {
                    /* Initialize flag back to false */
                    done = NU_FALSE;

                    if (lineDir < 3)
                    {
                        /* Ymajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                        done = NU_TRUE;
                    }

                    if (lineDir == 4)
                    {
                        /* Ymajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                        done = NU_TRUE;
                    }

                    if(!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                                             - errTermL - 1) / errTermAdjUpL;
                    }
                    clipped = NU_TRUE;
                }
            case 7:
                /* | |
                  -----
                   |S|
                  -----
                   | |E */
                /* Clip 07 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrRightTocYmaxOrcXmax();
                    clipped = NU_TRUE;
                }
            case 8:
                /* | |
                  -----
                   |S|
                  -----
                   | |E */
                /* Clip 08 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrLeftTocYmaxOrcXmin();
                    clipped = NU_TRUE;
                }
                /* is first point null */
                if(clipped && drawStat < ZERO)
                {
                    status = CLIP_HandleFirstPointNull();
                }

                break;
            case 10:
                /* | |
                  -----
                   |E|S
                  -----
                   | |  */
                /* Clip 10 */

                /* # of points to clip edge along horizontal axis */
                clipPoints = dRect.Xmin - cRect.Xmax;

                /* new start X coordinate */
                dRect.Xmin = cRect.Xmax;
                if (lineDir >= 3)
                {
                 /* Xmajor, which is easier--calculate the distance to the
                    intercept and advance errTermL, dXmin, and dYmin accordingly,
                    and remove the clipped points from minorAxisLengthM1 */
                    majorAxisLengthM1 -= clipPoints;
                    errTermL          += (clipPoints * errTermAdjUpL);

                    if (errTermL < 0)
                    {
                     /* the error term didn't turn over
                        even once, so the minor axis doesn't advance at all */
                        done = NU_TRUE;
                    }
                    if(!done)
                    {
                        errPoints = (errTermL % errTermAdjDownL);
                        clipPoints = (errTermL / errTermAdjDownL) + 1;
                        dRect.Ymin += clipPoints;
                        errTermL = errPoints - errTermAdjDownL;
                        done = NU_TRUE;
                    }

                }

                if (!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate # points to skip */
                    errPoints         = errTermAdjDownL * (clipPoints - 1);
                    clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                    majorAxisLengthM1 -= clipPoints;
                    dRect.Ymin        += clipPoints;
                    errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                          errTermAdjDownL);
                }
                clipped = NU_TRUE;
            case 20:
                /* | |
                  -----
                  S|E|
                  -----
                   | |  */
                /* Clip 20 */
                if(!clipped)
                {

                    /* # of points to clip edge along horizontal axis */
                    clipPoints = cRect.Xmin - dRect.Xmin;

                    /* new start X coordinate */
                    dRect.Xmin = cRect.Xmin;
                    if (lineDir >= 3)
                    {
                     /* Xmajor, which is easier--calculate the distance to the
                        intercept and advance errTermL, dXmin, and dYmin accordingly,
                        and remove the clipped points from minorAxisLengthM1 */
                        majorAxisLengthM1 -= clipPoints;
                        errTermL          += (clipPoints * errTermAdjUpL);
                        if (errTermL < 0)
                        {
                         /* the error term didn't turn over
                            even once, so the minor axis doesn't advance at all */
                            done = NU_TRUE;
                        }

                        if(!done)
                        {
                            errPoints = (errTermL % errTermAdjDownL);
                            clipPoints = (errTermL / errTermAdjDownL) + 1;
                            dRect.Ymin += clipPoints;
                            errTermL = errPoints - errTermAdjDownL;
                            done = NU_TRUE;
                        }
                    }

                    if (!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate # points to skip */
                        errPoints         = errTermAdjDownL * (clipPoints - 1);
                        clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                        majorAxisLengthM1 -= clipPoints;
                        dRect.Ymin        += clipPoints;
                        errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                              errTermAdjDownL);
                    }
                    clipped = NU_TRUE;
                }
            case 30:
                /* |S|
                  -----
                   |E|
                  -----
                   | |  */
                /* Clip 30 */
                if(!clipped)
                {
                    CLIP_ClipTopTocYmin();
                    clipped = NU_TRUE;
                }
            case 40:
                /* | |S
                  -----
                   |E|
                  -----
                   | |  */
                /* Clip 40 */
                if(!clipped)
                {
                    CLIP_ClipTopOrRightTocYminOrcXmax();
                    clipped = NU_TRUE;
                }
            case 50:
                /*S| |
                  -----
                   |E|
                  -----
                   | |  */
                /* Clip 50 */
                if(!clipped)
                {
                    CLIP_ClipTopOrLeftTocYminOrcXmin();
                    clipped = NU_TRUE;
                }

                /* is last point null */
                if (clipped && drawStat > 0)
                {
                    /* count off the first point*/
                    majorAxisLengthM1--;
                    if (majorAxisLengthM1 < 0)
                    {
                        status = -12;
                    }
                }

                break;
            case 12:
                /* | |
                  -----
                  E| |S
                  -----
                  | |  */
                /* Clip 12 */
                if (lineDir >= 3)
                {
                    /* Xmajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = dRect.Xmin - cRect.Xmin;
                    done = NU_TRUE;
                }

                if(!done)
                {

                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (dRect.Xmin - cRect.Xmin)
                        - errTermL - 1) / errTermAdjUpL;
                }
                done = NU_FALSE;

                /* # of points to clip edge along horizontal axis */
                clipPoints = dRect.Xmin - cRect.Xmax;

                /* new start X coordinate */
                dRect.Xmin = cRect.Xmax;
                if (lineDir >= 3)
                {
                 /* Xmajor, which is easier--calculate the distance to the
                    intercept and advance errTermL, dXmin, and dYmin accordingly,
                    and remove the clipped points from minorAxisLengthM1 */
                    majorAxisLengthM1 -= clipPoints;
                    errTermL          += (clipPoints * errTermAdjUpL);

                    if (errTermL < 0)
                    {
                     /* the error term didn't turn over
                        even once, so the minor axis doesn't advance at all */
                        done = NU_TRUE;
                    }
                    if(!done)
                    {

                        errPoints = (errTermL % errTermAdjDownL);
                        clipPoints = (errTermL / errTermAdjDownL) + 1;
                        dRect.Ymin += clipPoints;
                        errTermL = errPoints - errTermAdjDownL;
                        done = NU_TRUE;
                    }

                }

                if (!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate # points to skip */
                    errPoints         = errTermAdjDownL * (clipPoints - 1);
                    clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                    majorAxisLengthM1 -= clipPoints;
                    dRect.Ymin        += clipPoints;
                    errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                          errTermAdjDownL);
                }
                break;
            case 16:
                /* | |
                  -----
                   | |S
                  -----
                   |E|  */
                /* Clip 16 */
                if (lineDir < 3)
                {
                    /* Ymajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                    done = NU_TRUE;
                }

                if(!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                                         - errTermL - 1) / errTermAdjUpL;
                }
                done = NU_FALSE;

                /* # of points to clip edge along horizontal axis */
                clipPoints = dRect.Xmin - cRect.Xmax;

                /* new start X coordinate */
                dRect.Xmin = cRect.Xmax;
                if (lineDir >= 3)
                {
                 /* Xmajor, which is easier--calculate the distance to the
                    intercept and advance errTermL, dXmin, and dYmin accordingly,
                    and remove the clipped points from minorAxisLengthM1 */
                    majorAxisLengthM1 -= clipPoints;
                    errTermL          += (clipPoints * errTermAdjUpL);

                    if (errTermL < 0)
                    {
                     /* the error term didn't turn over
                        even once, so the minor axis doesn't advance at all */
                        done = NU_TRUE;
                    }
                    if(!done)
                    {
                        errPoints = (errTermL % errTermAdjDownL);
                        clipPoints = (errTermL / errTermAdjDownL) + 1;
                        dRect.Ymin += clipPoints;
                        errTermL = errPoints - errTermAdjDownL;
                        done = NU_TRUE;
                    }

                }

                if (!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate # points to skip */
                    errPoints         = errTermAdjDownL * (clipPoints - 1);
                    clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                    majorAxisLengthM1 -= clipPoints;
                    dRect.Ymin        += clipPoints;
                    errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                          errTermAdjDownL);
                }
                clipped = NU_TRUE;
            case 18:
                /* | |
                  -----
                   | |S
                  -----
                  E| |  */
                /* Clip 18 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrLeftTocYmaxOrcXmin();

                    /* # of points to clip edge along horizontal axis */
                    clipPoints = dRect.Xmin - cRect.Xmax;

                    /* new start X coordinate */
                    dRect.Xmin = cRect.Xmax;
                    if (lineDir >= 3)
                    {
                     /* Xmajor, which is easier--calculate the distance to the
                        intercept and advance errTermL, dXmin, and dYmin accordingly,
                        and remove the clipped points from minorAxisLengthM1 */
                        majorAxisLengthM1 -= clipPoints;
                        errTermL          += (clipPoints * errTermAdjUpL);

                        if (errTermL < 0)
                        {
                         /* the error term didn't turn over
                            even once, so the minor axis doesn't advance at all */
                            done = NU_TRUE;
                        }
                        if(!done)
                        {

                            errPoints = (errTermL % errTermAdjDownL);
                            clipPoints = (errTermL / errTermAdjDownL) + 1;
                            dRect.Ymin += clipPoints;
                            errTermL = errPoints - errTermAdjDownL;
                            done = NU_TRUE;
                        }

                    }

                    if (!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate # points to skip */
                        errPoints         = errTermAdjDownL * (clipPoints - 1);
                        clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                        majorAxisLengthM1 -= clipPoints;
                        dRect.Ymin        += clipPoints;
                        errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                              errTermAdjDownL);
                    }
                    clipped = NU_TRUE;
                }
            case 26:
                /* | |
                  -----
                  S| |E
                  -----
                   | |  */
                /* Clip 26 */
                if(!clipped)
                {
                    if (lineDir < 3)
                    {
                        /* Ymajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                        done = NU_TRUE;
                    }

                    if(!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                                             - errTermL - 1) / errTermAdjUpL;
                    }

                    /* # of points to clip edge along horizontal axis */
                    clipPoints = cRect.Xmin - dRect.Xmin;

                    /* new start X coordinate */
                    dRect.Xmin = cRect.Xmin;
                    if (lineDir >= 3)
                    {
                     /* Xmajor, which is easier--calculate the distance to the
                        intercept and advance errTermL, dXmin, and dYmin accordingly,
                        and remove the clipped points from minorAxisLengthM1 */
                        majorAxisLengthM1 -= clipPoints;
                        errTermL          += (clipPoints * errTermAdjUpL);
                        if (errTermL < 0)
                        {
                         /* the error term didn't turn over
                            even once, so the minor axis doesn't advance at all */
                            done = NU_TRUE;
                        }

                        if(!done)
                        {
                            errPoints = (errTermL % errTermAdjDownL);
                            clipPoints = (errTermL / errTermAdjDownL) + 1;
                            dRect.Ymin += clipPoints;
                            errTermL = errPoints - errTermAdjDownL;
                            done = NU_TRUE;
                        }
                    }

                    if (!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate # points to skip */
                        errPoints         = errTermAdjDownL * (clipPoints - 1);
                        clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                        majorAxisLengthM1 -= clipPoints;
                        dRect.Ymin        += clipPoints;
                        errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                              errTermAdjDownL);
                    }
                    clipped = NU_TRUE;
                }
            case 27:
                /* | |
                  -----
                  S| |
                  -----
                   | |E */
                /* Clip 27 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrRightTocYmaxOrcXmax();

                    /* # of points to clip edge along horizontal axis */
                    clipPoints = cRect.Xmin - dRect.Xmin;

                    /* new start X coordinate */
                    dRect.Xmin = cRect.Xmin;
                    if (lineDir >= 3)
                    {
                     /* Xmajor, which is easier--calculate the distance to the
                        intercept and advance errTermL, dXmin, and dYmin accordingly,
                        and remove the clipped points from minorAxisLengthM1 */
                        majorAxisLengthM1 -= clipPoints;
                        errTermL          += (clipPoints * errTermAdjUpL);
                        if (errTermL < 0)
                        {
                         /* the error term didn't turn over
                            even once, so the minor axis doesn't advance at all */
                            done = NU_TRUE;
                        }

                        if(!done)
                        {
                            errPoints = (errTermL % errTermAdjDownL);
                            clipPoints = (errTermL / errTermAdjDownL) + 1;
                            dRect.Ymin += clipPoints;
                            errTermL = errPoints - errTermAdjDownL;
                            done = NU_TRUE;
                        }
                    }

                    if (!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate # points to skip */
                        errPoints         = errTermAdjDownL * (clipPoints - 1);
                        clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                        majorAxisLengthM1 -= clipPoints;
                        dRect.Ymin        += clipPoints;
                        errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                              errTermAdjDownL);
                    }

                    clipped = NU_TRUE;
                }
            case 31:
                /* |S|
                  -----
                   | |E
                  -----
                   | |  */
                /* Clip 31 */
                if(!clipped)
                {
                    if (lineDir >= 3)
                    {
                        /* Xmajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = cRect.Xmax - dRect.Xmin;
                        done = NU_TRUE;
                    }
                    if(!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Xmax - dRect.Xmin)
                                             - errTermL - 1) / errTermAdjUpL;
                        done = NU_TRUE;
                    }

                    CLIP_ClipTopTocYmin();
                    clipped = NU_TRUE;
                }
            case 32:
                /* |S|
                  -----
                  E| |
                  -----
                   | |  */
                /* Clip 32 */
                if(!clipped)
                {
                    if (lineDir >= 3)
                    {
                        /* Xmajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = dRect.Xmin - cRect.Xmin;
                        done = NU_TRUE;
                    }

                    if(!done)
                    {

                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (dRect.Xmin - cRect.Xmin)
                                            - errTermL - 1) / errTermAdjUpL;
                    }
                    CLIP_ClipTopTocYmin();
                    clipped = NU_TRUE;
                }
            case 37:
                /* |S|
                  -----
                   | |
                  -----
                   | |E  */
                /* Clip 37 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrRightTocYmaxOrcXmax();
                    CLIP_ClipTopTocYmin();
                    clipped = NU_TRUE;
                }
            case 38:
                /* |S|
                  -----
                   | |
                  -----
                  E| |   */
                /* Clip 38 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrLeftTocYmaxOrcXmin();
                    CLIP_ClipTopTocYmin();
                    clipped = NU_TRUE;
                }
                if (!done && clipped && (dRect.Ymin > cRect.Ymax))
                {
                    status = -13;
                }
                break;
            case 21:
                /* | |
                  -----
                  S| |E
                  -----
                   | |  */
                /* Clip 21 */

                if (lineDir >= 3)
                {
                    /* Xmajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = cRect.Xmax - dRect.Xmin;
                    done = NU_TRUE;
                }
                if(!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (cRect.Xmax - dRect.Xmin)
                                         - errTermL - 1) / errTermAdjUpL;
                }
                done = NU_FALSE;

                /* # of points to clip edge along horizontal axis */
                clipPoints = cRect.Xmin - dRect.Xmin;

                /* new start X coordinate */
                dRect.Xmin = cRect.Xmin;
                if (lineDir >= 3)
                {
                 /* Xmajor, which is easier--calculate the distance to the
                    intercept and advance errTermL, dXmin, and dYmin accordingly,
                    and remove the clipped points from minorAxisLengthM1 */
                    majorAxisLengthM1 -= clipPoints;
                    errTermL          += (clipPoints * errTermAdjUpL);
                    if (errTermL < 0)
                    {
                     /* the error term didn't turn over
                        even once, so the minor axis doesn't advance at all */
                        done = NU_TRUE;
                    }

                    if(!done)
                    {
                        errPoints = (errTermL % errTermAdjDownL);
                        clipPoints = (errTermL / errTermAdjDownL) + 1;
                        dRect.Ymin += clipPoints;
                        errTermL = errPoints - errTermAdjDownL;
                        done = NU_TRUE;
                    }
                }

                if (!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate # points to skip */
                    errPoints         = errTermAdjDownL * (clipPoints - 1);
                    clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                    majorAxisLengthM1 -= clipPoints;
                    dRect.Ymin        += clipPoints;
                    errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                          errTermAdjDownL);
                }
                break;
            case 36:
                /* |S|
                  -----
                   | |
                  -----
                   |E|  */
                /* Clip 36 */
                if (lineDir < 3)
                {
                    /* Ymajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                    done = NU_TRUE;
                }

                if(!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                                         - errTermL - 1) / errTermAdjUpL;
                }
                CLIP_ClipTopTocYmin();
                break;
            case 42:
                /* | |S
                  -----
                  E| |
                  -----
                   | |  */
                /* Clip 42 */
                if (lineDir >= 3)
                {
                    /* Xmajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = dRect.Xmin - cRect.Xmin;
                    done = NU_TRUE;
                }

                if(!done)
                {

                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (dRect.Xmin - cRect.Xmin)
                                        - errTermL - 1) / errTermAdjUpL;
                }
                clipped = NU_TRUE;
            case 46:
                /* | |S
                  -----
                   | |
                  -----
                   |E|  */
                /* Clip 46 */
                if(!clipped)
                {
                    if (lineDir < 3)
                    {
                        /* Ymajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                        done = NU_TRUE;
                    }

                    if(!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                                             - errTermL - 1) / errTermAdjUpL;
                    }
                    clipped = NU_TRUE;
                }
            case 48:
                /* | |S
                  -----
                   | |
                  -----
                  E| |  */
                /* Clip 48 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrLeftTocYmaxOrcXmin();
                    clipped = NU_TRUE;
                }
                if(clipped)
                {
                    status = CLIP_ClipTopOrRightTocYminOrcXmax();
                }
                break;
            case 51:
                /*S| |
                  -----
                   | |E
                  -----
                   | |  */
                /* Clip 51 */
                if (lineDir >= 3)
                {
                    /* Xmajor, which is easy--just chop off the end */
                    /* # of points yet to draw */
                    majorAxisLengthM1 = cRect.Xmax - dRect.Xmin;
                    done = NU_TRUE;
                }
                if(!done)
                {
                    /* we'll have to figure out the intercept */
                    /* calculate new length - 1 */
                    majorAxisLengthM1 = (errTermAdjDownL * (cRect.Xmax - dRect.Xmin)
                                         - errTermL - 1) / errTermAdjUpL;
                }
                clipped = NU_TRUE;
            case 56:
                /*S| |
                  -----
                   | |
                  -----
                   |E|  */
                /* Clip 56 */
                if(!clipped)
                {
                    if (lineDir < 3)
                    {
                        /* Ymajor, which is easy--just chop off the end */
                        /* # of points yet to draw */
                        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
                        done = NU_TRUE;
                    }

                    if(!done)
                    {
                        /* we'll have to figure out the intercept */
                        /* calculate new length - 1 */
                        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                                             - errTermL - 1) / errTermAdjUpL;
                    }
                    clipped = NU_TRUE;
                }
            case 57:
                /*S| |
                  -----
                   | |
                  -----
                   | |E */
                /* Clip 57 */
                if(!clipped)
                {
                    CLIP_ClipBottomOrRightTocYmaxOrcXmax();
                    clipped = NU_TRUE;
                }
                if(clipped)
                {
                    status = CLIP_ClipTopOrLeftTocYminOrcXmin();
                }
                break;
            default:
                status = -14;
                break;
            }
        }
    }
    return (status);
}


/***************************************************************************
* FUNCTION
*
*    CLIP_HandleFirstPointNull
*
* DESCRIPTION
*
*    Advances the line drawing parameters 1 point.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    STATUS        :   Returns NU_SUCCESS if successful initialization,
*                      else a negative is returned if there is only one point..
*
****************************************************************************/
INT32 CLIP_HandleFirstPointNull(VOID)
{
    INT32 status = NU_SUCCESS;
    INT16 done   = NU_FALSE;

    switch (lineDir)
    {
    case 0:
        /* r->l, t->b Ymajor */
        errTermL += errTermAdjUpL;

        /* do we advance along the minor axis? */
        if (errTermL >= 0)
        {
            /* yes */
            /* advance along the X (minor) axis */
            dRect.Xmin--;

            /* adjust the error term back down */
            errTermL -= errTermAdjDownL;
        }
        /* also does case 1 */

    case 1:
        /* vertical top->bottom */
        /* advance to the second point along the Y(major) axis */
        dRect.Ymin++;

        /* count off the first point*/
        majorAxisLengthM1--;
        if (majorAxisLengthM1 < 0)
        {
            /* done with line */
            status = -1;
        }
        done = NU_TRUE;
    case 2:
        if(!done && status == NU_SUCCESS)
        {
            /* l->r, t->b Ymajor */
            /* advance to the second point along the Y(major) axis */
            dRect.Ymin++;
            errTermL += errTermAdjUpL;
            /* do we advance along the minor axis? */
            if (errTermL >= 0)
            {
                /* yes */
                /* advance along the X (minor) axis */
                dRect.Xmin++;

                /* adjust the error term back down */
                errTermL -= errTermAdjDownL;
            }
            /* count off the first point*/
            majorAxisLengthM1--;
            if (majorAxisLengthM1 < 0)
            {
                /* done with line */
                status = -2;
            }
            done = NU_TRUE;
        }
    case 3:
        if(!done && status == NU_SUCCESS)
        {
            /* l->r, t->b diag */
            /* advance to the second point*/
            dRect.Xmin++;
            dRect.Ymin++;

            /* count off the first point*/
            majorAxisLengthM1--;
            if (majorAxisLengthM1 < 0)
            {
                status = -3;
            }
            done = NU_TRUE;
        }
    case 4:
         if(!done && status == NU_SUCCESS)
        {
            /* l->r, t->b Xmajor */
            errTermL += errTermAdjUpL;

            /* do we advance along the minor axis? */
            if (errTermL >= 0)
            {
                /* yes */
                /* advance along the Y (minor) axis */
                dRect.Ymin++;

                /* adjust the error term back down */
                errTermL -= errTermAdjDownL;
            }
            /* also does case 1 */
        }
    case 5:
         if(!done && status == NU_SUCCESS)
         {
            /* vertical top->bottom */
            /* advance to the second point along the X(major) axis */
            dRect.Xmin++;

            /* count off the first point*/
            majorAxisLengthM1--;

            if (majorAxisLengthM1 < 0)
            {
                /* done with line */
                status = -4;
            }
            done = NU_TRUE;
         }
    case 6:
        if(!done && status == NU_SUCCESS)
        {
            /* r->l, t->b diag */
            /* advance to the second point*/
            dRect.Xmin--;
            dRect.Ymin++;

            /* count off the first point*/
            majorAxisLengthM1--;
            if (majorAxisLengthM1 < 0)
            {
                /* done with line */
                status = -5;
            }
            done = NU_TRUE;
        }

    case 7:
         if(!done && status == NU_SUCCESS)
         {
            /* r->l, t->b Xmajor */
            errTermL += errTermAdjUpL;
            /* do we advance along the minor axis? */
            if (errTermL >= 0)
            {   /* yes */
                /* advance along the Y (minor) axis */
                dRect.Ymin++;

                /* adjust the error term back down */
                errTermL -= errTermAdjDownL;
            }
            /* advance to the second point along the X(major) axis */
            dRect.Xmin--;

            /* count off the first point*/
            majorAxisLengthM1--;
            if (majorAxisLengthM1 < 0)
            {
                /* done with line */
                status = -6;
            }
         }
    default:
        break;
    }
    return(status);
}



/***************************************************************************
* FUNCTION
*
*    CLIP_ClipBottomOrRightTocYmaxOrcXmax
*
* DESCRIPTION
*
*    Clips the end of a top->bottom,left->right line to match cYmax or cXmax,
*    whichever it crosses first.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_ClipBottomOrRightTocYmaxOrcXmax(VOID)
{
    INT32 temMajorAxisLength;
    INT16 done = NU_FALSE;

    /* get count until cYmax crossed */
    if (lineDir < 3)
    {
        /* Ymajor, which is easy--just chop off the end */
        /* # of points yet to draw */
        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;
        done = NU_TRUE;
    }

    if(!done)
    {
        /* we'll have to figure out the intercept */
        /* calculate new length - 1 */
        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                             - errTermL - 1) / errTermAdjUpL;
    }

    temMajorAxisLength = majorAxisLengthM1;

    done = NU_FALSE;
    /* get count until cXmax crossed */
    if (lineDir >= 3)
    {
        /* Xmajor, which is easy--just chop off the end */
        /* # of points yet to draw */
        majorAxisLengthM1 = cRect.Xmax - dRect.Xmin;
        done = NU_TRUE;
    }
    if(!done)
    {
        /* we'll have to figure out the intercept */
        /* calculate new length - 1 */
        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Xmax - dRect.Xmin)
                             - errTermL - 1) / errTermAdjUpL;
    }

    /* use the smaller of the two */
    if (majorAxisLengthM1 > temMajorAxisLength)
    {
        majorAxisLengthM1 = temMajorAxisLength;
    }

}

/***************************************************************************
* FUNCTION
*
*    CLIP_ClipBottomOrLeftTocYmaxOrcXmin
*
* DESCRIPTION
*
*    Clips the end of a top->bottom,right->left line to match cYmax or cXmin,
*    whichever it crosses first.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_ClipBottomOrLeftTocYmaxOrcXmin(VOID)
{
    INT32 temMajorAxisLength;

    /* get count until cYmax crossed */
    if (lineDir < 3)
    {
        /* Ymajor, which is easy--just chop off the end */
        /* # of points yet to draw */
        majorAxisLengthM1 = cRect.Ymax - dRect.Ymin;

        temMajorAxisLength = majorAxisLengthM1;

        majorAxisLengthM1 = (errTermAdjDownL * (dRect.Xmin - cRect.Xmin)
                            - errTermL) / errTermAdjUpL;
    }

    else
    {

        /* we'll have to figure out the intercept */
        /* calculate new length - 1 */
        majorAxisLengthM1 = (errTermAdjDownL * (cRect.Ymax - dRect.Ymin)
                             - errTermL - 1) / errTermAdjUpL;

        temMajorAxisLength = majorAxisLengthM1;

        /* Xmajor, which is easy--just chop off the end */
        /* # of points yet to draw */
        majorAxisLengthM1 = dRect.Xmin - cRect.Xmin;

    }

    /* use the smaller of the two */
    if (majorAxisLengthM1 > temMajorAxisLength)
    {
        majorAxisLengthM1 = temMajorAxisLength;
    }

}

/***************************************************************************
* FUNCTION
*
*    CLIP_ClipTopTocYmin
*
* DESCRIPTION
*
*    Clips the start of a top->bottom to match cYmin.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_ClipTopTocYmin(VOID)
{
    INT32 clipPoints, errPoints;
    INT16 done = NU_FALSE;

    /* # of points to clip edge along perpendicular axis */
    clipPoints = cRect.Ymin - dRect.Ymin;

    /* new start Y coordinate */
    dRect.Ymin = cRect.Ymin;
    if (lineDir < 3)
    {
     /* Ymajor, which is easier--calculate the distance to the
        intercept and advance errTermL, dXmin, and dYmin accordingly,
        and remove the clipped points from minorAxisLengthM1 */
        majorAxisLengthM1 -= clipPoints;
        errTermL          += (clipPoints * errTermAdjUpL);
        if (errTermL < 0)
        {
            /* the error term didn't turn over even once, so the minor axis doesn't advance at all */
            done = NU_TRUE;
        }


        if (!done)
        {
            errPoints  = (errTermL % errTermAdjDownL);
            clipPoints = (errTermL / errTermAdjDownL) + 1;
            if (lineDir == 0)
            {
                dRect.Xmin -= clipPoints;
            }
            else
            {
                dRect.Xmin += clipPoints;
            }

            errTermL = errPoints - errTermAdjDownL;
            done = NU_TRUE;
        }
    }

    if(!done)
    {
        /* we'll have to figure out the intercept */
        /* calculate # points to skip */
        errPoints         = errTermAdjDownL * (clipPoints - 1);
        clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
        majorAxisLengthM1 -= clipPoints;
        if (lineDir >= 6)
        {
            dRect.Xmin -= clipPoints;
        }
        else
        {
            dRect.Xmin += clipPoints;
        }

        errTermL += ((clipPoints * errTermAdjUpL) - errPoints -
                    errTermAdjDownL);
    }

}

/***************************************************************************
* FUNCTION
*
*    CLIP_ClipTopOrLeftTocYminOrcXmin
*
* DESCRIPTION
*
*    Clips the start of a top->bottom,left->right line to match cYmin or cXmin,
*    whichever it crosses first.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    Status returned is NU_SUCCESS or negative if the line is not visible.
*
****************************************************************************/
INT32 CLIP_ClipTopOrLeftTocYminOrcXmin(VOID)
{
    INT32 dXmin, dYmin, saveMajorAxis, saveErrTerm;
    INT32 clipPoints, errPoints;
    INT16 done = NU_FALSE;
    INT32 status = NU_SUCCESS;

    if (lineDir >= 3)
    {   /* X major; try the cXmin intercept first and see if
        it's the one we want */
        dYmin = dRect.Ymin; /* preserve variables that will be altered */
        dXmin = dRect.Xmin;
        saveMajorAxis = majorAxisLengthM1;
        saveErrTerm = errTermL;

        /* clip left */
        /* # of points to clip edge along horizontal axis */
        clipPoints = cRect.Xmin - dRect.Xmin;

        /* new start X coordinate */
        dRect.Xmin = cRect.Xmin;
        if (lineDir >= 3)
        {
         /* Xmajor, which is easier--calculate the distance to the
            intercept and advance errTermL, dXmin, and dYmin accordingly,
            and remove the clipped points from minorAxisLengthM1 */
            majorAxisLengthM1 -= clipPoints;
            errTermL          += (clipPoints * errTermAdjUpL);
            if (errTermL < 0)
            {
             /* the error term didn't turn over
                even once, so the minor axis doesn't advance at all */
                done = NU_TRUE;
            }

            if(!done)
            {
                errPoints = (errTermL % errTermAdjDownL);
                clipPoints = (errTermL / errTermAdjDownL) + 1;
                dRect.Ymin += clipPoints;
                errTermL = errPoints - errTermAdjDownL;
                done = NU_TRUE;
            }
        }

        /* is the new dYmin in the clip rect? */
        if (dRect.Ymin > cRect.Ymax)
        {
            /* no, it's above,the line isn't visible */
            status = -1;
            done = NU_TRUE;
        }

        if (dRect.Ymin < cRect.Ymin)
        {
            /* no, try cYmin clip */
            /* restore the variables to their original state */
            errTermL          = saveErrTerm;
            majorAxisLengthM1 = saveMajorAxis;
            dRect.Xmin        = dXmin;
            dRect.Ymin        = dYmin;

            /* calculate cYmin clipping */
            CLIP_ClipTopTocYmin();

            /* is the new dYmin in the clip rect? */
            if (dRect.Xmin > cRect.Xmax)
            {
                /* no */
                status = -2;
                done = NU_TRUE;
            }
        }

      if(!done)
      {
          /* yes, we have a usable intercept */
          done = NU_TRUE;
      }
    }

    if( !done )
    {
        /* Y major line; try the cYmin intercept first and see if it's
        the one we want */
        /* preserve variables that will be altered */
        dYmin         = dRect.Ymin;
        dXmin         = dRect.Xmin;
        saveMajorAxis = majorAxisLengthM1;
        saveErrTerm   = errTermL;

        /* clip top */
        CLIP_ClipTopTocYmin();

        /* is the new dYmin in the clip rect? */
        if (dRect.Xmin > cRect.Xmax)
        {
            /* no, it's outside, the line isn't visible */
            status = -1;
            done = NU_TRUE;
        }
        if (!done && (dRect.Xmin < cRect.Xmin))
        {
            /* no, try cXmin clip */
            /* restore the variables to their original state */
            errTermL          = saveErrTerm;
            majorAxisLengthM1 = saveMajorAxis;
            dRect.Xmin        = dXmin;
            dRect.Ymin        = dYmin;

            /* # of points to clip edge along horizontal axis */
            clipPoints = cRect.Xmin - dRect.Xmin;

            /* new start X coordinate */
            dRect.Xmin = cRect.Xmin;
            if (lineDir >= 3)
            {
             /* Xmajor, which is easier--calculate the distance to the
                intercept and advance errTermL, dXmin, and dYmin accordingly,
                and remove the clipped points from minorAxisLengthM1 */
                majorAxisLengthM1 -= clipPoints;
                errTermL          += (clipPoints * errTermAdjUpL);
                if (errTermL < 0)
                {
                 /* the error term didn't turn over
                    even once, so the minor axis doesn't advance at all */
                    done = NU_TRUE;
                }

                if(!done)
                {
                    errPoints = (errTermL % errTermAdjDownL);
                    clipPoints = (errTermL / errTermAdjDownL) + 1;
                    dRect.Ymin += clipPoints;
                    errTermL = errPoints - errTermAdjDownL;
                    done = NU_TRUE;
                }
            }

            if (!done)
            {
                /* we'll have to figure out the intercept */
                /* calculate # points to skip */
                errPoints         = errTermAdjDownL * (clipPoints - 1);
                clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                majorAxisLengthM1 -= clipPoints;
                dRect.Ymin        += clipPoints;
                errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                      errTermAdjDownL);
            }

            /* is the new dYmin in the clip rect? */
            if (dRect.Ymin > cRect.Ymax)
            {
                 /* no */
                status = -2;
            }
        }
    }

    return(status);
}

/***************************************************************************
* FUNCTION
*
*    CLIP_ClipTopOrRightTocYminOrcXmax
*
* DESCRIPTION
*
*    Clips the start of a top->bottom,right->left line to match cYmin or cXmax,
*    whichever it crosses first.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    Status returned is NU_SUCCESS or negative if the line is not visible.
*
****************************************************************************/
INT32 CLIP_ClipTopOrRightTocYminOrcXmax(VOID)
{
    INT32 dXmin, dYmin, saveMajorAxis, saveErrTerm;
    INT32 clipPoints, errPoints;
    INT32 status = NU_SUCCESS;
    INT16 done = NU_FALSE;

    if (lineDir >= 3)
    {
     /* X major; try the cXmin intercept first and see if
        it's the one we want */
        /* preserve variables that will be altered */
        dYmin           = dRect.Ymin;
        dXmin           = dRect.Xmin;
        saveMajorAxis   = majorAxisLengthM1;
        saveErrTerm     = errTermL;

        /* clip right */
        /* # of points to clip edge along horizontal axis */
        clipPoints = dRect.Xmin - cRect.Xmax;
        dRect.Xmin = cRect.Xmax;    /* new start X coordinate */
        if (lineDir >= 3)
        {
         /* Xmajor, which is easier--calculate the distance to the
            intercept and advance errTermL, dXmin, and dYmin accordingly,
            and remove the clipped points from minorAxisLengthM1 */
            majorAxisLengthM1 -= clipPoints;
            errTermL          += (clipPoints * errTermAdjUpL);

            if (errTermL < 0)
            {
             /* the error term didn't turn over
                even once, so the minor axis doesn't advance at all */
                done = NU_TRUE;
            }
            if(!done)
            {

                errPoints = (errTermL % errTermAdjDownL);
                clipPoints = (errTermL / errTermAdjDownL) + 1;
                dRect.Ymin += clipPoints;
                errTermL = errPoints - errTermAdjDownL;
                done = NU_TRUE;
            }

        }

        if (!done)
        {
            /* we'll have to figure out the intercept */
            /* calculate # points to skip */
            errPoints         = errTermAdjDownL * (clipPoints - 1);
            clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
            majorAxisLengthM1 -= clipPoints;
            dRect.Ymin        += clipPoints;
            errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                  errTermAdjDownL);
        }

        done = NU_FALSE;

        /* is the new dYmin in the clip rect? */
        if (dRect.Ymin > cRect.Ymax)
        {
            /* no, it's above, the line isn't visible */
            status = -1;
            done = NU_TRUE;
        }


        if (!done && (dRect.Ymin < cRect.Ymin))
        {
            /* no, try cYmin clip */
            /* restore the variables to their original state */
            errTermL            = saveErrTerm;
            majorAxisLengthM1   = saveMajorAxis;
            dRect.Xmin          = dXmin;
            dRect.Ymin          = dYmin;

            /* calculate cYmin clipping */
            CLIP_ClipTopTocYmin();

            /* is the new dYmin in the clip rect? */
            if (dRect.Xmin < cRect.Xmin)
            {
                /* no */
                status = -2;
                done = NU_TRUE;
            }
        }

         /* yes, we have a usable intercept */
        if (!done)
        {
            done = NU_TRUE;
        }
    }

    if (!done)
    {
      /* Y major line; try the cYmin intercept first and see if it's
         the one we want */
        /* preserve variables that will be altered */
        dYmin           = dRect.Ymin;
        dXmin           = dRect.Xmin;
        saveMajorAxis   = majorAxisLengthM1;
        saveErrTerm     = errTermL;

        /* clip top */
        CLIP_ClipTopTocYmin();

        /* is the new dYmin in the clip rect? */
        if (dRect.Xmin > cRect.Xmax)
        {
            /* no, it's outside,the line isn't visible */
            status = -3;
            done = NU_TRUE;
        }

        if (!done && (dRect.Xmin < cRect.Xmin))
        {
            /* no, try cXmin clip */
            /* restore the variables to their original state */
            errTermL            = saveErrTerm;
            majorAxisLengthM1   = saveMajorAxis;
            dRect.Xmin          = dXmin;
            dRect.Ymin          = dYmin;

            /* calculate cXmin clipping */
            /* # of points to clip edge along horizontal axis */
            clipPoints = dRect.Xmin - cRect.Xmax;

            /* new start X coordinate */
            dRect.Xmin = cRect.Xmax;
            if (lineDir >= 3)
            {   /* Xmajor, which is easier--calculate the distance to the
                intercept and advance errTermL, dXmin, and dYmin accordingly,
                and remove the clipped points from minorAxisLengthM1 */
                majorAxisLengthM1 -= clipPoints;
                errTermL          += (clipPoints * errTermAdjUpL);

                if (errTermL < 0)
                {
                 /* the error term didn't turn over
                    even once, so the minor axis doesn't advance at all */
                    done = NU_TRUE;
                }
                if(!done)
                {
                    errPoints = (errTermL % errTermAdjDownL);
                    clipPoints = (errTermL / errTermAdjDownL) + 1;
                    dRect.Ymin += clipPoints;
                    errTermL = errPoints - errTermAdjDownL;
                    done = NU_TRUE;
                }

            }

            if (!done)
            {
                /* we'll have to figure out the intercept */
                /* calculate # points to skip */
                errPoints         = errTermAdjDownL * (clipPoints - 1);
                clipPoints        = ((errPoints - errTermL - 1) / errTermAdjUpL) + 1;
                majorAxisLengthM1 -= clipPoints;
                dRect.Ymin        += clipPoints;
                errTermL          += ((clipPoints * errTermAdjUpL) - errPoints -
                                      errTermAdjDownL);
            }

            /* is the new dYmin in the clip rect? */
            if (dRect.Ymin > cRect.Ymax)
            {
                /* no */
                status = -4;
            }
        }
    }

    return(status);
}

/***************************************************************************
* FUNCTION
*
*    CLIP_ClipAndDrawEntry
*
* DESCRIPTION
*
*    Performs the trivial clipping/clipping categorization.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_ClipAndDrawEntry(VOID)
{
    INT16  done = NU_FALSE;

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)
        
    rect   temp_dRect,swap_dRect;

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef USING_DIRECT_X
    rect scrnRect;
#endif

    /* determine clipping category */
    lineEncode = 0;

    if (dRect.Xmin < cRect.Xmin)
    {
        lineEncode += 18;
    }
    else if (dRect.Xmin > cRect.Xmax)
    {
        lineEncode += 9;
    }

    if (dRect.Ymin < cRect.Ymin)
    {
        lineEncode += 27;
    }
    else if (dRect.Ymin > cRect.Ymax)
    {
        lineEncode += 54;
    }

    if (dRect.Xmax < cRect.Xmin)
    {
        lineEncode += 2;
    }
    else if (dRect.Xmax > cRect.Xmax)
    {
        lineEncode += 1;
    }

    if (dRect.Ymax < cRect.Ymin)
    {
        lineEncode += 3;
    }
    else if (dRect.Ymax > cRect.Ymax)
    {
        lineEncode += 6;
    }

    /* perform the clipping and set the drawing variables */
    if (CLIP_LineClipping() != NU_SUCCESS)
    {
        /* return if the line is completely clipped,
           with nothing left to draw */
        done = NU_TRUE;
    }
    if( !done )
    {
#ifdef USING_DIRECT_X
        scrnRect = dRect;
        if (scrnRect.Xmin > scrnRect.Xmax)
        {
            scrnRect.Xmin++;
        }
        else
        {
            scrnRect.Xmax++;
        }

        if (scrnRect.Ymin > scrnRect.Ymax)
        {
            scrnRect.Ymin++;
        }
        else
        {
            scrnRect.Ymax++;
        }
#endif

        /* point to the row table entry for the initial row */
        rowTablePtr[0] = (long *)(rowTablePtr[1] + dRect.Ymin);

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

        temp_dRect = dRect;
        
#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

        /* Vector to the low-level optimization routine */
        optPtr();

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

       /* Save current dRect value. */
       swap_dRect = dRect;

       /* Restore the dRect value before optPtr() call. */
       dRect = temp_dRect;

       /* Call the function for post-processing operations. */
       SCREENI_Display_Device.display_post_process_hook();

       /* Restore current dRect. */
       dRect = swap_dRect ;

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */
          
#ifdef USING_DIRECT_X
        /* Blit back buffer to screen */
        BlitBacktoFront(&scrnRect);
#endif

    }
}

#endif  /* THIN_LINE_OPTIMIZE */

#ifndef NO_REGION_CLIP

/***************************************************************************
* FUNCTION
*
*    CLIP_Blit_Clip_Region
*
* DESCRIPTION
*
*    Fills/blits a rectangle, clipped to a region, processing the region
*    from top->bottom, and left->right within each band for the blit routines.
*
* INPUTS
*
*    blitRcd *fcRec - pointer to blitRcd
*    rect *fillRect - pointer rect to draw, already clipped to the clip rect
*    rect *sRect    - pointer rect to draw from, already clipped to the clip
*                     rect; Ymin must be less than 0x7fff
*
* OUTPUTS
*
*    Returns a 1 if YX banded and there are more rectangles
*              0 for all else.
*
****************************************************************************/
INT32 CLIP_Blit_Clip_Region(blitRcd *fcRec, rect *fillRect, rect *sRect)
{

    INT16  done       = NU_FALSE;

    rect *regionRectPtr;
    INT16  bandFully  = NU_FALSE;
    INT16  bandBottom;

    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtr;

    /* Find a region band not entirely above dest rect. */
    while( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        regionRectPtr++;
    }

    /* Start at this point next time if the blitRcd is YX banded. */
    if( lclbfYXBanded != 0 )
    {
        nextRegionPtr = regionRectPtr;
    }

    /* Skip if the band is entirely below dest rect. */
    if( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        bandFully = NU_TRUE;
    }

    if( !bandFully )
    {
        /* So long as we're in a band that overlaps vertically with destRect,
           scan across from the left until we find a region rect that overlaps
           horizontally or we move to another band, then draw destRect clipped
           to each region rect until we either find a region rect that doesn't
           overlap or we move to another band, then scan until we move to another
           band. If the next band isn't past the bottom of destRect, repeat. */

        /* make a copy of dest rect, because we're going
           to alter the standard copy each time we clip it
           to a region rect */
        bRect = *fillRect;

        /* likewise, make a copy of the source rect */
        bsRect = *sRect;

        do
        {
            /* Initialize bandBottom to NU_FALSE for all passes through this loop */
            bandBottom = NU_FALSE;

            /* clipping and drawing loop */
            /* remember top edge of this YX band */
            bandYmin = regionRectPtr->Ymin;

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
                sYminTemp = bsRect.Ymin - (bRect.Ymin - dYminTemp);
            }
            else
            {
                dYminTemp = bRect.Ymin;
                sYminTemp = bsRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
                sYmaxTemp = bsRect.Ymax - (bRect.Ymax - dYmaxTemp);
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
                sYmaxTemp = bsRect.Ymax;
            }

            /* skip region rects in this band that are fully to the left of dest rect */
            while( bRect.Xmin >= regionRectPtr->Xmax  && !bandBottom)
            {
                regionRectPtr++;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin)
                {
                    bandBottom= NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmax >= regionRectPtr->Xmin && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;

                /* set the Y extent of the rectangle from which to draw (base
                   source rect clipped to YX band) */
                sRect->Ymin = sYminTemp;
                sRect->Ymax = sYmaxTemp;
                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                    sRect->Xmax = bsRect.Xmax - (bRect.Xmax - dRect.Xmax);
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                    sRect->Xmax = bsRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                    sRect->Xmin = bsRect.Xmin - (bRect.Xmin - dRect.Xmin);
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                    sRect->Xmin = bsRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr++;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin)
                {
                    bandBottom = NU_TRUE;
                }
            }
            if( !bandBottom )
            {
                /* skip region rects in this band that are fully to the right of
                   dest rect; basically, scan to the next YX band */
                do
                {
                    regionRectPtr++;
                } while( regionRectPtr->Ymin == bandYmin );
            }
            /* continue as long as the dest rect's bottom is below the region
               rect's top (they overlap vertically) */
            /* remember top edge of this YX band */
            bandYmin = regionRectPtr->Ymin;

        } while( bRect.Ymax > bandYmin );

       /* get next rectangle */
        done = NU_TRUE;
    }

    if(bandFully &&  regionRectPtr->Ymin != 0x7fff )
    {
        done = NU_FALSE;
    }

    if(bandFully && lclbfYXBanded != 0 )
    {
        done = NU_TRUE;
    }

    return(done);
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Fill_Clip_Region
*
* DESCRIPTION
*
*    Fills a rectangle, clipped to a region, processing the region from
*    top->bottom, and left->right within each band for the non-blit routines.
*
* INPUTS
*
*    blitRcd *fcRec - pointer to blitRcd
*    rect *fillRect - pointer rect to draw, already clipped to the clip rect
*                     Ymin must be less than 0x7fff
*
* OUTPUTS
*
*    Returns a 1 if YX banded and there are more rectangles
*              0 for all else.
*
****************************************************************************/
INT32 CLIP_Fill_Clip_Region(blitRcd *fcRec, rect *fillRect)
{
    INT16  done = NU_FALSE;

    rect *regionRectPtr;
    INT16  bandFully  = NU_FALSE;
    INT16  bandBottom = NU_FALSE;


    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtr;

    /* Find a region band not entirely above dest rect. */
    while (1)
    {
        if( fillRect->Ymin < regionRectPtr->Ymax )
        {
            break;
        }
        regionRectPtr++;
    }

    /* Start at this point next time if the blitRcd is YX banded. */
    if( lclbfYXBanded != 0 )
    {
        nextRegionPtr = regionRectPtr;
    }

    /* Skip if the band is entirely below dest rect. */
    if( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        bandFully = NU_TRUE;
    }

    if( !bandFully )
    {
        /* So long as we're in a band that overlaps vertically with destRect,
        scan across from the left until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the bottom of destRect, repeat. */
        bRect = *fillRect;  /* make a copy of dest rect, because we're going
                            to alter the standard copy each time we clip it
                            to a region rect */

        /* remember top edge of this YX band */
        bandYmin = regionRectPtr->Ymin;
        do
        {
            /* clipping and drawing loop */

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
            }
            else
            {
                dYminTemp = bRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
            }

            /* skip region rects in this band that are fully to the left of
            dest rect */
            while( bRect.Xmin >= regionRectPtr->Xmax && !bandBottom )
            {
                regionRectPtr++;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmax >= regionRectPtr->Xmin && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;
                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);
                regionRectPtr++;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            if( !bandBottom )
            {
                /* skip region rects in this band that are fully to the right of
                dest rect; basically, scan to the next YX band */
                do
                {
                    regionRectPtr++;

                } while( regionRectPtr->Ymin == bandYmin );
            }

            /* continue as long as the dest rect's bottom is below the region
            rect's top (they overlap vertically) */

            /* remember top edge of this YX band */
            bandBottom = NU_FALSE;
            bandYmin = regionRectPtr->Ymin;

        } while( bRect.Ymax > bandYmin  && !bandBottom);

         /* get next rectangle */
        done = NU_TRUE;
    }

    if(bandFully && regionRectPtr->Ymin != 0x7fff )
    {
        done = NU_FALSE;
    }

    if(bandFully && lclbfYXBanded != 0 )
    {
        done = NU_TRUE;;
    }

    return(done);
}

#endif  /* NO_REGION_CLIP */

/***************************************************************************
* FUNCTION
*
*    CLIP_Set_Up_Clip
*
* DESCRIPTION
*
*    Sets up rectangular and/or region clipping as specified by the blitRcd
*    clipBlit.
*
* INPUTS
*
*    blitRcd *clipBlit
*    rect    *clipR
*    INT32    blitMayOverlap
*    INT32    isLine
*
* OUTPUTS
*
*    Returns 1 if no drawing to do (region clip specified but region empty,
*    or both region and rectangular clip specified but rectangle and region
*    extent have no overlap) else 0.
*
****************************************************************************/
INT32 CLIP_Set_Up_Clip(blitRcd *clipBlit, rect *clipR, INT32 blitMayOverlap, INT32 isLine)
{
    INT16  status = NU_FALSE;

#ifndef NO_REGION_CLIP
    INT16  endEarly = NU_FALSE;
#endif

    if( isLine )
    {
        /* lines are never YX banded */
        lclbfYXBanded = 0;
    }
    else
    {
        /* 0 if not YX banded, !0 if is */
        lclbfYXBanded = clipBlit->blitFlags & bfYXBanded;
    }

    clipToRectFlag = clipBlit->blitFlags & bfClipRect;
    if( clipToRectFlag != 0 )
    {
        /* rect clipping; copy over the clip rect */
        *clipR = *clipBlit->blitClip;
    }

#ifndef NO_REGION_CLIP
                
    clipToRegionFlag = clipBlit->blitFlags & bfClipRegn;
    if (clipToRegionFlag == 0)
    {
        endEarly = NU_TRUE;
    }

    if( !endEarly )
    {
        if( blitMayOverlap )
        {
            /* point to the last rect in the region list */
            nextRegionPtrB = clipBlit->blitRegn->rgnListEnd;
        }

        nextRegionPtr = clipBlit->blitRegn->rgnList;

        /* if the first region rect is the end-of-region rect, then there are
        no active region rects and we're done */
        if( nextRegionPtr->Ymin == 0x07fff )
        {
            status = NU_TRUE;;
            endEarly = NU_TRUE;
        }

        if( !endEarly )
        {
            if( clipToRectFlag == 0 )
            {
                /* there is no rect clipping, so make the region extent the clip rect */
                *clipR = clipBlit->blitRegn->rgnRect;
                clipToRectFlag = 1; /* we'll clip to the region extent rect as if
                                       it were a normal clip rect */
                endEarly = NU_TRUE;
            }

            if( !endEarly )
            {
                /* rect clipping so improve efficiency by trimming the clip rect to the
                region extent, and/or trimming the region to the clip rect */
                if( clipR->Xmin < clipBlit->blitRegn->rgnRect.Xmin )
                {
                    clipR->Xmin = clipBlit->blitRegn->rgnRect.Xmin;
                }
                if( clipBlit->blitRegn->rgnRect.Ymin >= clipR->Ymin )
                {
                    clipR->Ymin = clipBlit->blitRegn->rgnRect.Ymin;
                }
                else
                {
                    if( clipR->Ymin == 0x7fff )
                    {
                        status = NU_TRUE;
                        endEarly = NU_TRUE;
                    }

                    if( !endEarly )
                    {

                        /* there's no way anything can be drawable with this clip rect;
                        at least part of the region is off the top of the clip rect,
                        so advance the region pointer to the first region rect that
                        isn't fully clipped off the top */
                        while( nextRegionPtr->Ymax <= clipR->Ymin )
                        {
                            nextRegionPtr++;
                        }
                    }
                }

                if( !endEarly)

                {
                    if( clipR->Xmax > clipBlit->blitRegn->rgnRect.Xmax )
                    {
                        clipR->Xmax = clipBlit->blitRegn->rgnRect.Xmax;
                    }
                    if( clipBlit->blitRegn->rgnRect.Ymax <= clipR->Ymax )
                    {
                        clipR->Ymax = clipBlit->blitRegn->rgnRect.Ymax;
                    }
                    else
                    {
                        if( blitMayOverlap )
                        {
                            /* advance (backwards) the region-end pointer to the first region
                            rect going backwards that isn't fully clipped off the bottom */
                            while( nextRegionPtrB->Ymin >= clipR->Ymax )
                            {
                                nextRegionPtrB--;
                            }
                            /* no need for overlap checking, since we already did that going
                            in the other direction */
                        }
                    }

                    /* done if there's no overlap between the clip rect and the region
                    extent */
                    if( (clipR->Xmin >= clipR->Xmax) || (clipR->Ymin >= clipR->Ymax) )
                    {
                        status   = NU_TRUE;
                    }
                }
            }
        }
    }
    
#endif

    return(status);
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Check_YX_Band_Blit
*
* DESCRIPTION
*
*    Checks to see whether the rectangle was trivially clipped because it
*    was fully below the clip rect, in which case we can discard the rest
*    of a YX banded blitList, or because it was fully above the clip rect,
*    in which case we can whiz ahead through a YX banded blitList until
*    we run out of rects or find a rect that isn't fully above the clip rect.
*
* INPUTS
*
*    rect *bandRect
*    rect *blitRectPtr
*    rect *clipR
*    INT32 *rectCnt
*
* OUTPUTS
*
*    Returns 1 if done with checking; else 0.
*
****************************************************************************/
INT32 CLIP_Check_YX_Band_Blit(rect *bandRect, rect *blitRectPtr, rect *clipR, INT32 *rectCnt)
{
    INT16  status = NU_FALSE;
    INT16  done   = NU_FALSE;

    if( lclbfYXBanded == 0 )
    {
        /* can't do anything clever; blitList is YX banded */
        done = NU_TRUE;
    }
    else if( bandRect->Ymin >= clipR->Ymax )
    {
        /* the rest of the blitList is also below and we're done */
        status = NU_TRUE;
        done = NU_TRUE;
    }
    else if( bandRect->Ymax > clipR->Ymin )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* dest rect is fully above the clip rect, so skip ahead until the
        dest rect does overlap the clip rect or we run out of rects */
        while( (blitRectPtr + 1)->Ymax <= clipR->Ymin && !done)
        {
            *rectCnt -= 1;  /* count off rects */
            if( *rectCnt < 0 )
            {
                /* done if no more rects */
                done = NU_TRUE;
                status = NU_TRUE;
            }

            if( !done && status == NU_FALSE)
            {
                /* point to next rect pair in blitList */
                blitRectPtr += 2;
            }
        }
    }
    return(status);
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Check_YX_Band_Blit
*
* DESCRIPTION
*
*    Checks to see whether the rectangle was trivially clipped because it
*    was fully below the clip rect, in which case we can discard the rest
*    of a YX banded blitList, or because it was fully above the clip rect,
*    in which case we can whiz ahead through a YX banded blitList until
*    we run out of rects or find a rect that isn't fully above the clip rect.
*    This function is for the non-blit case.
*
* INPUTS
*
*    rect *bandRect
*    rect *blitRectPtr
*    rect *clipR
*    INT32 *rectCnt
*
* OUTPUTS
*
*    Returns 1 if done with checking; else 0.
*
****************************************************************************/
INT32 CLIP_Check_YX_Band(rect *bandRect, rect *blitRectPtr, rect *clipR, INT32 *rectCnt)
{
    INT16  status = NU_FALSE;
    INT16  done   = NU_FALSE;

    if( lclbfYXBanded == 0 )
    {
        /* can't do anything clever; blitList is YX banded */
        done = NU_TRUE;
    }
    else if( bandRect->Ymin >= clipR->Ymax )
    {
        /* the rest of the blitList is also below and we're done */
        done = NU_TRUE;
        status = NU_TRUE;
    }
    else if( bandRect->Ymax > clipR->Ymin )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* dest rect is fully above the clip rect, so skip ahead until the
        dest rect does overlap the clip rect or we run out of rects */
        while( blitRectPtr->Ymax <= clipR->Ymin && !done)
        {
            *rectCnt -= 1;  /* count off rects */
            if( *rectCnt < 0 )
            {
                /* done if no more rects */
                done = NU_TRUE;
                status = NU_TRUE;
            }
            if( !done && status == NU_FALSE)
            {
                /* point to next rect in blitList */
                blitRectPtr++;
            }
        }
    }
    return(status);
}

#ifndef NO_REGION_CLIP

#ifdef  THIN_LINE_OPTIMIZE

/***************************************************************************
* FUNCTION
*
*    CLIP_Line_Clip_Region
*
* DESCRIPTION
*
*    Draws a line list, clipped to a region. This is a self-contained loop.
*
* INPUTS
*
*    INT32rectCnt
*    lineS *listPtr
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_Line_Clip_Region(INT32 rectCnt, lineS *listPtr)
{
    rect *lclRectPtr;
    INT16  bandBottom;

    while( --rectCnt >= 0 )
    {
        bandBottom = NU_FALSE;

        /* get the draw first/last status */
        drawStat = (signed char) listPtr->flStat;
        dRect.Xmin = listPtr->lStart.X;
        dRect.Ymin = listPtr->lStart.Y;
        dRect.Xmax = listPtr->lEnd.X;
        dRect.Ymax = listPtr->lEnd.Y;

        /* point to the next line to draw */
        listPtr++;

        /* make a copy of the dest rect, because the low level alters the
        standard copy each time we clip it to a region rect */
        bRect = dRect;

        /* put the limits in standard rect format (upper left->lower right) */
        if( dRect.Xmin > dRect.Xmax )
        {
            dRect.Xmin = dRect.Xmax;
            dRect.Xmax = bRect.Xmin;
        }
        if( dRect.Ymin > dRect.Ymax )
        {
            dRect.Ymin = dRect.Ymax;
            dRect.Ymax = bRect.Ymin;
        }

        /* clip the dest rect to the clip rect, for use in calculating
        region overlap (both are in old-style MW) */
        if( dRect.Xmin < bcXmin )
        {
            dRect.Xmin = bcXmin;
        }

        if( dRect.Ymin < bcYmin )
        {
            dRect.Ymin = bcYmin;
        }

        if( dRect.Xmax > bcXmax )
        {
            dRect.Xmax = bcXmax;
        }

        if( dRect.Ymax > bcYmax )
        {
            dRect.Ymax = bcYmax;
        }

        if( dRect.Xmin > dRect.Xmax )
        {
            /* line is fully clipped by clip rect */
            continue;
        }

        if( dRect.Ymin > dRect.Ymax)
        {
            /* line is fully clipped by clip rect */
            continue;
        }

        /* convert to X-style rect */
        dRect.Xmax++;
        dRect.Ymax++;
        cdXmin = dRect.Xmin;
        cdYmin = dRect.Ymin;
        cdXmax = dRect.Xmax;
        cdYmax = dRect.Ymax;

        /* point to the first region rect at which to start clipping */
        lclRectPtr = nextRegionPtr;

        /* Find a region band not entirely above clipped dest rect */
        while( dRect.Ymin >= lclRectPtr->Ymax )
        {
            lclRectPtr++;
        }

        if( lclRectPtr->Ymin > dRect.Ymax )
        {
           /* the band is fully below dest rect so there's no possible overlap */
            continue;
        }

        /* So long as we're in a band that overlaps vertically with the
        clipped destRect, scan across from the left until we find a region
        rect that overlaps horizontally or we move to another band, then
        draw destRect clipped to each region rect until we either find a
        region rect that doesn't overlap or we move to another band, then
        scan until we move to another band. If the next band isn't past
        the bottom of destRect, repeat. */

        /* remember top edge of this YX band */
        bandYmin = lclRectPtr->Ymin;
        do
        {
            /* set Y extent of dest rect, clipped to the Y extent of this band */
            if( lclRectPtr->Ymin >= cdYmin )
            {
                cRect.Ymin = lclRectPtr->Ymin;
            }
            else
            {
                cRect.Ymin = cdYmin;
            }
            if( lclRectPtr->Ymax <= cdYmax )
            {
                cRect.Ymax = lclRectPtr->Ymax;
            }
            else
            {
                cRect.Ymax = cdYmax;
            }

            /* convert from X-style to old MW style, which the
               clip and draw code expects */
            cRect.Ymax--;

            /* skip region rects in this band that are fully to the left
            of dest rect */
            while( cdXmin >= lclRectPtr->Xmax && !bandBottom)
            {
                lclRectPtr++;
                /* switch to next band if we just changed bands */
                if( lclRectPtr->Ymin != bandYmin)
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( cdXmax > lclRectPtr->Xmin && !bandBottom)
            {
                if( cdXmax <= lclRectPtr->Xmax )
                {
                    cRect.Xmax = cdXmax;
                }
                else
                {
                    cRect.Xmax = lclRectPtr->Xmax;
                }

                cRect.Xmax--;   /* convert from X-style to old MW style,
                                   which the clip and draw code expects */
                if( cdXmin >= lclRectPtr->Xmin )
                {
                    cRect.Xmin = cdXmin;
                }
                else
                {
                    cRect.Xmin = lclRectPtr->Xmin;
                }

                /* the region rect intersect clip rect intersect line =
                effective clipping rect is all set */

                dRect = bRect;  /* get back the line to draw */

                /* draw the line clipped to clip rect intersect region rect */
                LineDrawer();

                lclRectPtr++;
                /* switch to next band if we just changed bands */
                if( lclRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            if( !bandBottom)
            {
                /* skip region rects in this band that are fully to the right
                of dest rect;; basically, scan to the next YX band */
                do
                {
                    lclRectPtr++;
                }while( lclRectPtr->Ymin == bandYmin );
            }

            /* continue as long as the dest rect's bottom is below the
               region rect's top (they overlap vertically) */
            /* remember top edge of this YX band */
            bandYmin = lclRectPtr->Ymin;

        }while( cdYmax > bandYmin && !bandBottom);

    }   /* back for the next line */
}

#endif  /* THIN_LINE_OPTIMIZE */

/***************************************************************************
* FUNCTION
*
*    CLIP_Blit_Clip_Region_BT_RL
*
* DESCRIPTION
*
*    Fills/blits a rectangle, clipped to a region, processing the region from
*    bottom->top, and right->left within each band for the blit routines.
*
* INPUTS
*
*    fcRec    - pointer to blitRcd
*    fillRect - pointer rect to draw, already clipped to the clip rect
*    sRect    - pointer rect to draw from, already clipped to the clip rect; Ymin
*               must be less than 0x7fff
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_Blit_Clip_Region_BT_RL(blitRcd *fcRec, rect *fillRect, rect *sRect)
{
    rect *regionRectPtr;
    INT16  done = NU_FALSE;
    INT16  bandBottom = NU_FALSE;

    regionRectPtr = nextRegionPtrB; /* point to the first region rect at
                                       which to start clipping */
    /* Find a region band not entirely below dest rect. */
    while( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        regionRectPtr--;
    }

    /* Skip if the band is entirely above dest rect. */
    if( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* So long as we're in a band that overlaps vertically with destRect,
        scan across from the right until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the top of destRect, repeat. */

        /* make a copy of dest rect, because we're going to alter
           the standard copy each time we clip it to a region rect */
        bRect = *fillRect;

        /* likewise, make a copy of the source rect */
        bsRect = *sRect;

        /* remember bottom edge of this YX band */
        bandYmax = regionRectPtr->Ymax;
        do
        {
            /* clipping and drawing loop */
            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
                sYminTemp = bsRect.Ymin - (bRect.Ymin - dYminTemp);
            }
            else
            {
                dYminTemp = bRect.Ymin;
                sYminTemp = bsRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
                sYmaxTemp = bsRect.Ymax - (bRect.Ymax - dYmaxTemp);
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
                sYmaxTemp = bsRect.Ymax;
            }

            /* skip region rects in this band that are fully to the right of
            dest rect */
            while( bRect.Xmax <= regionRectPtr->Xmin  && !bandBottom)
            {
                regionRectPtr--;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymax != bandYmax)
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmin < regionRectPtr->Xmax && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped
                   to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;

                /* set the Y extent of the rectangle from which to draw (base
                source rect clipped to YX band) */
                sRect->Ymin = sYminTemp;
                sRect->Ymax = sYmaxTemp;

                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                    sRect->Xmax = bsRect.Xmax - (bRect.Xmax - dRect.Xmax);
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                    sRect->Xmax = bsRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                    sRect->Xmin = bsRect.Xmin - (bRect.Xmin - dRect.Xmin);
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                    sRect->Xmin = bsRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr--;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymax != bandYmax)
                {
                    bandBottom = NU_TRUE;
                }
            }

            if( !bandBottom )
            {
                /* skip region rects in this band that are fully to the left of
                dest rect; basically, scan to the next YX band */
                do
                {
                    regionRectPtr--;

                } while( regionRectPtr->Ymax == bandYmax );
            }

            /* continue as long as the dest rect's bottom is above the region
            rect's bottom (they overlap vertically) */
            /* remember bottom edge of this YX band */
            bandYmax = regionRectPtr->Ymax;

        } while( bRect.Ymin < bandYmax && !bandBottom);
    }

    return;
}


/***************************************************************************
* FUNCTION
*
*    CLIP_Fill_Clip_Region_BT_RL
*
* DESCRIPTION
*
*    Fills a rectangle, clipped to a region, processing the region from
*    bottom->top, and right->left within each band for the non-blit routines.
*
* INPUTS
*
*    fcRec    - pointer to blitRcd
*    fillRect - pointer rect to draw, already clipped to the clip rect
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_Fill_Clip_Region_BT_RL(blitRcd *fcRec, rect *fillRect)
{
    rect *regionRectPtr;
    INT16  bandBottom = NU_FALSE;
    INT16  done       = NU_FALSE;

    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtrB;

    /* Find a region band not entirely below dest rect. */
    while( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        regionRectPtr--;
    }

    /* Skip if the band is entirely above dest rect. */
    if( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* So long as we're in a band that overlaps vertically with destRect,
        scan across from the right until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the top of destRect, repeat. */

        /* make a copy of dest rect, because we're going to alter
           the standard copy each time we clip it to a region rect */
        bRect = *fillRect;

        /* remember bottom edge of this YX band */
        bandYmax = regionRectPtr->Ymax;
        do
        {
            /* clipping and drawing loop */

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
            }
            else
            {
                dYminTemp = bRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
            }

            /* skip region rects in this band that are fully to the right of
            dest rect */
            while( bRect.Xmax <= regionRectPtr->Xmin  && !bandBottom)
            {
                regionRectPtr--;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymax != bandYmax )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmin < regionRectPtr->Xmax && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped
                to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;
                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr--;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymax != bandYmax )
                {
                    bandBottom = NU_TRUE;
                }
            }

            if( !bandBottom )
            {
                /* skip region rects in this band that are fully to the left of
                dest rect; basically, scan to the next YX band */
                do
                {
                    regionRectPtr--;

                } while( regionRectPtr->Ymax == bandYmax );
            }

            /* continue as long as the dest rect's bottom is above the region
            rect's bottom (they overlap vertically) */

            /* remember bottom edge of this YX band */
            bandYmax = regionRectPtr->Ymax;

        } while( bRect.Ymin < bandYmax && !bandBottom);
    }
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Blit_Clip_Region_BT_LR
*
* DESCRIPTION
*
*    Fills/blits a rectangle, clipped to a region, processing the region from
*    bottom->top, and left->right within each band for the blit routines.
*
* INPUTS
*
*    fcRec    - pointer to blitRcd
*    fillRect - pointer rect to draw, already clipped to the clip rect
*    sRect    - pointer rect to draw from, already clipped to the clip rect; Ymin
*               must be less than 0x7fff
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_Blit_Clip_Region_BT_LR(blitRcd *fcRec, rect *fillRect, rect *sRect)
{
    rect *regionRectPtr;
    INT16  done       = NU_FALSE;
    INT16  bandBottom = NU_FALSE;

    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtrB;

    /* Find a region band not entirely below dest rect. */
    while( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        regionRectPtr--;
    }

    /* Skip if the band is entirely above dest rect. */
    if( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* Scan across from the right until we find the start of the band, then
        so long as we're in a band that overlaps vertically with destRect, scan
        back from there to the right until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the top of destRect, repeat. */
        bRect = *fillRect;  /* make a copy of dest rect, because we're going
                            to alter the standard copy each time we clip it
                            to a region rect */
        bsRect = *sRect;    /* likewise, make a copy of the source rect */

        do
        {
            /* clipping and drawing loop */

            /* remember bottom edge of this YX band */
            bandYmin = regionRectPtr->Ymin;

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
                sYminTemp = bsRect.Ymin - (bRect.Ymin - dYminTemp);
            }
            else
            {
                dYminTemp = bRect.Ymin;
                sYminTemp = bsRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
                sYmaxTemp = bsRect.Ymax - (bRect.Ymax - dYmaxTemp);
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
                sYmaxTemp = bsRect.Ymax;
            }

            /* Find the left end of the YX band by scanning to the left until
            the band changes. Actually overscans by one, ending up pointing
            to the rightmost rect in the next YX band. */
            do
            {
                regionRectPtr--;

            } while( regionRectPtr->Ymin == bandYmin );

            tempRegionPtr = regionRectPtr; /* remember where the next band starts */
            regionRectPtr++;               /* point to the leftmost region rect in the current YX band */

            /* skip region rects in this band that are fully to the left of
            dest rect */
            while( bRect.Xmin >= regionRectPtr->Xmax && !bandBottom)
            {
                regionRectPtr++;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                   bandBottom = NU_TRUE;;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmax > regionRectPtr->Xmin && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;

                /* set the Y extent of the rectangle from which to draw (base
                source rect clipped to YX band) */
                sRect->Ymin = sYminTemp;
                sRect->Ymax = sYmaxTemp;
                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                    sRect->Xmax = bsRect.Xmax - (bRect.Xmax - dRect.Xmax);
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                    sRect->Xmax = bsRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                    sRect->Xmin = bsRect.Xmin - (bRect.Xmin - dRect.Xmin);
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                    sRect->Xmin = bsRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr++;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* skip region rects in this band that are fully to the right of
            dest rect; basically, scan to the next YX band */

            regionRectPtr = tempRegionPtr;  /* point to the rightmost end of
                                               the next YX band */
        /* continue as long as the dest rect's top is above the region
        rect's bottom (they overlap vertically) */
        } while( bRect.Ymin < regionRectPtr->Ymax && !bandBottom);
    }
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Fill_Clip_Region_BT_LR
*
* DESCRIPTION
*
*    Fills a rectangle, clipped to a region, processing the region from
*    bottom->top, and left->right within each band for the non-blit routines.
*
* INPUTS
*
*    fcRec    - pointer to blitRcd
*    fillRect - pointer rect to draw, already clipped to the clip rect
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID CLIP_Fill_Clip_Region_BT_LR(blitRcd *fcRec, rect *fillRect)
{
    rect *regionRectPtr;
    INT16  done       = NU_FALSE;
    INT16  bandBottom = NU_FALSE;

    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtrB;

    /* Find a region band not entirely below dest rect. */
    while( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        regionRectPtr--;
    }

    /* Skip if the band is entirely above dest rect. */
    if( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* Scan across from the right until we find the start of the band, then
        so long as we're in a band that overlaps vertically with destRect, scan
        back from there to the right until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the top of destRect, repeat. */
        /* make a copy of dest rect, because we're going to alter
           the standard copy each time we clip it to a region rect */
        bRect = *fillRect;

        do
        {
            /* clipping and drawing loop */
            /* remember bottom edge of this YX band */
            bandYmin = regionRectPtr->Ymin;

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
            }
            else
            {
                dYminTemp = bRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
            }

            /* Find the left end of the YX band by scanning to the left until
            the band changes. Actually overscans by one, ending up pointing
            to the rightmost rect in the next YX band. */
            do
            {
                regionRectPtr--;

            } while( regionRectPtr->Ymin == bandYmin );

            tempRegionPtr = regionRectPtr;  /* remember where the next band starts */
            regionRectPtr++;    /* point to the leftmost region rect in the current YX band */

            /* skip region rects in this band that are fully to the left of dest rect */
            while( bRect.Xmin >= regionRectPtr->Xmax && !bandBottom)
            {
                regionRectPtr++;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmax > regionRectPtr->Xmin && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;

                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr++;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* skip region rects in this band that are fully to the right of
            dest rect; basically, scan to the next YX band */
            /* point to the rightmost end of the next YX band */
            regionRectPtr = tempRegionPtr;

        /* continue as long as the dest rect's top is above the region
        rect's bottom (they overlap vertically) */
        } while( bRect.Ymin < regionRectPtr->Ymax && !bandBottom);
    }
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Blit_Clip_Region_TB_RL
*
* DESCRIPTION
*
*    Fills a rectangle, clipped to a region, processing the region from
*    top->bottom, and right->left within each band for the blit routines.
*
* INPUTS
*
*    fcRec    - pointer to blitRcd
*    fillRect - pointer rect to draw, already clipped to the clip rect
*    sRect    - pointer rect to draw from, already clipped to the clip rect;
*               Ymin must be less than 0x7fff
*
* OUTPUTS
*
*    Returns a 0 if YX banded and there are more rectangles to draw, else 0.
*
****************************************************************************/
INT32 CLIP_Blit_Clip_Region_TB_RL(blitRcd *fcRec, rect *fillRect, rect *sRect)
{
    INT16  done            = NU_FALSE;
    rect *regionRectPtr;
    INT16  bandFullyBelow  = NU_FALSE;
    INT16  bandBottom      = NU_FALSE;

    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtr;

    /* Find a region band not entirely below dest rect. */
    while( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        regionRectPtr++;
    }

    /* Start at this point next time if the blitRcd is YX banded. */
    if( lclbfYXBanded != 0 )
    {
        nextRegionPtr = regionRectPtr;
    }

    /* Skip if the band is entirely below dest rect. */
    if( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        bandFullyBelow = NU_TRUE;
    }

    if( !bandFullyBelow )
    {
        /* Scan from the left until we find the right end of the band, then
        so long as we're in a band that overlaps vertically with destRect, scan
        back from there to the left until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the bottom of destRect, repeat. */
        bRect = *fillRect;  /* make a copy of dest rect, because we're going
                            to alter the standard copy each time we clip it
                            to a region rect */
        bsRect = *sRect;    /* likewise, make a copy of the source rect */

        do
        {
            /* clipping and drawing loop */

            /* remember bottom edge of this YX band */
            bandYmin = regionRectPtr->Ymin;

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
                sYminTemp = bsRect.Ymin - (bRect.Ymin - dYminTemp);
            }
            else
            {
                dYminTemp = bRect.Ymin;
                sYminTemp = bsRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
                sYmaxTemp = bsRect.Ymax - (bRect.Ymax - dYmaxTemp);
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
                sYmaxTemp = bsRect.Ymax;
            }

            /* Find the right end of the YX band by scanning to the right until
            the band changes. Actually overscans by one, ending up pointing
            to the leftmost rect in the next YX band. */
            do
            {
                regionRectPtr++;

            } while( regionRectPtr->Ymin == bandYmin );

            /* remember where the next band starts */
            tempRegionPtr = regionRectPtr;

            /* point to the rightmost region rect in the current YX band */
            regionRectPtr--;

            /* skip region rects in this band that are fully to the right of dest rect */
            while( bRect.Xmax <= regionRectPtr->Xmin && !bandBottom)
            {
                regionRectPtr--;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmin < regionRectPtr->Xmax && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;

                /* set the Y extent of the rectangle from which to draw (base
                source rect clipped to YX band) */
                sRect->Ymin = sYminTemp;
                sRect->Ymax = sYmaxTemp;

                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                    sRect->Xmax = bsRect.Xmax - (bRect.Xmax - dRect.Xmax);
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                    sRect->Xmax = bsRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                    sRect->Xmin = bsRect.Xmin - (bRect.Xmin - dRect.Xmin);
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                    sRect->Xmin = bsRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr--;
                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* point to the rightmost end of the next YX band */
            regionRectPtr = tempRegionPtr;

        /* continue as long as the dest rect's top is above the region
        rect's bottom (they overlap vertically) */
        } while( bRect.Ymax > regionRectPtr->Ymin && !bandBottom);

        done = NU_TRUE;
    }

    if(bandFullyBelow && regionRectPtr->Ymin != 0x7fff )
    {
        done = NU_FALSE;
    }

    if(bandFullyBelow && lclbfYXBanded != 0 )
    {
        done = NU_TRUE;
    }

    return(done);
}

/***************************************************************************
* FUNCTION
*
*    CLIP_Fill_Clip_Region_TB_RL
*
* DESCRIPTION
*
*    Fills a rectangle, clipped to a region, processing the region from
*    top->bottom, and right->left within each band for the non-blit routines.
*
* INPUTS
*
*    fcRec    - pointer to blitRcd
*    fillRect - pointer rect to draw, already clipped to the clip rect
*
* OUTPUTS
*
*    Returns a 0 if YX banded and there are more rectangles to draw, else 0.
*
****************************************************************************/
INT32 CLIP_Fill_Clip_Region_TB_RL(blitRcd *fcRec, rect *fillRect)
{
    INT16  done           = NU_FALSE;
    rect *regionRectPtr;
    INT16  bandFullyBelow = NU_FALSE;
    INT16  bandBottom     = NU_FALSE;

    /* point to the first region rect at which to start clipping */
    regionRectPtr = nextRegionPtr;

    /* Find a region band not entirely below dest rect. */
    while( fillRect->Ymin >= regionRectPtr->Ymax )
    {
        regionRectPtr++;
    }

    /* Start at this point next time if the blitRcd is YX banded. */
    if( lclbfYXBanded != 0 )
    {
        nextRegionPtr = regionRectPtr;
    }

    /* Skip if the band is entirely below dest rect. */
    if( fillRect->Ymax <= regionRectPtr->Ymin )
    {
        bandFullyBelow = NU_TRUE;
    }

    if( !bandFullyBelow )
    {
        /* Scan from the left until we find the right end of the band, then
        so long as we're in a band that overlaps vertically with destRect, scan
        back from there to the left until we find a region rect that overlaps
        horizontally or we move to another band, then draw destRect clipped
        to each region rect until we either find a region rect that doesn't
        overlap or we move to another band, then scan until we move to another
        band. If the next band isn't past the bottom of destRect, repeat. */
        /* make a copy of dest rect, because we're going to alter
           the standard copy each time we clip it to a region rect */
        bRect = *fillRect;
        do
        {
            /* clipping and drawing loop */

            /* remember bottom edge of this YX band */
            bandYmin = regionRectPtr->Ymin;

            /* set Y extent of dest rect, clipped to the Y extent of this YX band */
            if( bRect.Ymin < regionRectPtr->Ymin )
            {
                dYminTemp = regionRectPtr->Ymin;
            }
            else
            {
                dYminTemp = bRect.Ymin;
            }

            if( bRect.Ymax > regionRectPtr->Ymax )
            {
                dYmaxTemp = regionRectPtr->Ymax;
            }
            else
            {
                dYmaxTemp = bRect.Ymax;
            }

            /* Find the right end of the YX band by scanning to the right until
            the band changes. Actually overscans by one, ending up pointing
            to the leftmost rect in the next YX band. */
            do
            {
                regionRectPtr++;

            } while( regionRectPtr->Ymin == bandYmin );

            /* remember where the next band starts */
            tempRegionPtr = regionRectPtr;

            /* point to the rightmost region rect in the current YX band */
            regionRectPtr--;

            /* skip region rects in this band that are fully to the right of dest rect */
            while( bRect.Xmax <= regionRectPtr->Xmin )
            {
                regionRectPtr--;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                   bandBottom = NU_TRUE;
                }
            }

            /* start clipping and drawing */
            while( bRect.Xmin < regionRectPtr->Xmax  && !bandBottom)
            {
                /* set the Y extent of the rectangle to draw (base rect clipped
                to YX band) */
                dRect.Ymin = dYminTemp;
                dRect.Ymax = dYmaxTemp;

                if( bRect.Xmax > regionRectPtr->Xmax )
                {
                    dRect.Xmax = regionRectPtr->Xmax;
                }
                else
                {
                    dRect.Xmax = bRect.Xmax;
                }

                if( bRect.Xmin < regionRectPtr->Xmin )
                {
                    dRect.Xmin = regionRectPtr->Xmin;
                }
                else
                {
                    dRect.Xmin = bRect.Xmin;
                }

                /* fill the dest rect clipped to the region rect */
                FillDrawer(fcRec);

                regionRectPtr--;

                /* switch to next band if we just changed bands */
                if( regionRectPtr->Ymin != bandYmin )
                {
                    bandBottom = NU_TRUE;
                }
            }

            /* point to the rightmost end of the next YX band */
            regionRectPtr = tempRegionPtr;

            /* continue as long as the dest rect's top is above the region
            rect's bottom (they overlap vertically) */
        } while( bRect.Ymax > regionRectPtr->Ymin && !bandBottom);

        /* get next rectangle */
        done = NU_TRUE;
    }

    if(bandFullyBelow && regionRectPtr->Ymin != 0x7fff )
    {
        done = NU_FALSE;
    }

    if(bandFullyBelow && lclbfYXBanded != 0 )
    {
        done = NU_TRUE;
    }

    return(done);
}

#endif  /* NO_REGION_CLIP */

