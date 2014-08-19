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
*  rectd.c                                                      
*
* DESCRIPTION
*
*  Contains the API function RS_Rectangle_Draw and support functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Rectangle_Draw
*  AddOuterEdges
*  AddInnerEdges
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  edges.h
*  rectd.h
*  globalrsv.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/edges.h"
#include "ui/rectd.h"
#include "ui/globalrsv.h"
#include "ui/gfxstack.h"

VOID (*lineExecPntr)();

/* round rectangle diameters */
static INT32 argDiaX;              
static INT32 argDiaY;

static INT8  vertDirection;

/* Functions with local scope. */
static VOID AddInnerEdges(qarcState *newEdge);

/***************************************************************************
* FUNCTION
*
*    RS_Rectangle_Draw
*
* DESCRIPTION
*
*    The function RS_Rectangle_Draw is used to draw rectangles.
*
* INPUTS
*
*    ObjectAction action - This would be the action that would be performed on the object,
*                          this will be a list of actions in an enumerated data type.
*
*                              EX: FRAME  = 0
*                                  PAINT  = 1
*                                  FILL   = 2
*                                  ERASE  = 3
*                                  INVERT = 4
*                                  POLY   = 5  (only used by BEZIER) 
*
*    rect *argRect       - Pointer to rect structure for the rectangle.
*
*    INT32 patt          - This is the Pattern, fill pattern structure that contains 32 default
*                          values for patterns. So the value is 0 to 31. -1 if not used.
*                          This can be user Defined
*
*    INT32 DiaX         - Used for rounded rectangles: x diameter.
*                          For no rounding, set to 0.
*
*    INT32 DiaY)        - Used for rounded rectangles: y diameter.
*                          For no rounding, set to 0.
*
* OUTPUTS
*
*    STATUS             -  Returns NU_SUCCESS if successful.
*
***************************************************************************/
STATUS RS_Rectangle_Draw( ObjectAction action, rect *argRect, INT32 patt, INT32 DiaX, INT32 DiaY)
{
    STATUS    status = ~NU_SUCCESS;
    
    /* base rectangle */
    rect      rXminBase;       
    
    /* offsets to edges of square pen */
    UINT32    halfWidth;       
    UINT32    halfHeight;
    UINT32    i;
    
    /* pointer to new edge to add to table */
    qarcState *newEdge = 0;    

    rect tempRect;

    struct _lineRcd
    {
        rect LR;
        UINT16 skipStat;
        INT16  pad;
    } lineRcd[4];

    rect elseRect[4];

   /* offsets to edges of square pen */
    UINT32 halfWidthLeft;
    UINT32 halfWidthRight;
    UINT32 halfHeightUp;
    UINT32 halfHeightDown;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    if( theGrafPort.pnLevel < 0 )
    {
        /* nothing to do */
        status = NU_SUCCESS; 
    }
    /* Normal Rectangle */
    else if( DiaX==0 && DiaY==0 ) 
    {
        /* get rectangle */
        tempRect = * argRect;
        if (globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(tempRect, &tempRect, 1);
        }

        if( action == FRAME )
        {
            /* what kind of optimization can we do (as vectors or fills)? */
            if( (theGrafPort.pnFlags & pnDashFlg) || (!(theGrafPort.pnFlags & pnSizeFlg)) )
            {
                /* do vectors since only vectors know how to dash; also fills
                   are faster only if wide pen */

                /* setup blitlist and call MultiVect for 4 line segments; use
                   skiplast drawing clockwise to avoid double draw of end points */

                /* top */
                lineRcd[0].LR.Xmin  = tempRect.Xmin;
                lineRcd[0].LR.Ymin  = tempRect.Ymin;
                lineRcd[0].LR.Xmax  = tempRect.Xmax;
                lineRcd[0].LR.Ymax  = tempRect.Ymin;
                lineRcd[0].skipStat = SkipLast;
                lineRcd[0].pad      = 0;

                /* bottom */
                lineRcd[1].LR.Xmin  = tempRect.Xmax;
                lineRcd[1].LR.Ymin  = tempRect.Ymax;
                lineRcd[1].LR.Xmax  = tempRect.Xmin;
                lineRcd[1].LR.Ymax  = tempRect.Ymax;
                lineRcd[1].skipStat = SkipLast;
                lineRcd[1].pad      = 0;

                /* left */
                lineRcd[2].LR.Xmin  = tempRect.Xmin;
                lineRcd[2].LR.Ymin  = tempRect.Ymax;
                lineRcd[2].LR.Xmax  = tempRect.Xmin;
                lineRcd[2].LR.Ymax  = tempRect.Ymin;
                lineRcd[2].skipStat = SkipLast;
                lineRcd[2].pad      = 0;

                /* right */
                lineRcd[3].LR.Xmin  = tempRect.Xmax;
                lineRcd[3].LR.Ymin  = tempRect.Ymin;
                lineRcd[3].LR.Xmax  = tempRect.Xmax;
                lineRcd[3].LR.Ymax  = tempRect.Ymax;
                lineRcd[3].skipStat = SkipLast;
                lineRcd[3].pad      = 0;

                /* set pointer to our blitlist */
                grafBlit.blitList = (SIGNED) &lineRcd[0];
                grafBlit.blitCnt  = 4;

                /* draw rectangle */
                lineExecPntr = (VOID (*)()) lineExecIDV;
                lineExecPntr(&grafBlit);
            }
            else
            {
                /* do fills */
                /* setup blitlist and call filler for 4 rects */
                /* Determine offsets to edges of square pen */

                /* width/2 to left (round odd down) */
                halfWidthLeft  = ( theGrafPort.pnSize.X      >> 1);

                /* width/2 to right (round odd up) */
                halfWidthRight = ((theGrafPort.pnSize.X + 1) >> 1);

                /* height/2 up (round odd down) */
                halfHeightUp   = ( theGrafPort.pnSize.Y      >> 1);

                /* height/2 down (round odd up) */
                halfHeightDown = ((theGrafPort.pnSize.Y + 1) >> 1);

                /* top */
                elseRect[0].Xmin = tempRect.Xmin - halfWidthLeft;
                elseRect[0].Ymin = tempRect.Ymin - halfHeightUp;
                elseRect[0].Xmax = tempRect.Xmax + halfWidthRight;
                elseRect[0].Ymax = tempRect.Ymin + halfHeightDown;

                /* bottom */
                elseRect[1].Xmin = tempRect.Xmin - halfWidthLeft;
                elseRect[1].Ymin = tempRect.Ymax - halfHeightUp;
                elseRect[1].Xmax = tempRect.Xmax + halfWidthRight;
                elseRect[1].Ymax = tempRect.Ymax + halfHeightDown;

                /* left */
                elseRect[2].Xmin = tempRect.Xmin - halfWidthLeft;
                elseRect[2].Ymin = tempRect.Ymin + halfHeightDown;
                elseRect[2].Xmax = tempRect.Xmin + halfWidthRight;
                elseRect[2].Ymax = tempRect.Ymax - halfHeightUp;

                /* right */
                elseRect[3].Xmin = tempRect.Xmax - halfWidthLeft;
                elseRect[3].Ymin = tempRect.Ymin + halfHeightDown;
                elseRect[3].Xmax = tempRect.Xmax + halfWidthRight;
                elseRect[3].Ymax = tempRect.Ymax - halfHeightUp;

                /* set pointer to our blitlist */
                grafBlit.blitList = (SIGNED) & elseRect[0];
                grafBlit.blitCnt  = 4;

                /* draw filled rectangle */
                grafBlit.blitDmap->prFill(&grafBlit);
            }

            /* restore standard blit list pointer */
            grafBlit.blitList = (SIGNED) &grafBlist;
            grafBlit.blitCnt = 1;
            status = NU_SUCCESS;
        }
        else
        {
			if( patt > 32 )
            {
                /* reset to default pattern since only 32 patterns exist */
                patt = 1;
            }

            /* nothing to do */
            if( theGrafPort.pnLevel < 0 )
            {
                status = NU_SUCCESS;
            }

            if( status != NU_SUCCESS )
            {
                /* put rectangle in default blit list */
                grafBlist.Xmin = tempRect.Xmin;
                grafBlist.Ymin = tempRect.Ymin;
                grafBlist.Xmax = tempRect.Xmax;
                grafBlist.Ymax = tempRect.Ymax;

                switch(action)
                {
                    case ERASE:
                        /* set up blit record to erase */
                        grafBlit.blitRop = zREPz;
                        grafBlit.blitPat = theGrafPort.bkPat;
                        break;

                    case FILL:
                        /* set up blit record so that it is a fill action. */
                        grafBlit.blitRop = zREPz;
                        grafBlit.blitPat = patt;
                        break;

                    case INVERT:
                        /* set up blit record for inverting */
                        grafBlit.blitRop = zINVERTz;
                        break;
					case TRANS: /* action = 6*/
						grafBlit.blitRop = xAVGx;
					    grafBlit.blitCnt  = 4;            
						break;
                    default:
                        break;
                }
            }

            /* draw filled rectangle */
            grafBlit.blitDmap->prFill(&grafBlit);

            /* restore default blit record */
            if( (action == ERASE) || (action == INVERT) )
            {
                /* Set the raster op back to the pen mode */
                grafBlit.blitRop = theGrafPort.pnMode;
            }

            if( action == ERASE )
            {
                /* reset the blit pattern so that it will not be erasing */ 
                grafBlit.blitPat = theGrafPort.pnPat;
            }

            status = NU_SUCCESS;
        }
    } /* else if( DiaX==0 && DiaY==0 ) Normal Rectangle */
    else /* Round Rectangle */
    {
        switch(action)
        {
        case FILL:
            /* make replace rop and the pattern the defaults temporarily, for PaintRect */
            grafBlit.blitRop = zREPz;
            grafBlit.blitPat = patt;
            break;

        case ERASE:
            /* set up default blit record */
            grafBlit.blitRop = zREPz;
            grafBlit.blitPat = theGrafPort.bkPat;
            break;

        case INVERT:
            /* make raster op invert */
            grafBlit.blitRop = zINVERTz;
			break;
		case TRANS: /* action = 6*/
			grafBlit.blitRop = xAVGx;
			break;
		default:
			break;
        }

        /* Paint filled rectangle */
        /* Do for above and PAINT and FRAME */

        /* get passed parameters */
        argDiaX = DiaX;
        argDiaY = DiaY;

        /* GET is initially empty */
        for( i = 0; i < 12; i++)
        {
            qaEdgeBuffer[i].NextEdge = 0;
        }

        /* check if virtual mode */
        if( theGrafPort.portFlags & pfVirtual )
        {
            /* Virtual To Global size */
            V2GSIZE(argDiaX, argDiaY, (INT32 *)&argDiaX, (INT32 *)&argDiaY);
        }

        /* get rectangle */
        rXmin = *argRect;

        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(rXmin, &rXmin, 0);
        }

        /* if( PAINT or FRAME - thin ) */

        /* get width and height */
        rXWidth = rXmin.Xmax - rXmin.Xmin;
        rXHeight = rXmin.Ymax - rXmin.Ymin;

        switch(action)
        {
        case FILL:
        case ERASE:
        case INVERT:
        case PAINT:

            /* For all actions - except FRAME */
 
            /* Continue PAINT */
        
            /* bad rectangle ? */
            if( (rXWidth < 0) || (rXHeight < 0) )
            {
                break;
            }

            /* if either corner diameter is greater than the corresponding rectangle
               dimension, limit it to the rectangle dimension */

            if( argDiaX < rXWidth )
            {
                /* use diameter */
                rXWidth = argDiaX; 
            }
            if( argDiaY < rXHeight )
            {
                /* use diameter */
                rXHeight = argDiaY; 
            }

            xRadius = (rXWidth >> 1);
            yRadius = (rXHeight >> 1);

            /* left edge   + height */
            thisLeftEdge   = rXmin.Xmin + xRadius;     

            /* top edge    + height */
            nextTopEdge    = rXmin.Ymin + yRadius;     

            /* right edge  + height */
            thisRightEdge  = rXmin.Xmax - xRadius;     

            /* bottom edge - height -1 for filling */
            nextBottomEdge = rXmin.Ymax - yRadius - 1; 

            /* set up the upper left arc */
            GETPtr = &qaEdgeBuffer[0];
            newEdge = GETPtr;
            EDGES_rsSetupQuadrantArc(newEdge, xRadius, yRadius, thisLeftEdge, nextTopEdge, -1, -1);

            /* set up the upper right arc */
            newEdge = &qaEdgeBuffer[1];
            EDGES_rsSetupQuadrantArc(newEdge, xRadius, yRadius, thisRightEdge, nextTopEdge, 1, 1);

            /* insert the new edge in GET, YX sorted */
            AddToGETYXSorted(newEdge);

            /* now add the bottom arcs, with the top points trimmed off to avoid
               overlap problems */

            /* are arcs only one scan line high? */
            if( qaEdgeBuffer[0].Count != 1 )
            {
                /* no */
                newEdge = &qaEdgeBuffer[2];
                EDGES_rsSetupBottomQuadrantArc(newEdge, xRadius, yRadius, thisLeftEdge, nextBottomEdge, 1, -1);

                /* advance Y by 1 (skip first point) */
                newEdge->StepVector(newEdge); 

                /* start on the next scan line */
                newEdge->StartY++;            

                /* skip the topmost point */
                newEdge->Count--;             

                /* insert the new edge in GET, YX sorted */
                AddToGETYXSorted(newEdge);

                /* advance bottom arcs by one scan line to avoid overlap problems (skip
                   the top scan line, which can stick out too far and always overlaps with
                   the top quadrant arcs anyway) */

                /* set up the lower right arc */
                newEdge = &qaEdgeBuffer[3];
                EDGES_rsSetupBottomQuadrantArc(newEdge, xRadius, yRadius, thisRightEdge, nextBottomEdge, -1, 1);

                /* advance Y by 1 (skip first point) */
                newEdge->StepVector(newEdge); 

                /* start on the next scan line */
                newEdge->StartY++;            

                /* skip the topmost point */
                newEdge->Count--;             

                /* insert the new edge in GET, YX sorted */
                AddToGETYXSorted(newEdge);
            }   

            /* now add the vertical outer sides, if applicable */
            /* start on the scan line after the top arc ends */
            nextTopEdge = rXmin.Ymin + yRadius + 1; 

            /* top of bottom arc */
            nextBottomEdge = rXmin.Ymax - yRadius;  
    
            if( nextBottomEdge > nextTopEdge )
            {
                /* add the vertical edges */
                /* distance between top & bottom, not inclusive */
                nextBottomEdge -= nextTopEdge;  
                newEdge = &qaEdgeBuffer[4];
                EDGES_rsSetupVerticalLineEdge( (Vedge *)newEdge, rXmin.Xmin, nextTopEdge, nextBottomEdge, -1);
  
                /* insert the new edge in GET, YX sorted */
                AddToGETYXSorted(newEdge);

                /* reverse direction for other side */
                vertDirection = -vertDirection; 
                newEdge = &qaEdgeBuffer[5];
                EDGES_rsSetupVerticalLineEdge( (Vedge *)newEdge, rXmin.Xmax, nextTopEdge, nextBottomEdge, 1);

                /* insert the new edge in GET, YX sorted */
                AddToGETYXSorted(newEdge);
            }        
            status = NU_SUCCESS;
            break;

        case FRAME:
            /* FRAME - Wide */

            /* bad rectangle ? */
            if( (rXWidth < 0) || (rXHeight < 0) )
            {
                break;
            }

            /* thin or wide line? */
            if( !(theGrafPort.pnFlags & pnSizeFlg) )
            {
                /* set up the outer edges first */
                AddOuterEdges(newEdge);

                /* width */
                rXWidth  = rXmin.Xmax - rXmin.Xmin; 

                /* height */
                rXHeight = rXmin.Ymax - rXmin.Ymin; 
                if( (rXWidth > 0) && (rXHeight > 0) )
                {
                    /* need inner edges too */
                    AddInnerEdges(newEdge); 
                }
            }
            else
            {
                /* wide line */
                /* Draws a wide-line framed rounded rectangle. The approach is the same
                   as  for thin framed rounded rects, except that the outer edges are moved
                   out by the X and Y pen radii, and the inner arcs are moved in similarly. */
                rXminBase = rXmin;

                /* For odd pen dimensions, we can be symmetric about the thin edge. For
                   even, we bias the extra pixel to the outside of the frame. */

                /* set up the four outer arcs first and rounded up */
                halfWidth = (theGrafPort.pnSize.X >> 1);
                rXmin.Xmin -= halfWidth;
                rXmin.Xmax += halfWidth;
                halfHeight = (theGrafPort.pnSize.Y >> 1);
                rXmin.Ymin -= halfHeight;
                rXmin.Ymax += halfHeight;

                /* get width and height */
                rXWidth = rXmin.Xmax - rXmin.Xmin;
                rXHeight = rXmin.Ymax - rXmin.Ymin;
                
                /* bad rectangle ? */
                if( (rXWidth < 0) || (rXHeight < 0) )
                {
                    /* status is !NU_SUCCESS (as preset) */
                    break; 
                }

                /* set up the outer edges first */
                AddOuterEdges(newEdge); 

                /*Inner arcs are pulled in from frame edge by pen radii. For odd
                  pen dimensions, we can be symmetric between inner and outer edge
                  displacements.  For even pen dimensions, we bias the extra pixel
                  to the outside. */
                halfWidth  = ((theGrafPort.pnSize.X - 1) >> 1);
                rXmin.Xmin = rXminBase.Xmin + halfWidth;
                rXmin.Xmax = rXminBase.Xmax - halfWidth;
                halfHeight = ((theGrafPort.pnSize.Y - 1) >> 1);
                rXmin.Ymin = rXminBase.Ymin + halfHeight;
                rXmin.Ymax = rXminBase.Ymax - halfHeight;

                /* width */
                rXWidth  = rXmin.Xmax - rXmin.Xmin; 

                /* height */
                rXHeight = rXmin.Ymax - rXmin.Ymin; 
                if( (rXWidth >= 0) && (rXHeight > 0) )
                {
                    /* need inner edges too */
                    AddInnerEdges(newEdge); 
                }
            } /* Wide Frame */

            /* action == FRAME */
            status = NU_SUCCESS;
            break;
            
        /* Action not applicable. Not implemented. */	
        case POLY:     
        case TRANS:     
        break;        

        } /* switch(action) */

        if( status == NU_SUCCESS )
        {
            /* draw the oval and done! */
            EDGES_rsScansAndFillsEdgeList( (VOID **)&GETPtr, (blitRcd *)&rFillRcd, fillRcdSize, cmplx, 1, 0);

            /* restore default blit record */
            grafBlit.blitRop = theGrafPort.pnMode;
            grafBlit.blitPat = theGrafPort.pnPat;
        }

    } /* Round Rectangle */ 

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/***************************************************************************
* FUNCTION
*
*    AddOuterEdges
*
* DESCRIPTION
*
*    Function AddOuterEdges builds a GET containing a set of 4 to 6 outer
*    edges (vertical edges are only added if they're needed).
*
*    Global input is dimensions of bounding rectangle:
*        rXmin.Xmin = right edge
*        rXmin.Ymin = top edge
*        rXWidth    = width
*        rXHeight   = height
*
* INPUTS
*
*    qarcState *newEdge - Pointer to qarcState structure for the edges.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID AddOuterEdges(qarcState *newEdge)
{
    INT16 Done = NU_FALSE;

    /* if either corner diameter is greater than the corresponding rectangle
       dimension, limit it to the rectangle dimension */

    if( argDiaX < rXWidth)
    {
        /* use diameter */
        rXWidth  = argDiaX; 
    }
    if( argDiaY < rXHeight)
    {
        /* use diameter */
        rXHeight = argDiaY; 
    }

    xRadius = ( rXWidth  >> 1);
    yRadius = ( rXHeight >> 1);

    /* left edge + height */
    thisLeftEdge   = rXmin.Xmin + xRadius;     

    /* top edge + height */
    nextTopEdge    = rXmin.Ymin + yRadius;     

    /* shift right by 1 to compensate for the filler not drawing right edges */
    thisRightEdge  = rXmin.Xmax - xRadius + 1; 

    /* bottom edge - height */
    nextBottomEdge = rXmin.Ymax - yRadius;     

    /* set up the upper left arc */
    GETPtr  = &qaEdgeBuffer[0];
    newEdge = GETPtr;
    EDGES_rsSetupQuadrantArc(newEdge, xRadius, yRadius, thisLeftEdge, nextTopEdge, -1, -1);

    /* set up the upper right arc */
    newEdge = &qaEdgeBuffer[1];
    EDGES_rsSetupQuadrantArc(newEdge, xRadius, yRadius, thisRightEdge, nextTopEdge, 1, 1);

    /* insert the new edge in GET, YX sorted */
    AddToGETYXSorted(newEdge);

    /* set up the lower left arc */
    newEdge = &qaEdgeBuffer[2];
    EDGES_rsSetupBottomQuadrantArc(newEdge, xRadius, yRadius, thisLeftEdge, nextBottomEdge, 1, -1);

    /* insert the new edge in GET, YX sorted */
    AddToGETYXSorted(newEdge);

    /* set up the upper right arc */
    newEdge = &qaEdgeBuffer[3];
    EDGES_rsSetupBottomQuadrantArc(newEdge, xRadius, yRadius, thisRightEdge, nextBottomEdge, -1, 1);

    /* insert the new edge in GET, YX sorted */
    AddToGETYXSorted(newEdge);

    /* now add the vertical outer sides, if applicable */

    /* assume bottom and top arcs don't  overlap, so
       vertical edges can run in same direction as arcs */
    vertDirection = -1; 

    /* start on the scan line after the top arc ends */
    nextTopEdge = rXmin.Ymin + yRadius + 1; 

    /* top of bottom arc */
    nextBottomEdge = rXmin.Ymax - yRadius;  
    
    if( nextBottomEdge == nextTopEdge )
    {
        /* distance is 0; edges fit perfectly */
        Done = NU_TRUE;  
    }

    if( !Done )
    {
        if( nextBottomEdge < nextTopEdge )
        {
            /* edges overlap by 1; neutralize 1 so we get correct winding rule counts */
            /* point back to bottom scan of top arc; make the edges 1 long, starting
               at the scan line that ends the top edges (inclusive), to neutralize one
               of the arcs; the top & bottom arcs overlap by 1, and that gives one winding
               count too many */
            nextBottomEdge = nextTopEdge;
            nextTopEdge--;

            /* edges have to run opposite arcs in order to neutralize one of them */
            vertDirection = 1; 
        }

        /* add the vertical edges */
        nextBottomEdge -= nextTopEdge; /* distance between top & bottom, not inclusive */
        newEdge = &qaEdgeBuffer[8];
        EDGES_rsSetupVerticalLineEdge((Vedge *)newEdge, rXmin.Xmin, nextTopEdge, nextBottomEdge, vertDirection);

        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);

        /* reverse direction for other side */
        vertDirection = -vertDirection; 
        newEdge = &qaEdgeBuffer[11];
        EDGES_rsSetupVerticalLineEdge((Vedge *)newEdge, (rXmin.Xmax + 1), nextTopEdge, nextBottomEdge, vertDirection);

        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);

    } /* if( !Done ) */

}

