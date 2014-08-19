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
*  linespcase.c                                                 
*
* DESCRIPTION
*
*  LineSPCase is a special-case optimization for patterned and dashed thin and square
*  pen line drawing to all types of bitmaps.
*
*  Note: Dashed lines get the patterns and dashing info from the current port,
*  rather than from the blitRcd. This module isn't a primitive, so it can
*  actually get any info it wants from the current port. (This is necessary for
*  the background pattern, at least, because there is no such blitRcd field.)
*  Therefore, the blitRcd passed in must agree with the port's default blitRcd,
*  or unexpected results may occur (like the first fg double-dash being in the
*  blitRcd's pen color, and the rest of the fg double-dashes being in the
*  current port's pen color).
*
*  Note:  Clipped dashed lines could be drawn much faster with high-level
*  clipping than with the current low-level pixel-by-pixel clipping; even
*  unclipped lines would be faster.  However, this requires bottom->top clip
*  handling and jumping dashes ahead to the clip, neither of which is trivial.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  LSPC_rsSpecialLinePatternAndSquare
*  HandlePattSq
*  DoSquareAsPoly
*  LSPC_rsLineSpecificSetup
*  LSPC_rsLineScanOutToRectList
*  LSPC_rsDashThinLine
*  DoubleDash
*  HandleWideSq
*  ProcessDashes
*  ProcessDashesSq
*  DoCapPoints
*  SetUpCommon
*
* DEPENDENCIES
*
*  rs_base.h
*  lines.h
*  linespcase.h
*  display_config.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/lines.h"
#include "ui/linespcase.h"
#include "drivers/display_config.h"
#include "ui/gfxstack.h"

/* Global Variables */
struct _lineState lineState;
dashPoly dashPoly1;
dashPoly dashPoly2;
blitVect *vListPtr;

#ifdef CFG_NU_OS_DRVR_DISPLAY_ENABLE

extern INT32 rectCnt;

#endif

/* Functions with local scope. */
static VOID HandlePattSq(VOID);
static VOID DoSquareAsPoly(rect *rlistPtr);
static INT32 LSPC_rsLineSpecificSetup(rect *rlistPtr);
static VOID LSPC_rsLineScanOutToRectList(VOID);

#ifdef  DASHED_LINE_SUPPORT

static VOID DoubleDash(VOID);
static VOID HandleWideSq(VOID);
static VOID ProcessDashes(rect *rlistPtr);
static VOID ProcessDashesSq(rect *rlistPtr, dashPoly *dashPolyPtr);
static VOID DoCapPoints(rect *rlistPtr, dashPoly *dashPolyPtr, INT32 capStl, INT32 capNd);

#endif  /* DASHED_LINE_SUPPORT */

static VOID SetUpCommon(blitRcd *LINEREC);

/***************************************************************************
* FUNCTION
*
*    LSPC_rsSpecialLinePatternAndSquare
*
* DESCRIPTION
*       
*    Function is a special-case optimization for patterned and square
*    pen line drawing to all types of bitmaps.
*
* INPUTS
*
*    blitRcd *LINEREC - Pointer to the line blitRcd.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID LSPC_rsSpecialLinePatternAndSquare(blitRcd *LINEREC)
{
    rect *rlistPtr;
    INT32 lRectCnt;

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    SetUpCommon(LINEREC);
    lRectCnt = rectCnt;

    if( (theGrafPort.pnFlags & pnSizeFlg) != 0 )
    {
        HandlePattSq();
    }

    else
    {
        /*  Buffer size  */
        lineState.lsMaxRects = MAX_RECTS;  

        /* Main drawing looping  */
        while( lRectCnt-- > 0 )
        {
            /* Setup the line start for this line  */
        
            /* Get draw first and last status  */
            lineState.lsSkipStat = (signed char) vListPtr->skipStat;
            rlistPtr = (rect *) ((SIGNED) vListPtr++);
            if( LSPC_rsLineSpecificSetup(rlistPtr) )
            {
                continue;
            }

            do
            {
                LSPC_rsLineScanOutToRectList();
                lclblitRec->blitDmap->prFill(lclblitRec);

            }while( lineState.lsMajorAxisLengthM1 >= 0 );
        }
    } /* else */

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();
}

/***************************************************************************
* FUNCTION
*
*    HandlePattSq
*
* DESCRIPTION
*
*     The function HandlePattSq is used for patterned/solid square pen wide lines.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID HandlePattSq(VOID)
{
    INT16 JumpDoSquareAsRectV = NU_FALSE;
    INT16 JumpDSARP1          = NU_FALSE;

    INT32 tem; SIGNED check;
    rect listRect, *rlistPtr;
    INT32 lRectCnt;

    lRectCnt = rectCnt;

    halfHeightUp        = theGrafPort.pnSize.Y >> 1;
    halfHeightDownRoDn  = halfHeightUp;
    halfHeightDownRoUp  = (theGrafPort.pnSize.Y + 1) >> 1;
    halfHeightDown      = halfHeightDownRoUp;

    halfWidthLeft       = theGrafPort.pnSize.X >> 1;
    halfWidthRightRoDn  = halfWidthLeft;
    halfWidthRightRoUp  = (theGrafPort.pnSize.X + 1) >> 1;
    halfWidthRight      = halfWidthRightRoUp; 

    oddPenSizeFlag = theGrafPort.pnSize.X | theGrafPort.pnSize.Y;

    /* Draw one rectangle at at time  */
    lclblitRec->blitCnt = 1; 
    rlistPtr = (rect *) lclblitRec->blitList;

    /* Main loop for drawing solid/patterned square pen line  */
    while( lRectCnt-- > 0 )
    {
        /* Build a fill record for the square pen wide line and call the 
           convex polygon filler (or the rectangle filler , for horizontal 
           and vertical lines)  */

        /* See if this is a horizontal or vertical line, and use the
           rectangle filler if so for maximum speed  */
        listRect = vListPtr->VectData;
        vListPtr++;
        if( listRect.Xmin == listRect.Xmax )
        {
            JumpDoSquareAsRectV = NU_TRUE;
        }

        if( !JumpDoSquareAsRectV )
        {
            if( listRect.Ymin != listRect.Ymax )
            {
                DoSquareAsPoly(&listRect);
                continue;
            }
            if( listRect.Xmin >= listRect.Xmax )
            {
                tem = listRect.Xmin;
                listRect.Xmin = listRect.Xmax;
                listRect.Xmax = tem;
            }
            JumpDSARP1 = NU_TRUE;
        }

        JumpDoSquareAsRectV = NU_FALSE;

        if( !JumpDSARP1 )
        {
            if( listRect.Ymin >= listRect.Ymax )
            {
                tem = listRect.Ymin;
                listRect.Ymin = listRect.Ymax;
                listRect.Ymax = tem;
            }
        }
        JumpDSARP1 = NU_FALSE;

        check = listRect.Xmin - halfWidthLeft;
        if( check < -32768 )
        {
            check = -32768;
        }
        listRect.Xmin = check;

        check = listRect.Xmax + halfWidthRight;
        if( check > 0x7FFF )
        {
            check = 0x7FFF;
        }
        listRect.Xmax = check;

        check = listRect.Ymin - halfHeightUp;
        if( check < -32768 )
        {
            check = -32768;
        }
        listRect.Ymin = check;

        check = listRect.Ymax + halfHeightDown;
        if( check > 0x7FFF )
        {
            check = 0x7FFF;
        }
        listRect.Ymax = check;

        *rlistPtr = listRect;
        lclblitRec->blitDmap->prFill(lclblitRec);

    } /* while( lRectCnt-- > 0 ) */

}

/***************************************************************************
* FUNCTION
*
*    DoSquareAsPoly
*
* DESCRIPTION
*
*     The function DoSquareAsPoly is used for square pen wide line as a poly lines.
*     Not used for horizontal or vertical.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID DoSquareAsPoly(rect *rlistPtr)
{
    SIGNED tem;
    INT32 majorMove,deltaX;

    /* Check and swap between min and max coord */
    if( rlistPtr->Ymin > rlistPtr->Ymax )
    {
        tem = rlistPtr->Xmin;
        rlistPtr->Xmin = rlistPtr->Xmax;
        rlistPtr->Xmax = tem;

        tem = rlistPtr->Ymin;
        rlistPtr->Ymin = rlistPtr->Ymax;
        rlistPtr->Ymax = tem;
    }

    /* Check for the size if 1x1 */
    if( oddPenSizeFlag == 1 )
    {
        deltaX = rlistPtr->Xmin - rlistPtr->Xmax;
        if( deltaX < 0 )
        {
            deltaX = -deltaX;
        }

        majorMove = deltaX - rlistPtr->Ymax + rlistPtr->Ymin;
        if( majorMove >= 0 )
        {
            halfWidthRight = halfWidthRightRoDn;
            halfHeightDown = halfHeightDownRoUp;
        }
        else
        {
            halfHeightDown = halfHeightDownRoDn;
            halfWidthRight = halfWidthRightRoUp;
        }
    }

    /* Set up the vertex list. For odd dimensions, bias the extra pixel down or
       to the right, because the polygon filler biases in favor of the top and left.
       Check for vertices that overflow out of the coordinate systems; such vertices 
       are simply forced back to the edge of coordinate system. This can distort the 
       edges of the wide line; no attempt is make to clip such vertices properly,
       they're just make valid so there's no garbage or crash  */
    /*
                    ;set all vertices that don't vary
                    ; with line X direction:
                    ;
                    ;  5     0        5 0
                    ;   X---X        or        X---X
                    ;   |    \                /    |
                    ;   o     \              /     o
                    ;  4 \     \            /     / 1
                    ;     \     \ 1      4 /     /
                    ;      \     o        o     /
                    ;       \    |        |    /
                    ;        X---X        X---X
                    ;       3     2      3     2            */

    tem = halfWidthRight + rlistPtr->Xmin;
    
    if( tem > 0x7FFF )
    {
        tem = 0x7FFF;
    }

    /* dashPoly1 is the buffer which points to dashPoly struct */   
    dashPoly1.FirstVert[0].X = tem;
    tem = rlistPtr->Ymin - halfHeightUp;
    if( tem < -32768 )
    {
        tem = -32768;
    }

    /* Y coord of upper right  */
    dashPoly1.FirstVert[0].Y = tem; 

    /* Y coordinate upper left  */
    dashPoly1.FirstVert[5].Y = tem;  

    tem = halfWidthRight + rlistPtr->Xmax;
    if( tem > 0x7FFF )
    {
        tem = 0x7FFF;
    }

    /* X coord of lower right  */
    dashPoly1.FirstVert[2].X = tem; 
    tem = halfHeightDown + rlistPtr->Ymax;
    if( tem > 0x7FFF )
    {
        tem = 0x7FFF;
    }

    /* Y coord of lower right  */
    dashPoly1.FirstVert[2].Y = tem; 

    /* Y coord of lower left   */
    dashPoly1.FirstVert[3].Y = tem; 

    tem = rlistPtr->Xmax - halfWidthLeft; 
    if( tem < -32768 )
    {
        tem = -32768;
    }

    /* X coord of lower left   */
    dashPoly1.FirstVert[3].X = tem; 
    tem = rlistPtr->Xmin - halfWidthLeft;
    if( tem < -32768 )
    {
        tem = -32768;
    }

    /* X coord of upper left   */
    dashPoly1.FirstVert[5].X = tem; 
    if( rlistPtr->Xmin > rlistPtr->Xmax )
    {
        /*     ;goes left; set vertices for:
            ;
            ;   5     0
            ;        o---o
            ;       /    |
            ;      /     X
            ;     /     / 1
            ;  4 /     /
            ;   X     /
            ;   |    /
            ;   o---o
            ;  3     2      */

        dashPoly1.FirstVert[1].X = dashPoly1.FirstVert[0].X;
        dashPoly1.FirstVert[4].X = dashPoly1.FirstVert[3].X;
        tem = rlistPtr->Ymin + halfHeightDown;
        if( tem > 0x7FFF )
        {
            /* Force to target valid value  */
            tem = 0x7FFF; 
        }

        dashPoly1.FirstVert[1].Y = tem;
        tem = rlistPtr->Ymax - halfHeightUp;
        if( tem < -32768 )
        {
            tem = -32768;
        }
        dashPoly1.FirstVert[4].Y = tem;
    }
    else
    {
        /*     ;goes right; set vertices for:
            ;
            ;  5     0
            ;   o---o
            ;   |    \
            ;   X     \
            ;  4 \     \
            ;     \     \ 1
            ;      \     X
            ;       \    |
            ;        o---o
            ;       3     2     */

        /* X coord of middle left  */
        dashPoly1.FirstVert[4].X = tem; 
        dashPoly1.FirstVert[1].X = dashPoly1.FirstVert[2].X;
        tem = rlistPtr->Ymax - halfHeightUp;
        if( tem < -32768 )
        {
            /* Force to smallest valid value  */
            tem = -32768; 
        }

        /* Y coord of middle right  */
        dashPoly1.FirstVert[1].Y = tem; 
        tem = rlistPtr->Ymin + halfHeightDown;
        if( tem > 0x7FFF )
        {
            /* Force to largest valid value  */
            tem = 0x7FFF; 
        }

        dashPoly1.FirstVert[4].Y = tem;
    }

    tem = MAX_RECTS*sizeof(rect) + sizeof(blitRcd);
    POLYGONS_rsScanAndDrawConvex( (UINT8 *)lclblitRec,tem,dashPoly1.FirstVert,6,coordModeOrigin,0,0 );

}