/***************************************************************************
* FUNCTION
*
*    AddInnerEdges
*
* DESCRIPTION
*
*     Function AddInnerEdges adds a set of 4 to 6 inner edges to GET (vertical
*     edges are only added if they're needed).
*
*     Global input is dimensions of bounding rectangle:
*         rXmin.Xmin = right edge
*         rXmin.Ymin = top edge
*         rXWidth    = width
*         rXHeight   = height
*
* INPUTS
*
*    qarcState *newEdge - Pointer to qarcState structure for the edges.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID AddInnerEdges(qarcState *newEdge)
{
    INT16 Done = NU_FALSE;

   /* if either corner diameter is greater than the corresponding rectangle
      dimension, limit it to the rectangle dimension */
   if( argDiaX < rXWidth )
   {
       /* use diameter */
        rXWidth = argDiaX; 
   }
   if( argDiaY < rXHeight )
   {
       /* use diameter */
       rXHeight = argDiaY; 
   }

    xRadius = (rXWidth  >> 1);
    yRadius = (rXHeight >> 1);

    /* left edge + height */
    thisLeftEdge   = rXmin.Xmin + xRadius;     

    /* top edge + height */
    nextTopEdge    = rXmin.Ymin + yRadius;   

    /* shift right by 1 to compensate for the filler not drawing right edges */
    nextBottomEdge = rXmin.Ymax - yRadius;     

    /* bottom edge - height */
    thisRightEdge  = rXmin.Xmax - xRadius + 1; 

    /* set up the upper left arc */
    newEdge = &qaEdgeBuffer[4];
    EDGES_rsSetupQuadrantArc(newEdge, xRadius, yRadius, thisLeftEdge, nextTopEdge, -1, 1);

    /* set up the upper right arc */
    newEdge = &qaEdgeBuffer[5];
    EDGES_rsSetupQuadrantArc(newEdge, xRadius, yRadius, thisRightEdge, nextTopEdge, 1, -1);

    /* set up the lower left arc */
    newEdge = &qaEdgeBuffer[6];
    EDGES_rsSetupBottomQuadrantArc(newEdge, xRadius, yRadius, thisLeftEdge, nextBottomEdge, 1, 1);

    /* set up the upper right arc */
    newEdge = &qaEdgeBuffer[7];
    EDGES_rsSetupBottomQuadrantArc(newEdge, xRadius, yRadius, thisRightEdge, nextBottomEdge, -1, -1);

    /* Now shift the edges down and right 1, to compensate for filler
       characteristics and to make 1 wide be really 1 wide, and add to the GET. */

    newEdge = &qaEdgeBuffer[4];

    /* shift 1 to the right */
    newEdge->CurrentX++;    

    /* shift down 1 */
    newEdge->StartY++;      

    /* skip the bottommost point */
    newEdge->Count--;       

    /* check if more to do */
    if( newEdge->Count != 0 )
    {
        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);

        newEdge = &qaEdgeBuffer[5];
        /* shift 1 to the left */
        newEdge->CurrentX--; 
        newEdge->StartY++;

        /* shift down 1 */
        newEdge->Count--;   

        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);
    }

    newEdge = &qaEdgeBuffer[6];

    /* shift 1 to the right */
    newEdge->CurrentX++;           

    /* advance Y by 1 (skip first point) */
    newEdge->StepVector(newEdge);  
    newEdge->Count--;

    /* check if more to do */
    if( newEdge->Count != 0 )
    {
        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);

        newEdge = &qaEdgeBuffer[7];

        /* shift 1 to the left */
        newEdge->CurrentX--;         

         /* advance Y by 1 (skip first point) */
        newEdge->StepVector(newEdge);

        /* skip the topmost point */
        newEdge->Count--;             

        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);
    }

    /* now add the vertical outer sides, if applicable */

    /* assume bottom and top arcs don't  overlap, so
       vertical edges can run in same direction as arcs */
    vertDirection = 1;                      

    /* start on the scan line after the top arc ends */
    nextTopEdge = rXmin.Ymin + yRadius + 1; 

    /* top of bottom arc */
    nextBottomEdge = rXmin.Ymax - yRadius;  
    
    if( nextBottomEdge == nextTopEdge )
    {
        /* distance is 0; edges fit perfectly */
        Done = NU_TRUE; 
    }
    
    if( !Done )
    {
        if( nextBottomEdge < nextTopEdge )
        {
            /* edges overlap by 1; neutralize 1 so we get correct winding rule counts */
            /* point back to bottom scan of top arc; make the edges 1 long, starting
               at the scan line that ends the top edges (inclusive), to neutralize one
               of the arcs; the top & bottom arcs overlap by 1, and that gives one winding
               count too many */
            nextBottomEdge = nextTopEdge;
            nextTopEdge--;

            /* edges have to run opposite arcs in order to neutralize one of them */
            vertDirection = -1; 
        }

        /* add the vertical edges */
        /* distance between top & bottom, not inclusive */
        nextBottomEdge -= nextTopEdge;  
        newEdge = &qaEdgeBuffer[9];
        EDGES_rsSetupVerticalLineEdge((Vedge *)newEdge, (rXmin.Xmin + 1), nextTopEdge, nextBottomEdge, vertDirection);

        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);

        /* reverse direction for other side */
        vertDirection = -vertDirection; 
        newEdge = &qaEdgeBuffer[10];
        EDGES_rsSetupVerticalLineEdge((Vedge *)newEdge, rXmin.Xmax, nextTopEdge, nextBottomEdge, vertDirection);

        /* insert the new edge in GET, YX sorted */
        AddToGETYXSorted(newEdge);

    } /* if( !Done ) */

}