/***************************************************************************
* FUNCTION
*
*    LSPC_rsLineSpecificSetup
*
* DESCRIPTION
*
*    Sets up the drawing parameters for the specified line.
*    Expects dest rect to be in (AE, BE) (start point) and (CE, DE) (end point).
*    This routine performs skipLast by cutting the length and skipFirst by
*    advancing the Bresenham's variables by one point, and will reject the line
*    if zero points are left after skipping. The calling routine need not worry
*    about skipFirst/skipLast, and in fact should pay no attention to
*    lsSkipStat after the clipping call, because all skipping will already have
*    been performed.
*    This routine returns Carry set if the line is not visible at all and
*    therefore cannot be drawn. Otherwise, it sets the lsDXmin, lsDYmin,
*    initial lsErrTerm, lsMajorAxisLengthM1, lsLineDir, lsErrTermAdjUp, and
*    lsErrTermAdjDown variables appropriately for the line.
*
*    Line directions here are different from in general line drawing, because
*    horizontal, vertical, and diagonal lines aren't special-cased, and because
*    lines aren't forced to top->bottom (I'd like to force them to be top->
*    bottom, but if they were, dashes wouldn't flow properly). lsLineDir
*    settings are:
*
*    0 = r->l, t->b Ymajor
*    1 = l->r, t->b Ymajor
*    2 = l->r, t->b Xmajor
*    3 = r->l, t->b Xmajor
*    4 = r->l, b->t Ymajor
*    5 = l->r, b->t Ymajor
*    6 = l->r, b->t Xmajor
*    7 = r->l, b->t Xmajor
*
* INPUTS
*
*    rect *rlistPtr - Pointer to line state list rectangle.
*
* OUTPUTS
*
*    INT32 - Returns 0 if successful.
*          - Returns 1 if not successful.
*
***************************************************************************/
static INT32 LSPC_rsLineSpecificSetup(rect *rlistPtr)
{
    INT32 value = 0;

    INT16 JumpRToL           = NU_FALSE;
    INT16 JumpLToRBToT       = NU_FALSE;
    INT16 JumpSetInitialVars = NU_FALSE;
    INT16 JumpRToLBToT       = NU_FALSE;
    INT16 JumpRToLBToTYMajor = NU_FALSE;
    INT16 JumpLToRBToTYMajor = NU_FALSE;

    INT16 grafErrValue;
    INT32 tem = 0;
    SIGNED  check,deltaX,deltaY,temdelta;

    lineState.lsDXmin = rlistPtr->Xmin;
    lineState.lsDXmax = rlistPtr->Xmax;
    lineState.lsDYmin = rlistPtr->Ymin;
    lineState.lsDYmax = rlistPtr->Ymax;
    
    deltaX = rlistPtr->Xmax - rlistPtr->Xmin;
    deltaY = rlistPtr->Ymax - rlistPtr->Ymin;
    if((deltaX > 0x7FFF || deltaX < -32768) ||
       (deltaY > 0x7FFF || deltaY < -32768))
    {
        grafErrValue = c_LineTo + c_OfloLine;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        value = 1;
    }
    else
    {
        if( deltaX < 0 )
        {
            JumpRToL = NU_TRUE;
        }

        if( !JumpRToL )
        {
            if( deltaY < 0 )
            {
                JumpLToRBToT = NU_TRUE;
            }

            if( !JumpLToRBToT )
            {
                if( deltaX < deltaY )
                {
                    lineState.lsLineDir = 1;
                    temdelta = deltaX;
                    deltaX   = deltaY;
                    deltaY   = temdelta;
                    JumpSetInitialVars = NU_TRUE;
                }
                if( !JumpSetInitialVars )
                {
                    /* 2*SIZEOFF  */
                    lineState.lsLineDir = 2; 
                    JumpSetInitialVars = NU_TRUE;
                }
            }
        }
        JumpRToL = NU_FALSE;

        /* to remove paradigm warning */
        (VOID)JumpRToL;

        if( !JumpLToRBToT && !JumpSetInitialVars )
        {
            deltaX = -deltaX;
            if( deltaY < 0 )
            {
                JumpRToLBToT = NU_TRUE;
            }

            if( !JumpRToLBToT )
            {
                if( deltaX < deltaY )
                {
                    lineState.lsLineDir = 0;
                    temdelta = deltaX;
                    deltaX   = deltaY;
                    deltaY   = temdelta;
                    JumpSetInitialVars = NU_TRUE;
                }
                if( !JumpSetInitialVars )
                {
                    lineState.lsLineDir = 3;
                    JumpSetInitialVars = NU_TRUE;
                }
            }
        }

        JumpRToLBToT = NU_FALSE;

        /* to remove paradigm warning */
        (VOID)JumpRToLBToT;

        if( !JumpLToRBToT && !JumpSetInitialVars )
        {
            tem++;
            deltaY = -deltaY;

            if( deltaX < deltaY )
            {
                JumpRToLBToTYMajor = NU_TRUE;
            }
            if( !JumpRToLBToTYMajor )
            {
                lineState.lsLineDir = 7;
                JumpSetInitialVars = NU_TRUE;
            }
        }
        JumpLToRBToT = NU_FALSE;

        /* to remove paradigm warning */
        (VOID)JumpLToRBToT;

        if( !JumpSetInitialVars && !JumpRToLBToTYMajor )
        {
            tem++;
            deltaY = -deltaY;

            if( deltaX < deltaY )
            {
                JumpLToRBToTYMajor = NU_TRUE;
            }
            if( !JumpLToRBToTYMajor )
            {
                lineState.lsLineDir = 6;
                JumpSetInitialVars = NU_TRUE;
            }
        }
        JumpLToRBToTYMajor = NU_FALSE;

        /* to remove paradigm warning */
        (VOID)JumpLToRBToTYMajor;

        if( !JumpSetInitialVars && !JumpRToLBToTYMajor )
        {
            lineState.lsLineDir = 5;
            temdelta = deltaX;
            deltaX   = deltaY;
            deltaY   = temdelta;
            JumpSetInitialVars = NU_TRUE;
        }

        JumpRToLBToTYMajor = NU_FALSE;

        /* to remove paradigm warning */
        (VOID)JumpRToLBToTYMajor;

        if( !JumpSetInitialVars )
        {
            lineState.lsLineDir = 4;
            temdelta = deltaX;
            deltaX   = deltaY;
            deltaY   = temdelta;
        }
       
        /* Enter here to setup initial line-drawing variable  */
        JumpSetInitialVars = NU_FALSE;

        /* to remove paradigm warning */
        (VOID)JumpSetInitialVars;

        lineState.lsMajorAxisLengthM1 = deltaX;
        lineState.lsErrTermAdjDown = (deltaX << 1);
        deltaX  = -deltaX;
        deltaX -= tem;
        lineState.lsErrTerm      =  deltaX;
        lineState.lsErrTermAdjUp = (deltaY << 1);

        /* Finally, do first/last skipping, as needed  */
        if( lineState.lsSkipStat < 0 )
        {
            switch(lineState.lsLineDir)
            {
            /*  r->l, t->b Ymajor  */
            case 0:
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                lineState.lsErrTerm =  check;
                if( check > 0x7FFF )
                {
                    lineState.lsDXmin--;
                    lineState.lsErrTerm -= lineState.lsErrTermAdjDown;
                }
                lineState.lsDYmin++;
                lineState.lsMajorAxisLengthM1--;
                break;

            /*  l->r, t->b Ymajor  */
            case 1:
                lineState.lsDYmin++;
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                lineState.lsErrTerm = check;
                if( check > 0x7FFF )
                {
                    lineState.lsDXmin++;
                    lineState.lsErrTerm -= lineState.lsErrTermAdjDown;
                }
                lineState.lsMajorAxisLengthM1--;
                break;

            /* l->r, t->b Xmajor  */
            case 2:
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                if( check > 0x7FFF )
                {
                    lineState.lsDYmin++;
                    check -= lineState.lsErrTermAdjDown;
                }
                lineState.lsErrTerm = check;
                lineState.lsDXmin++;
                lineState.lsMajorAxisLengthM1--;
                break;

            /* r->l, t->b Xmajor  */
            case 3:
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                if( check > 0x7FFF )
                {
                    lineState.lsDYmin++;
                    check -= lineState.lsErrTermAdjDown;
                }
                lineState.lsErrTerm = check;
                lineState.lsDXmin--;
                lineState.lsMajorAxisLengthM1--;
                break;

            /* r->l, b->t Ymajor  */
            case 4:
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                if( check > 0x7FFF )
                {
                    lineState.lsDXmin--;
                    check -= lineState.lsErrTermAdjDown;
                }
                lineState.lsErrTerm = check;
                lineState.lsDYmin--;
                lineState.lsMajorAxisLengthM1--;
                break;

            /* l->r, b->t Ymajor  */
            case 5:
                lineState.lsDYmin--;
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                if( check > 0x7FFF )
                {
                    lineState.lsDXmin++;
                    check -= lineState.lsErrTermAdjDown;
                }
                lineState.lsErrTerm = check;
                lineState.lsMajorAxisLengthM1--;
                break;

            /* l->r, b->t Xmajor  */
            case 6:
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                if( check > 0x7FFF )
                {
                    lineState.lsDYmin--;
                    check -= lineState.lsErrTermAdjDown;
                }
                lineState.lsErrTerm = check;
                lineState.lsDXmin++;
                lineState.lsMajorAxisLengthM1--;
                break;

            /* r->l, b->t Xmajor  */
            case 7:
                check = lineState.lsErrTerm + lineState.lsErrTermAdjUp;
                if( check > 0x7FFF )
                {
                    lineState.lsDYmin--;
                    check -= lineState.lsErrTermAdjDown;
                }
                lineState.lsErrTerm = check;
                lineState.lsDXmin--;
                lineState.lsMajorAxisLengthM1--;
                break;
            } /* switch() */

        } /* if( lineState.lsSkipStat < 0 ) */
    
        if( lineState.lsSkipStat > 0 )
        {
            lineState.lsMajorAxisLengthM1--;
        }

    } /* else */

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    LSPC_rsLineScanOutToRectList
*
* DESCRIPTION
*
*    The function LSPC_rsLineScanOutToRectList is used to scan out as much as 
*    possible of a line into a rect list in a blitRcd.  Performs all clipping, 
*    and saves the state in a lineState structure for restarting, if necessary.
*    Line is fully scanned if the lsMajorAxisLengthM1 field is < 0.
*    Only generates lsMaxRects-1 as the maximum blitCnt; an additional rect is
*    placed at the end of every burst (so there are blitCnt+1 rects in the
*    blitList), but isn't counted in blitCnt; this is to allow for proper angle
*    correction on the last pixel in dash handling.
*
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID LSPC_rsLineScanOutToRectList(VOID)
{
    rect *rlistPtr;
    INT32 temX,temY,tem;
    SIGNED error;

    tem = lineState.lsMajorAxisLengthM1 + 1;
    if (tem >= lineState.lsMaxRects )
    {
        tem = lineState.lsMaxRects - 1;
    }

    /* Set the # of rects for the filler to drawer after we generate them  */
    lclblitRec->blitCnt = tem;

    /* # of points - 1 for next time  */
    lineState.lsMajorAxisLengthM1 -= tem;
    rlistPtr = (rect *) lclblitRec->blitList;

    /* point to the location at which to store the first pixels rect  */
    switch(lineState.lsLineDir)
    {
    case 0: 
        temX  = lineState.lsDXmin + 1;
        temY  = lineState.lsDYmin;
        error = lineState.lsErrTerm;    
        while( tem-- > 0 )
        {
            rlistPtr->Xmax = temX--;
            rlistPtr->Ymin = temY++;
            rlistPtr->Xmin = temX;
            rlistPtr->Ymax = temY;
            rlistPtr++;
            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temX++;     
            }
            else
                error -= lineState.lsErrTermAdjDown;
        }
        lineState.lsErrTerm = error;
        lineState.lsDYmin = temY;
        rlistPtr->Xmax = temX--;
        rlistPtr->Ymin = temY++;
        rlistPtr->Xmin = temX;
        rlistPtr->Ymax = temY;
        lineState.lsDXmin = temX;
        break; /* switch( ) */

    case 1: 
        temX  = lineState.lsDXmin;
        temY  = lineState.lsDYmin;
        error = lineState.lsErrTerm;
        while( tem-- >0 )
        {
            rlistPtr->Xmin = temX++;
            rlistPtr->Ymin = temY++;
            rlistPtr->Xmax = temX;
            rlistPtr->Ymax = temY;
            rlistPtr++;
            error += lineState.lsErrTermAdjUp;  
            if( error < 0 )
            {
                temX--;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }
        lineState.lsErrTerm = error;
        lineState.lsDXmin = temX;
        lineState.lsDYmin = temY;

        rlistPtr->Xmin = temX++;
        rlistPtr->Ymin = temY++;
        rlistPtr->Xmax = temX;
        rlistPtr->Ymax = temY;
        break; /* switch( ) */

    case 2: 
        temX  = lineState.lsDXmin;
        temY  = lineState.lsDYmin;
        error = lineState.lsErrTerm;
        while( tem-- > 0 )
        {
            rlistPtr->Xmin = temX++;
            rlistPtr->Ymin = temY++;
            rlistPtr->Xmax = temX;
            rlistPtr->Ymax = temY;
            rlistPtr++;
            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temY--;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }
        lineState.lsErrTerm = error;
        lineState.lsDXmin = temX;
        lineState.lsDYmin = temY;
        rlistPtr->Xmin = temX++;
        rlistPtr->Ymin = temY++;
        rlistPtr->Xmax = temX;
        rlistPtr->Ymax = temY;
        break; /* switch( ) */

    case 3:
        temX = lineState.lsDXmin + 1;
        temY = lineState.lsDYmin;
        error = lineState.lsErrTerm;
        while( tem-- >0 )
        {
            rlistPtr->Xmax = temX--;
            rlistPtr->Ymin = temY++;
            rlistPtr->Xmin = temX;
            rlistPtr->Ymax = temY;
            rlistPtr++;
            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temY--;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }
        lineState.lsErrTerm = error;
        lineState.lsDYmin = temY;
        rlistPtr->Xmax = temX--;
        rlistPtr->Ymin = temY++;
        rlistPtr->Xmin = temX;
        rlistPtr->Ymax = temY;
        lineState.lsDXmin = temX;
        break; /* switch( ) */

    case 4:
        temX = lineState.lsDXmin + 1;
        temY = lineState.lsDYmin + 1;
        error = lineState.lsErrTerm;
        while( tem-- > 0 )
        {
            rlistPtr->Xmax = temX--;
            rlistPtr->Ymax = temY--;
            rlistPtr->Ymin = temY;
            rlistPtr->Xmin = temX;
            rlistPtr++;
            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temX++;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }
        lineState.lsErrTerm = error;
        rlistPtr->Ymax = temY--;
        rlistPtr->Xmax = temX--;
        rlistPtr->Xmin = temX;
        rlistPtr->Ymin = temY;
        lineState.lsDXmin = temX;
        lineState.lsDYmin = temY;
        break; /* switch( ) */
  
    case 5:
        temX = lineState.lsDXmin;
        temY = lineState.lsDYmin + 1;
        error = lineState.lsErrTerm;
        while( tem-- > 0 )
        {
            rlistPtr->Xmin = temX++;
            rlistPtr->Ymax = temY--;
            rlistPtr->Xmax = temX;
            rlistPtr->Ymin = temY;
            rlistPtr++;
            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temX--;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }
        lineState.lsErrTerm = error;
        lineState.lsDXmin   = temX;
        rlistPtr->Xmin = temX++;
        rlistPtr->Ymax = temY--;
        rlistPtr->Xmax = temX;
        rlistPtr->Ymin = temY;
        lineState.lsDYmin = temY;
        break; /* switch( ) */

    case 6:
        temX = lineState.lsDXmin;
        temY = lineState.lsDYmin + 1;
        error = lineState.lsErrTerm;
        while( tem-- > 0 )
        {
            rlistPtr->Xmin = temX++;
            rlistPtr->Ymax = temY--;
            rlistPtr->Ymin = temY;
            rlistPtr->Xmax = temX;
            rlistPtr++;

            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temY++;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }

        lineState.lsErrTerm = (INT16) error;
        lineState.lsDXmin = temX;
        rlistPtr->Xmin = temX++;
        rlistPtr->Ymax = temY--;
        rlistPtr->Ymin = temY;
        rlistPtr->Xmax = temX;
        lineState.lsDYmin = temY;
        break; /* switch( ) */

    case 7: 
        temX = lineState.lsDXmin + 1;
        temY = lineState.lsDYmin + 1;
        error = lineState.lsErrTerm;
        while( tem-- >0 )
        {
            rlistPtr->Xmax = temX--;
            rlistPtr->Ymax = temY--;
            rlistPtr->Xmin = temX;
            rlistPtr->Ymin = temY;
            rlistPtr++;

            error += lineState.lsErrTermAdjUp;
            if( error < 0 )
            {
                temY++;
            }
            else
            {
                error -= lineState.lsErrTermAdjDown;
            }
        }
        lineState.lsErrTerm = error;
        rlistPtr->Xmax = temY--;
        rlistPtr->Ymax = temX--;
        rlistPtr->Xmin = temX;
        rlistPtr->Ymin = temY;
        lineState.lsDYmin = temY;
        lineState.lsDXmin = temX;
        break; /* switch( ) */
    }
    
}

#ifdef  DASHED_LINE_SUPPORT
/***************************************************************************
* FUNCTION
*
*    LSPC_rsDashThinLine
*
* DESCRIPTION
*
*    The function LSPC_rsDashThinLine is used for dash thin line drawing.
*
* INPUTS
*
*    blitRcd *LINEREC - Pointer to the line blitRcd.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID LSPC_rsDashThinLine(blitRcd *LINEREC)
{
    SIGNED temcount;
    INT16 temindex;
    UINT8 *temlist;
    rect *rlistPtr;
    INT32 lclRectCnt;

    /* Foreground pattern  */
    workingPnPat  = theGrafPort.pnPat;   

    /* Background pattern  */
    workingBkPat  = theGrafPort.bkPat;   

    /* dash style and current state  */
    dashFlags     = theGrafPort.pnFlags; 

    temcount      = theGrafPort.pnDashCnt << 7;
    temindex      = theGrafPort.pnDashNdx;
    dashElements  = theGrafPort.pnDashRcd[theGrafPort.pnDash].dashSize;
    dashListStart = (UINT8 *) theGrafPort.pnDashRcd[theGrafPort.pnDash].dashList;

    /* use to move along the dash list  */
    temlist = dashListStart; 

    /* If dashCount is 0, then dashCount is initialized to the first entry
       in the dash list, dashIndex is forced to the index of the first non-
       zero-length dash, and the dash state is set to the first non-zero-
       length dash's state; that is, the dash sequence is restarted. An
       infinite loop will result if all dash lengths are 0!  */

    if( theGrafPort.pnDashCnt <= 0 )
    {
        /*  Is it a zero count, so reinitialize the dash sequence */
        temcount = 1;

        /* to remove paradigm warning */
        (VOID)(INT16) temcount;

        temindex = -1;
        dashFlags &= ~pnDashState;
        do
        {
            dashFlags ^= pnDashState;
            temindex++;
            temcount = (*temlist++) << 7;
        }while( temcount == 0 );
    }

    dashCount = temcount;
    dashIndex = temindex;
    SetUpCommon(LINEREC);

    /* Check for square pen wide line  */
    if( dashFlags & pnSizeFlg )
    {
        HandleWideSq();
    }

    /* Check for double dash line  */
    else if( (dashFlags & pnDashStyle) )
    {
        DoubleDash();
    }

    else
    {
        /* on-off line */
        lineState.lsMaxRects = MAX_RECTS;

        /* Main on-off line-drawing loop  */
        lclRectCnt = rectCnt;
    
        while( lclRectCnt-- > 0 )
        {
            /* Set up the line state for this line.  */
            lineState.lsSkipStat = (signed char) vListPtr->skipStat;
            rlistPtr = (rect *) ((SIGNED) vListPtr++);

            if( LSPC_rsLineSpecificSetup(rlistPtr) )
            {
                continue;
            }

            do
            {
                LSPC_rsLineScanOutToRectList();
                rlistPtr = (rect *) lclblitRec->blitList;
                ProcessDashes(rlistPtr);

                /* Call the filler to draw this chunk of the line  */
                lclblitRec->blitDmap->prFill(lclblitRec);
            }while( lineState.lsMajorAxisLengthM1 >= 0 );
        }

        theGrafPort.pnDashNdx = dashIndex;
        thePort->pnDashNdx = dashIndex;
        theGrafPort.pnDashCnt = (INT16) dashCount >> 7;
        thePort->pnDashCnt = theGrafPort.pnDashCnt;
        theGrafPort.pnFlags   = dashFlags;
        thePort->pnFlags   = dashFlags;

    } /* else */

}

/***************************************************************************
* FUNCTION
*
*    DoubleDash
*
* DESCRIPTION
*
*    The function DoubleDash is used for Double Dash line drawing.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID DoubleDash(VOID)
{
    rect *rlistPtr;
    INT32 saveBlitCnt;
    INT32 saveDashIndex;
    SIGNED saveDashCount;
    INT32 saveDashFlags;
    INT32 lRectCnt;

    lRectCnt = rectCnt;

    /* Remember where the blitList area  */ 
    blitListPtr   = lclblitRec->blitList;

    /* store dash-filtered rects  */
    secondListPtr = blitListPtr + (MAX_RECTS/2)*sizeof(rect);
    
    /* can only use half area for the blit list, the other half is for pre-filtered rects */
    lineState.lsMaxRects = MAX_RECTS/2;

    /* Main double-dash line-drawing loop */
    while( lRectCnt-- > 0 )
    {
        /* Set up the line state for  this line  */
        lineState.lsSkipStat = (signed char) vListPtr->skipStat;
        rlistPtr = (rect *) ((SIGNED) vListPtr++);

        if( LSPC_rsLineSpecificSetup(rlistPtr) )
        {
            continue;
        }

        do
        {
            lclblitRec->blitList = blitListPtr;
            LSPC_rsLineScanOutToRectList();
        
            /* go back to where we put dash-filtered rects  */
            lclblitRec->blitList = secondListPtr;
            saveBlitCnt = lclblitRec->blitCnt;
            saveDashIndex = dashIndex;
            saveDashCount = dashCount;
            saveDashFlags = dashFlags;
            rlistPtr = (rect *) blitListPtr;
            ProcessDashes(rlistPtr);
        
            /* Call the filler to draw the foreground dashes in this 
               chunk of the line  */
            lclblitRec->blitDmap->prFill(lclblitRec);

            lclblitRec->blitCnt = saveBlitCnt;

            dashIndex = saveDashIndex;
            dashCount = saveDashCount;
            dashFlags = saveDashFlags;

            lclblitRec->blitPat = workingBkPat;
            dashFlags ^= pnDashState;
            rlistPtr = (rect *) blitListPtr;
            ProcessDashes(rlistPtr);
        
            /* Call the filler to draw the background dashes in this
               chunk of the line  */
            lclblitRec->blitDmap->prFill(lclblitRec);

            /* Flip the pen back to normal  */
            dashFlags ^= pnDashState;

            /* Restore foreground  */
            lclblitRec->blitPat = workingPnPat; 

        }while( lineState.lsMajorAxisLengthM1 >= 0 );
    }

}

/***************************************************************************
* FUNCTION
*
*    HandleWideSq
*
* DESCRIPTION
*
*    The function HandleWideSq is used for wide dashed lines. Assumes square pen.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID HandleWideSq(VOID)
{
    INT32 saveBlitCnt;
    INT32 saveDashIndex;
    SIGNED saveDashCount;
    INT32 saveDashFlags;
    rect *rlistPtr;
    INT32 lRectCnt;

    lRectCnt = rectCnt;

    lclblitRec->blitList = lclblitRec->blitList + (MAX_RECTS/2)*sizeof(rect);

    /* use half of buffer size since using other half for POLYGONS_rsScanAndDrawConvex */
    lineState.lsMaxRects = MAX_RECTS /2;

    /* Setup pen half width */
    /* Round odd up and down  */
    halfHeightUp   = theGrafPort.pnSize.Y >> 1; 
    halfHeightDown = (theGrafPort.pnSize.Y + 1) >> 1;

    /* Round odd left and right  */
    halfWidthLeft  = theGrafPort.pnSize.X >> 1; 
    halfWidthRight = (theGrafPort.pnSize.X + 1) >> 1;

    lclCapStyle = (UINT8) theGrafPort.pnCap;

    /* on-off or double-dash?  */
    if( dashFlags & pnDashStyle )
    {
        /* Double-dash style square pen wide line  */

        /* cap non-end points flat so they fit together  */
        dashCap = capFlat;

        /* Main square pen double-dash line drawing loop  */
        while( lRectCnt-- > 0 )
        {
            /* Set up the line state for this line  */

            /* Skipping is ignored for square pen wide lines  */    
            lineState.lsSkipStat = NoSkip;
            rlistPtr = (rect *) ((SIGNED) vListPtr++);

            if( LSPC_rsLineSpecificSetup(rlistPtr) )
            {
                continue;
            }
            dashPoly1.numPoints = 0;
            dashPoly2.numPoints = 0;

            /* mark the first burst for this line  */
            firstTime = 1;
            do
            {
                LSPC_rsLineScanOutToRectList();
                saveBlitCnt   = lclblitRec->blitCnt;
                saveDashIndex = dashIndex;
                saveDashCount = dashCount;
                saveDashFlags = dashFlags;
                rlistPtr      = (rect *) lclblitRec->blitList;
                ProcessDashesSq(rlistPtr, &dashPoly1);

                lclblitRec->blitCnt = saveBlitCnt;
                dashIndex           = saveDashIndex;
                dashCount           = saveDashCount;
                dashFlags           = saveDashFlags ^ pnDashState;

                lclblitRec->blitPat = workingBkPat;
                rlistPtr            = (rect *) lclblitRec->blitList;
                ProcessDashesSq(rlistPtr, &dashPoly2);
                
                dashFlags ^= pnDashState;
                lclblitRec->blitPat = workingPnPat;
                firstTime           = 0;
            }while( lineState.lsMajorAxisLengthM1 >= 0 );
        }
    }
    else
    {
        /* On-off style line. */

        /* Dashes are capped with the same style as the line  */
        dashCap = lclCapStyle;

        /* Main on-off line-drawing loop.  */
        while( lRectCnt-- > 0 )
        {
            /* Set up line state for this line  */
            lineState.lsSkipStat = NoSkip;
            rlistPtr = (rect *) ((SIGNED) vListPtr++);

            if( LSPC_rsLineSpecificSetup(rlistPtr) )
            {
                continue;
            }

            firstTime = 1;
            dashPoly1.numPoints = 0;
            do
            {
                LSPC_rsLineScanOutToRectList();
                rlistPtr = (rect *) lclblitRec->blitList;
                ProcessDashesSq(rlistPtr, &dashPoly1);
                firstTime = 0;
            }while( lineState.lsMajorAxisLengthM1 >= 0 );
        }
    }

}   

/***************************************************************************
* FUNCTION
*
*    ProcessDashes
*
* DESCRIPTION
*
*    The function ProcessDashes is used to process rects according to the current
*    dash state and the dash list, keeping only "on" rects and approximating a 
*    correction for angle.
* 
*    Note: To process properly the last point, there must be
*    an extra point after that.  At the least, there must be *something*
*    legally addressable for 8 bytes after the last rect in the source list,
*    so that this routine has something to read to determine the final move
*    type. Dash count *must* be greater than zero on entry, and is always 
*    greater than zero on exit.
*
* INPUTS
*
*    rect *rlistPtr - Pointer to the rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID ProcessDashes(rect *rlistPtr)
{
    INT16 JumpPDCheckMoreBurst = NU_FALSE;

    rect *temp1Rec,*temp2Rec;
    SIGNED count;
    INT32 temcount,tempBlitCount;
    INT16 temdashIndex;
    UINT8 *temstart;

    count = dashCount;
    tempBlitCount = lclblitRec->blitCnt;

    temcount = tempBlitCount;

    if( tempBlitCount != 0 )
    {
        /* load the destination rects  */
        temp1Rec = (rect *) lclblitRec->blitList;

        do
        {
            if( dashFlags & pnDashState )
            {
                /* Dash is on; keep rects until either come to end or out of rects  */
                do
                {
                    temp2Rec = (rect *) (rlistPtr);
                    *temp1Rec++ = *rlistPtr++;
                    if( (temp2Rec->Xmin == rlistPtr->Xmin) ||
                        (temp2Rec->Ymin == rlistPtr->Ymin) )
                    {
                        count -= 128;
                    }
                    else
                    {
                        count -= 181;
                    }

                    temcount--;
                    if( temcount == 0 )
                    {
                        if( count > 0 )
                        {
                            JumpPDCheckMoreBurst = NU_TRUE;
                            break; 
                        }
                        else
                        {
                            break;
                        }
                    }
                } while( count > 0 );
            }
            else
            {
                /* Dash is off; skip rects until either come to end or out of rects  */
                do
                {
                    tempBlitCount--;
                    temp2Rec = (rect *) (rlistPtr);
                    rlistPtr++;
                    if( (temp2Rec->Xmin == rlistPtr->Xmin) ||
                        (temp2Rec->Ymin == rlistPtr->Ymin) )
                    {
                        count -= 128;
                    }
                    else
                    {
                        count -= 181;
                    }

                    temcount--;
                    if( temcount == 0 )
                    {
                        if( count > 0 )
                        {
                            JumpPDCheckMoreBurst = NU_TRUE;
                            break;  
                        }
                        else
                        {
                            break;
                        }
                    }
                } while ( count > 0 );
            }

            /* flip the state and advance the dash pointer  */
            if( !JumpPDCheckMoreBurst )
            do
            {
                dashFlags ^= pnDashState;  
                dashIndex++;
                if( dashIndex >= dashElements)
                {
                    dashIndex = 0;
                }

                temstart = (dashListStart + dashIndex);  
                temdashIndex = *temstart;
                count += temdashIndex << 7;
            } while( count <= 0 );

            JumpPDCheckMoreBurst = NU_FALSE;

        } while( temcount > 0 );

        dashCount = count;

        /* tempBlitCnt = 0 there is no line; = 127 no dash  */
        lclblitRec->blitCnt = tempBlitCount;

    } /* if( ) */

}

/***************************************************************************
* FUNCTION
*
*    ProcessDashesSq
*
* DESCRIPTION
*
*    The function HandleWideSq is used to process rects into square pen wide
*    line dashes according to the current dash state and the dash list,
*    keeping only "on" rects and approximating a correction for angle.
* 
*    Note: To process properly the last point, there must be
*    an extra point after that.  At the least, there must be *something*
*    legally addressable for 8 bytes after the last rect in the source list,
*    so that this routine has something to read to determine the final move
*    type. Dash count *must* be greater than zero on entry, and is always 
*    greater than zero on exit.
*
* INPUTS
*
*    rect *rlistPtr        - Pointer to the rectangle.
*
*    dashPoly *dashPolyPtr - Pointer to the dashPoly structure.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID ProcessDashesSq(rect *rlistPtr, dashPoly *dashPolyPtr)
{
    signed char   Done                       = NU_FALSE;
    signed char   JumpPDSqOff                = NU_FALSE;
    signed char   JumpPDSqHandleLast         = NU_FALSE;
    signed char   JumpPDSqDrawDashAndAdvance = NU_FALSE;
    signed char   JumpPDSqCheckAdvance       = NU_FALSE;
    signed char   JumpPDSqCheckDashAdvance   = NU_FALSE;
    signed char   JumpPDSqDashAdvance        = NU_FALSE;
    INT32  temBlitCnt;
    INT16  tem,temdashIndex;
    SIGNED temDashCnt;
    rect   *temp2Rec;
    UINT8 *temstart;
    SIGNED saveBlitList;

    temBlitCnt = lclblitRec->blitCnt;

    temDashCnt = dashCount;
    if( temBlitCnt == 0 )
    {
        /* No pixel to do  */
        Done = NU_TRUE; 
    }

    if( !Done && (firstTime) && (dashFlags & pnDashState) )
    {
        /* capping the start of the dash; add the cap points to the dash  */
        DoCapPoints(rlistPtr, dashPolyPtr, lclCapStyle, CAP_DASH_START);
    }

    if( !Done )
    {
        do /* main do loop */
        {
            if( (dashFlags & pnDashState) == 0 )
            {
                JumpPDSqOff = NU_TRUE;
            }

            if( !JumpPDSqOff )
            {
                do
                {
                    temp2Rec = (rect *) (rlistPtr);
                    rlistPtr++;
                    if( (temp2Rec->Xmin == rlistPtr->Xmin) ||
                        (temp2Rec->Ymin == rlistPtr->Ymin) )
                    {
                        temDashCnt -= 128;
                    }
                    else
                    {
                        temDashCnt -= 181;
                    }

                    if( --temBlitCnt == 0 )
                    {
                        JumpPDSqHandleLast = NU_TRUE;
                        break; /* do-while( temDashCnt > 0 ) */
                    }
                } while( temDashCnt > 0 );
            }

            if( !JumpPDSqOff && !JumpPDSqHandleLast )
            {
                JumpPDSqDrawDashAndAdvance = NU_TRUE;
            }

            JumpPDSqHandleLast = NU_FALSE;
            
            if( !JumpPDSqOff && !JumpPDSqDrawDashAndAdvance )
            {
                if( lineState.lsMajorAxisLengthM1 >= 0 )
                {
                    JumpPDSqCheckAdvance = NU_TRUE;
                }
            }

            if( !JumpPDSqOff && !JumpPDSqDrawDashAndAdvance && !JumpPDSqCheckAdvance )
            {
                /* end of the line so use capping and capping the end of the 
                dash; add the cap points to the dash's  */   
                DoCapPoints(rlistPtr, dashPolyPtr, lclCapStyle, CAP_DASH_END);

                saveBlitList = lclblitRec->blitList;
                tem = (MAX_RECTS/2) *(sizeof(rect)) + sizeof(blitRcd);

                POLYGONS_rsScanAndDrawConvex( (UINT8 *)lclblitRec, tem, dashPolyPtr->FirstVert,
                                            dashPolyPtr->numPoints, coordModeOrigin, 0, 0);
                lclblitRec->blitList = saveBlitList;
                JumpPDSqCheckDashAdvance = NU_TRUE;
            }

            JumpPDSqCheckAdvance = NU_FALSE;

            if( !JumpPDSqOff && !JumpPDSqDrawDashAndAdvance && !JumpPDSqCheckDashAdvance )
            {
                /* set the end points for this dash  */
                if( temDashCnt > 0 )
                {
                    dashCount = temDashCnt;
                    Done = NU_TRUE;
                    break; /* do( ) main loop */
                }
            }

            JumpPDSqDrawDashAndAdvance = NU_FALSE;

            if( !JumpPDSqOff && !JumpPDSqCheckDashAdvance )
            {
                /* Set the end points for this dash, not either end of line, so
                use dash capping  */
                DoCapPoints(rlistPtr, dashPolyPtr, dashCap, CAP_DASH_END);

                saveBlitList = lclblitRec->blitList;
                tem = (MAX_RECTS/2) *(sizeof(rect)) + sizeof(blitRcd);

                POLYGONS_rsScanAndDrawConvex( (UINT8*)lclblitRec, tem, dashPolyPtr->FirstVert,
                                            dashPolyPtr->numPoints, coordModeOrigin, 0, 0);
                lclblitRec->blitList = saveBlitList;
                dashPolyPtr->numPoints = 0;
                JumpPDSqDashAdvance = NU_TRUE;
            }

            JumpPDSqOff = NU_FALSE;

            if( !JumpPDSqCheckDashAdvance && !JumpPDSqDashAdvance )
            {
                /* Dash is off; skip rects until either come to end of dash or out or rects  */
                do
                {
                    temp2Rec = (rect *) (rlistPtr);
                    rlistPtr++;
                    if( (temp2Rec->Xmin == rlistPtr->Xmin) ||
                        (temp2Rec->Ymin == rlistPtr->Ymin) )
                    {
                        temDashCnt -= 128;
                    }
                    else
                    {
                        temDashCnt -= 181;
                    }

                    if( --temBlitCnt == 0 )
                    {
                        JumpPDSqCheckDashAdvance = NU_TRUE;
                        break; 
                    }
                } while( temDashCnt > 0 );
            }

            if( !JumpPDSqCheckDashAdvance && !JumpPDSqDashAdvance )
            {
                JumpPDSqDashAdvance = NU_TRUE;
            }

            JumpPDSqCheckDashAdvance = NU_FALSE;
            
            if( !JumpPDSqDashAdvance )
            {
                if( temDashCnt > 0 )
                {
                    dashCount = temDashCnt;
                    Done = NU_TRUE;
                    break; 
                }
            }

            JumpPDSqDashAdvance = NU_FALSE;
                  
            /* flip the state and advance the dash pointer  */
            do
            {
                dashFlags ^= pnDashState;
                dashIndex++;
                if( dashIndex >= dashElements )
                {
                    dashIndex = 0;
                }

                temstart = (dashListStart + dashIndex);  
                temdashIndex = *temstart;
                temDashCnt += temdashIndex << 7;
            } while( temDashCnt <= 0 );

            if( (dashFlags & pnDashState) != 0 )
            {
                DoCapPoints(rlistPtr, dashPolyPtr, dashCap, CAP_DASH_START);
            }

        } while( temBlitCnt != 0 );
    }
    if( !Done )
    {
        dashCount = temDashCnt;
    }

}

/***************************************************************************
* FUNCTION
*
*    DoCapPoints
*
* DESCRIPTION
*
*    The function DoCapPointst adds the two or three points for the specified
*    cap to the polygon for the dash.
*
* INPUTS
*
*    rect *rlistPtr        - Pointer to the rectangle.
*
*    dashPoly *dashPolyPtr - Pointer to the dashPoly structure.
*
*    INT32 capStl          - Cap style.
*
*    INT32 capNd           - Cap style.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID DoCapPoints(rect *rlistPtr, dashPoly *dashPolyPtr, INT32 capStl, INT32 capNd)
{
    INT32 temlineDir;
    INT8 lineFlipTable[8] = {5, 4, 7, 6, 1, 0, 3, 2};
    INT32 halfSize, pointNum;

    pointNum   = dashPolyPtr->numPoints;
    temlineDir = lineState.lsLineDir;   

    if( capNd != CAP_DASH_END )
    {
        temlineDir = lineFlipTable[temlineDir];
    }

    if( lineState.lsErrTermAdjUp == 0 )
    {
        switch(temlineDir)
        {
        case 0:
        case 1: /* CapDown */
            if( capStl )
            {
                halfSize = halfHeightDown;
            }
            else
            {
                halfSize = 0;
            }

            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfWidthRight;
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfSize;

            dashPolyPtr->FirstVert[pointNum+1].Y = dashPolyPtr->FirstVert[pointNum].Y;
            dashPolyPtr->FirstVert[pointNum+1].X = rlistPtr->Xmin - halfWidthLeft;  
            dashPolyPtr->numPoints += 2;
            break;
        case 2:
        case 6:  /* CapRight */
            if( capStl )
            {
                halfSize = halfWidthRight;
            }
            else
            {
                halfSize = 0;
            }

            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfHeightUp;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfSize;

            dashPolyPtr->FirstVert[pointNum+1].X = dashPolyPtr->FirstVert[pointNum].X;
            dashPolyPtr->FirstVert[pointNum+1].Y = rlistPtr->Ymin + halfHeightDown;
            dashPolyPtr->numPoints += 2;
            break;
        case 3:
        case 7:  /* CapLeft */
            if( capStl )
            {
                halfSize = halfWidthLeft;
            }
            else
            {
                halfSize = 0;
            }

            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfHeightDown;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfSize;

            dashPolyPtr->FirstVert[pointNum+1].X = dashPolyPtr->FirstVert[pointNum].X;
            dashPolyPtr->FirstVert[pointNum+1].Y = rlistPtr->Ymin - halfHeightUp;
            dashPolyPtr->numPoints += 2;
            break;
        case 4:
        case 5:  /* CapUp */
            if( capStl )
            {
                halfSize = halfHeightUp;
            }
            else
            {
                halfSize = 0;
            }

            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfWidthLeft;
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfSize;

            dashPolyPtr->FirstVert[pointNum+1].Y = dashPolyPtr->FirstVert[pointNum].Y;
            dashPolyPtr->FirstVert[pointNum+1].X = rlistPtr->Xmin + halfWidthRight;
            dashPolyPtr->numPoints += 2;
            break;
        }
    }
    else
    {
        switch(temlineDir)
        {
        case 0:
        case 3:  /* CapDownAndLeft */
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfWidthRight;
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfHeightDown;
            if( capStl )
            {
                dashPolyPtr->FirstVert[pointNum+1].Y = dashPolyPtr->FirstVert[pointNum].Y;
                pointNum++;
                dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfWidthLeft;
                dashPolyPtr->numPoints++;
            }
            pointNum++;

            /* There are two more points in the polygon, the first and the
               last point; the point middle was already counted if necessary  */
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfHeightUp;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfWidthLeft;
            dashPolyPtr->numPoints +=2;
            break;

        case 1:
        case 2:  /* CapDownAndRight */
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfHeightUp;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfWidthRight;
            if( capStl )
            {
                dashPolyPtr->FirstVert[pointNum+1].X = dashPolyPtr->FirstVert[pointNum].X;
                pointNum++;
                dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfHeightDown;
                dashPolyPtr->numPoints++;
            }
            pointNum++;

            /* There are two more points in the polygon, the first and the
               last point; the point middle was already counted if necessary  */
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfHeightDown;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfWidthLeft;
            dashPolyPtr->numPoints +=2;
            break;
        case 4:
        case 7:  /* CapUpAndLeft */
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfHeightDown;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfWidthLeft;
            if( capStl )
            {
                dashPolyPtr->FirstVert[pointNum+1].X = dashPolyPtr->FirstVert[pointNum].X;
                pointNum++;
                dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfHeightUp;
                dashPolyPtr->numPoints++;
            }
            pointNum++;

            /* There are two more points in the polygon, the first and the
               last point; the point middle was already counted if necessary  */
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfHeightUp;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfWidthRight;
            dashPolyPtr->numPoints +=2;
            break;
        case 5:
        case 6:  /* CapUpAndRight */
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin - halfWidthLeft;
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin - halfHeightUp;
            if( capStl )
            {
                dashPolyPtr->FirstVert[pointNum+1].Y = dashPolyPtr->FirstVert[pointNum].Y;
                pointNum++;
                dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfWidthRight;
                dashPolyPtr->numPoints++;
            }
            pointNum++;

            /* There are two more points in the polygon, the first and the
               last point; the point middle was already counted if necessary  */
            dashPolyPtr->FirstVert[pointNum].Y = rlistPtr->Ymin + halfHeightDown;
            dashPolyPtr->FirstVert[pointNum].X = rlistPtr->Xmin + halfWidthRight;
            dashPolyPtr->numPoints += 2;
            break;
        }
    }

}

#endif  /* DASHED_LINE_SUPPORT */

/***************************************************************************
* FUNCTION
*
*    SetUpCommon
*
* DESCRIPTION
*
*    The function SetUpCommon sets up a local copy of the passed-in blitRcd
*    are related value.
*
* INPUTS
*
*    blitRcd *LINEREC - Pointer to the blitRcd.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID SetUpCommon(blitRcd *LINEREC)
{
    /* Reset the rectangle counter  */
    rectCnt = LINEREC->blitCnt;

    /* Copy the pointer to the line list */
    vListPtr = (blitVect *) LINEREC->blitList;

    /* Copy blitRcd to working space  */
     lclblitRec = (blitRcd *) mpWorkSpace;
    *lclblitRec = *LINEREC;

    /* point to the start of the blitList, we'll fill out with the rects that 
       make up the line, which is in the stack segment along with the blitRcd  */
    lclblitRec->blitList = (SIGNED ) (mpWorkSpace + sizeof(blitRcd));
    
}
