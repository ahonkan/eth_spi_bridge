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
*  polygons.c                                                   
*
* DESCRIPTION
*
*  Contains polygon support functions and global variables.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  FillPolygon
*  BuildGET
*  AddEdgeToGET
*  POLYGONS_rsScanAndDrawConvex
*  ScanBurst
*  POLYGONS_rsScanPolygonEdge
*  POLYGONS_rsConvertEdges
*  POLYGONS_rsScanEdgeList
*  FindHighestEdge
*  AddEdgeToAET
*  POLYGONS_rsSuperSetPolyLineDrawer
*  BuildSquareWidePenPolyLineGET
*
* DEPENDENCIES
*
*  rs_base.h
*  globalrsv.h
*  polygons.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/globalrsv.h"
#include "ui/polygons.h"
#include "ui/gfxstack.h"

#if     (WORKSPACE_BUFFER_SIZE < STACK_BUFFER_SIZE) 

/* working storage space */
static  UINT8 buf1[STACK_BUFFER_SIZE];  

#endif  /* (WORKSPACE_BUFFER_SIZE < STACK_BUFFER_SIZE) */

/* pointer into buffer */
static  UINT8 *buf1Ptr;                 

/* buffer counter */
static  SIGNED bufCntr;                 

/* pointer to active edge table */
static  lineEdgeV *tmpAETPtr;       

/* pointer to byte after last byte available for edge struct storage */
static  SIGNED endAvailForGET;  

/* vector called when building the GET to add each edge to the GET */
static  INT16 (*addEdgeVector)(VOID);   

/* current, last line direction */
static  INT32 direction;        
/* 0 = 90 degrees
   1 = between 0 and 90 degrees
   2 = 0 degrees
   3 = between 270 and 0 degrees
   4 = 270 degrees
   5 = between 180 and 270 degrees
   6 = 180 degrees
   7 = between 90 and 180 degrees */
static  INT32 lastDirection;    

/* clip rect */
static  rect clpR;            

/* start and end points of the two edges formed around */
static  INT32 startInX;       

/* each line segment for wide lines */
static  INT32 startInY;       
static  INT32 startOutX;
static  INT32 startOutY;
static  INT32 endInX;
static  INT32 endInY;
static  INT32 endOutX;
static  INT32 endOutY;

/* end points of the two edges formed around the last*/
static  INT32 lastInX;        

/* line segment for wide lines */
static  INT32 lastInY;        
static  INT32 lastOutX;
static  INT32 lastOutY;

/* pointer to current vertex */
static  point *vertexListPtr;   

/* user coordinates of last vertex */
static  INT32 rawLastX;         
static  INT32 rawLastY;

/* user coordinates of last vertex connected to in polyline */
static  INT32 finalRawX;        
static  INT32 finalRawY;

/* global coordinates of last vertex connected to in polyline */
static  INT32 finalGblX;        
static  INT32 finalGblY;

/* # of lines until the next edge starts */
static  INT32 numLinesUntilNext;        

/* local copy of npointsparm for counting down */
static  INT32 lclNpointsparm;           

/* local copy of modeparm */
static  INT32 lclModeparm;              
                                     
/* vector called to update the AET each time in low-memory operation */
static  INT16 (*updateAETVector)(VOID); 

/* offset of first location after the last point struct in the points array */
static  SIGNED vertexListEnd;           

/* in previous (relative) mode only, the X and Y */
static  INT32 lastPointX; 

/* coordinates of the last point in the vertex list */
static  INT32 lastPointY; 

/* 1 if the first edge is being put into the GET */
static  signed char firstSegment;    

/* 1 if the last and first points are to be connected */
static  signed char finalWrap;       

/* start coordinates for current line */
static INT32 startX;         
static INT32 startY;

/* end coordinates for current line */
static INT32 endX;           
static INT32 endY;

/* start coordinates for added edge */
static INT32 addStartX;      
static INT32 addStartY;

/* end coordinates for added edge */
static INT32 addEndX;        
static INT32 addEndY;

/* start coordinates for next edge */
static INT32 nextStartX;     
static INT32 nextStartY;

/* offset of global edge table ptr in GET buffer */
static lineEdgeV **ptrToGETPtr;

/* offsets to edges of square pen */
static INT32 halfWidth[4];     

/* highest Y coord to be drawn */
static INT32 maxY;                     

/* Functions with local scope. */
static INT16 BuildGET(VOID);
static INT16 AddEdgeToGET(VOID);
static INT16 FindHighestEdge(VOID);
static INT16 AddEdgeToAET(VOID);
static INT16 BuildSquareWidePenPolyLineGET(VOID);
static VOID POLYGONS_rsScanEdgeList(VOID);
static VOID POLYGONS_rsConvertEdges( INT32 startX, INT32 endX, INT32 height, point **scanListPtrPtr,
                   INT32 skipFirst);
static VOID POLYGONS_rsScanPolygonEdge( INT32 startX, INT32 endX, INT32 height,
                          point **scanListPtrPtr, INT32 skipFirst,
                          restartStruc *restartStrucPtr, INT32 numScans);
static VOID ScanBurst( INT32 scanCnt, INT32 skipFirst, INT32 xAdjust, signed char scanDir,
                restartStruc *restartStrucPtr, point **scanListPtr,
                point *currentVertex, INT32 *currentXPtr, point *points, INT32 mode);

/***************************************************************************
* FUNCTION
*
*    FillPolygon
*
* DESCRIPTION
*
*    Function FillPolygon is the X-Windows compatible polygon drawer.
*    Convex polygons are simply scanned into a line list and then passed
*    to the filler. Non-convex and complex polygons are Y-sorted into a
*    global edge table, then scanned from top to bottom via an active edge
*    table. If the memory pool size is greater than or equal to
*    STACK_BUFFER_SIZE, the convex polygon edge list is maintained there;
*    otherwise, the convex polygon edge list is maintained in a buffer on
*    the stack. In either case, if the buffer fills, the contents are
*    drawn, and scanning continues, so convex polygons with any number of
*    edges can be handled without significant performance degradation. For
*    non-convex polygons, if there's enough memory the edge lists (active,
*    and global, if possible) are maintained in either the passed buffer
*    or the stack buffer; however, if the buffer overflows, a much slower
*    line-by-line approach is employed.  Therefore, it behooves the
*    application to provide as much memory in the memory pool as possible.
*
*    "Convex" means that every horizontal line drawn through the polygon
*    at any point would cross exactly two active edges (neither horizontal
*    lines nor zero-length edges count as active edges; both are
*    acceptable anywhere in the polygon), and that the right & left edges
*    never cross. (It's OK for them to touch, though, so long as the right
*    edge never crosses over to the left of the left edge.) Non-convex and
*    complex polygons won't be drawn properly if convex is specified.
*
*    "Non-convex" means that edges never cross. Complex polygons won't be
*    drawn properly if non-convex is specified. Non-convex polygons are
*    currently handled by the general-case complex polygon code.
*
*    "Complex" covers all polygons.
*
*    Polygon edges are scanned X-style: all interior points are filled; for
*    those points exactly on non-horizontal boundaries, only points that
*    have the body of the polygon immediately to the right are drawn; for
*    those points exactly on horizontal boundaries, only points that have
*    the body of the polygon immediately below are drawn. At vertices
*    joining horizontal and non-horizontal edges, the non-horizontal edge
*    rules apply.
*
* INPUTS
*
*    point *pointsparm  - Pointer to array of points that defines the polyline. 
*
*    UINT32 npointsparm - # of elements in the points array.
*
*    INT32 modeparm     - coordModeOrigin (coordinates are absolute) or coordModePrevious.
*
*    INT32 shape        - Shape (convex).
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID FillPolygon(point *pointsparm, INT32 npointsparm, INT32 modeparm, INT32 shape)
{
    INT16   Done          = NU_FALSE;
    signed char    JumpNotConvex = NU_FALSE;
    SIGNED  lclCoordAdjX;  /* local coord adjust */
    SIGNED  lclCoordAdjY;
    blitRcd *tmpBlit;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* there must be enough vertices to make a visible polygon */
    if( (npointsparm < 3 ) || (theGrafPort.pnLevel < 0) )
    {
        Done = NU_FALSE;
    }

    if( !Done )
    {
        /* copy clip rectangle over */
        clpR = ViewClip;

        /* decide which buffer to use, stack or memory pool */
        /* assume using general memory pool */
        buf1Ptr = mpWorkSpace;  

        bufCntr = (SIGNED) (mpWorkEnd - buf1Ptr);

#if     (WORKSPACE_BUFFER_SIZE < STACK_BUFFER_SIZE) 
        
        /* is the memory pool smaller than the stack buffer? */
        if( bufCntr <= STACK_BUFFER_SIZE )
        {
            /* set up to use the stack buffer */
            bufCntr = STACK_BUFFER_SIZE;    
            buf1Ptr = &buf1[0];
        }

#endif  /* (WORKSPACE_BUFFER_SIZE < STACK_BUFFER_SIZE) */
        
        /* for counting down */
        lclNpointsparm = npointsparm;   

        /* remember where the first vertex is */
        vertexListPtr  = pointsparm;     

        /* is the polygon guaranteed convex? */
        if( shape == convex )
        {
            /* yes, it's a convex polygon */
            if( globalLevel > 0 )
            {
                /* only local coords with port origin = upperLeft can be handled
                   by the special fast convex polygon filler */
                if( (theGrafPort.portFlags & (pfVirtual | pfUpper) ) ^ pfUpper )
                {
                    JumpNotConvex = NU_TRUE;
                }

                if( !JumpNotConvex )
                {
                    /* Set adjustments from local to global coordinates */
                    lclCoordAdjX = theGrafPort.portOrgn.X - theGrafPort.portRect.Xmin; 
                    lclCoordAdjY = theGrafPort.portOrgn.Y - theGrafPort.portRect.Ymin;
                }
            }
            else
            {
                /* zero local coord adjust */
                lclCoordAdjX = 0;
                lclCoordAdjY = 0;
            }
            
            if( !JumpNotConvex )
            {
                /* handle as special fast convex fill; now set up the fill record
                   according to the settings in the current port */
                tmpBlit = (blitRcd *) buf1Ptr;
                *tmpBlit = grafBlit;

                /* draw the convex polygon */
                POLYGONS_rsScanAndDrawConvex( buf1Ptr, bufCntr, pointsparm, npointsparm, modeparm, lclCoordAdjX, lclCoordAdjY);

                Done = NU_TRUE;
            }
    
        } /* if( shape == convex ) */

    } /* if( !Done ) */

    if( !Done )
    {
        /* See if there's room in the scratch buffer for the entire GET plus a
           blitRcd, two pointers, and at least one rect; if so, we can handle this
           the easy way, with a GET and an AET. */

        /* 1 for winding rule, 0 for odd/even */
        if( theGrafPort.portFlags & pfFillRule )
        {
            lclFillRule = 1;
        }
        else
        {
            lclFillRule = 0;
        }

        /* routine called to add each edge to the GET */
        addEdgeVector = AddEdgeToGET;  

        /* must leave room for at least one rect and a blitRcd - point to last byte at
           which an edge can start without overflowing the buffer */
        endAvailForGET = ( (SIGNED) buf1Ptr)
                     + bufCntr
                     - sizeof( blitRcd)
                     - sizeof( rect)
                     - sizeof( lineEdgeV);

        /* remember where the GET pointer is */
        ptrToGETPtr = (lineEdgeV **) (SIGNED) buf1Ptr;

        /* set the GET to empty to start */
        *ptrToGETPtr = 0;                            

        /* point past the GET and AET pointers */
        buf1Ptr += SIZEOFF * 2;                      

        /* create the GET, Y-X sorted */
        if( BuildGET() == 1 )
        {
            /* scan out the global edge table */
            /* # of bytes left for blitRcd */
            bufCntr -= ((SIGNED) buf1Ptr - (SIGNED) ptrToGETPtr); 
            EDGES_rsScansAndFillsEdgeList((VOID **)ptrToGETPtr, (blitRcd *)buf1Ptr, bufCntr, shape, (SIGNED) lclFillRule, 1);
        }
        else
        {
            /* There's not enough memory for the GET, two pointers, a blitRcd,
               and at least one rect, so we'll have to scan the whole vertex list
               each time to maintain the AET. */
            /* reset saved values */
            lclNpointsparm = npointsparm; 
            vertexListPtr = pointsparm;

            /* vector used to update the AET by scanning vertices */
            updateAETVector = BuildGET;  

            /* scan out the polygon */
            POLYGONS_rsScanEdgeList();
        }

    } /* if( !Done ) */

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    BuildGET
*
* DESCRIPTION
*
*    Function BuildGET builds a global edge table from the point list.
*    The GET is a linked list,  sorted first by Y coordinate, then by
*    X coordinate.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT16 - Returns 1 for success, 0 for failure (insufficient memory).
*
***************************************************************************/
static INT16 BuildGET(VOID)
{
    INT8  Done = NU_FALSE;
    INT16 value;
    point *vrtxListPtr; 

    /* remember this X,Y for next time */
    rawLastX = vertexListPtr->X;
    rawLastY = vertexListPtr->Y;

    vrtxListPtr = vertexListPtr;

    /* already in global? */
    if( globalLevel > 0 )
    {
        /* no, convert to global */
        U2GP(rawLastX, rawLastY, &nextStartX, &nextStartY, 0);
    }
    else
    {
        nextStartX = rawLastX;
        nextStartY = rawLastY;
    }

    do
    {
        /* is this the last vertex? */
        if( lclNpointsparm == 1 )
        {
            /* yes, so wrap back to the first point */
            /* load the endpoint of this segment and convert it
               to global coords */
            rawLastX = vertexListPtr->X;
            rawLastY = vertexListPtr->Y;
        }
        else
        {
            /* point to the next vertex */
            vrtxListPtr++;

            /* previous mode? */
            if( lclModeparm == coordModePrevious )
            {
                /* yes, calculate new position as a delta 
                   from the last X,Y in user coordinates */
                rawLastX += vrtxListPtr->X;
                rawLastY += vrtxListPtr->Y;
            }
            else
            {
                rawLastX = vrtxListPtr->X;
                rawLastY = vrtxListPtr->Y;
            }
        }

        /* already in global? */
        if( globalLevel > 0 )
        {
            /* no, convert to global */
            U2GP(rawLastX, rawLastY, &addEndX, &addEndY, 0);
        }
        else
        {
            addEndX = rawLastX;
            addEndY = rawLastY;
        }

        addStartX = nextStartX;
        addStartY = nextStartY;
        nextStartX = addEndX;
        nextStartY = addEndY;

        if( addEdgeVector() != 1 )
        {
            /* fail if return */
            value = 0; 

            /* Set the Done flag to exit the do while loop */
            Done = NU_TRUE;
        }

        if (!Done)
        {
            value = 1;
        }

    /* count down # of points remaining */    
    } while( (--lclNpointsparm > 0) && ( !Done ) ); 

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    AddEdgeToGET
*
* DESCRIPTION
*
*    Function AddEdgeToGET adds the edge to the GET, checking for
*    buffer overflow and maintaining Y primary/X secondary sorting.
*    Compares edge to clip rect, trivially rejecting if possible, and
*    adjusting top and length as needed to clip if partially clipped.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT16 - Returns 1 for success, 0 for failure (insufficient memory).
*
***************************************************************************/
static INT16 AddEdgeToGET(VOID)
{
    INT16     Done = NU_FALSE;
    INT16     value;

    INT32     tmpXY;
    INT32     localDeltaX;
    INT32     localDeltaY;
    lineEdgeV  *NextEdgePtr;
    lineEdgeV  *pGETPtr;
    lineEdgeV **lGETPtr;

    /* is there room for the structure? */
    if( ((SIGNED) buf1Ptr) >= endAvailForGET )
    {
        value = 0;
        Done  = NU_TRUE;
    }

    if( !Done )
    {
        pGETPtr = (lineEdgeV *) buf1Ptr;

        /* is this an active edge? */
        if( addStartY == addEndY)
        {
            value = 1;
            Done  = NU_TRUE;
        }
    }

    if( !Done )
    {
        if( addStartY < addEndY)
        {
            /* the edge is top to bottom, as desired */
            pGETPtr->TopToBottom = 1;
        }
        else
        {
            /* swap endpoints to make the edge go top to bottom */
            pGETPtr->TopToBottom = (signed char)-1;

            tmpXY = addStartX;
            addStartX = addEndX;
            addEndX = tmpXY;

            tmpXY = addStartY;
            addStartY = addEndY;
            addEndY = tmpXY;
        }

        /* see if we can trivially reject on the basis of Y coordinates */
        if( (addStartY >= clpR.Ymax) || (addEndY <= clpR.Ymin) )
        {
            value = 1;
            Done  = NU_TRUE;
        }
    }

    if( !Done )
    {
        pGETPtr->CurrentX = addStartX;
        pGETPtr->StartY   = addStartY;
        localDeltaY = addEndY - addStartY;
        pGETPtr->Count = localDeltaY;
        pGETPtr->ErrorTermAdjDownV = localDeltaY;

        /* check x direction */
        if( (localDeltaX = addEndX - addStartX) >= 0 )
        {
            /* left->right */
            pGETPtr->XDirection = 1;

            /* initial error term */
            pGETPtr->ErrorTermV = -1; 
        }
        else
        {
            /* right->left */
            pGETPtr->XDirection = (signed char)-1;

            /* initial error term */
            pGETPtr->ErrorTermV = -localDeltaY;  

            /* abs(DeltaX) */
            localDeltaX = -localDeltaX;               
        }

        /* X major or Y major? */
        if( localDeltaY >= localDeltaX )
        {
            /* Y major */
            pGETPtr->WholePixelXMoveV = 0;
            pGETPtr->ErrorTermAdjUpV = localDeltaX;
        }
        else
        {
            /* X major */
            pGETPtr->ErrorTermAdjUpV = localDeltaX % localDeltaY;
            if( pGETPtr->XDirection == 1 )
            {
                pGETPtr->WholePixelXMoveV = localDeltaX / localDeltaY;
            }
            else
            {
                pGETPtr->WholePixelXMoveV = - localDeltaX / localDeltaY;
            }
        }

        /* if the edge starts above the top clip, jump it ahead to the
           top clip scan line */

        /* does this edge start above top clip? */
        if( pGETPtr->StartY < clpR.Ymin )
        {
            /* yes, jump it ahead */
            localDeltaY = clpR.Ymin - pGETPtr->StartY;

            /* clip count */
            pGETPtr->Count -= localDeltaY;       

            /* new start Y (at top clip) */
            pGETPtr->StartY = clpR.Ymin;    

            /* adjust error term for # of times we would have adjusted it in
               advancing that many points */
            pGETPtr->ErrorTermV += (localDeltaY * pGETPtr->ErrorTermAdjUpV);

            /* check if it turned over */
            if( pGETPtr->ErrorTermV >= 0 )
            {
                /* yes it did so correct it */

                /* which direction? */
                if( pGETPtr->XDirection == 1 )
                {
                    /* left->right */
                    pGETPtr->CurrentX += ( (pGETPtr->ErrorTermV / pGETPtr->ErrorTermAdjDownV)
                                       + 1 );
                }
                else
                {
                    /* right to left */
                    pGETPtr->CurrentX -= ( (pGETPtr->ErrorTermV / pGETPtr->ErrorTermAdjDownV)
                                        + 1 );
                }

                pGETPtr->ErrorTermV = ( (pGETPtr->ErrorTermV % pGETPtr->ErrorTermAdjDownV)
                                   - pGETPtr->ErrorTermAdjDownV );
            }

            pGETPtr->CurrentX += (INT16) (localDeltaY * pGETPtr->WholePixelXMoveV);
        }

        /* if the edge goes past the bottom clip, truncate it there */
        /* # of scan lines to clip bottom, not including the clip bottom */
        localDeltaY = clpR.Ymax - pGETPtr->StartY; 

        if( pGETPtr->Count > localDeltaY )
        {
            pGETPtr->Count = localDeltaY;
        }

        /* finally, link the new edge in so that the edge list is still
           sorted by Y coordinate, and by X coordinate for all edges with the
           same Y coordinate */
        lGETPtr = ptrToGETPtr;
        
        while( ((NextEdgePtr = *lGETPtr) != 0) && ( !Done ) )
        {
            if(( NextEdgePtr->StartY > pGETPtr->StartY ) || 
              (( NextEdgePtr->StartY == pGETPtr->StartY) &&
               ( NextEdgePtr->CurrentX >= pGETPtr->CurrentX) ) )
            {
                Done = NU_TRUE;
            }
            else
            {
                lGETPtr = &NextEdgePtr->NextEdge;
            }
        }

        /* link in the edge before the next edge we just reached */
        pGETPtr->NextEdge = NextEdgePtr;
        *lGETPtr = pGETPtr;
        pGETPtr++;

        /* point to next free edge */
        buf1Ptr = (UINT8 *) (SIGNED) pGETPtr;

        value = 1;
    } /* if( !Done ) */

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    POLYGONS_rsScanAndDrawConvex
*
* DESCRIPTION
*
*    Function POLYGONS_rsScanAndDrawConvex scans and draws a convex polygon.  Intended for internal
*    use; doesn't check pnLevel.
*
*    NOTES:
*        blitCnt and blitList are modified in the passed-in blitRcd, so this
*        should be a throwaway copy, not a permanent or reusable blitRcd.
*
*        Coordinates must be either global (in which case the adjust values
*        should be 0), or local with port origin = TOPBOT (in which case the
*        adjust values convert from local to global). Coordinates that are
*        virtual or coordinates with port origin != TOPBOT will not produce
*        proper results!
*
* INPUTS
*
*    UINT8 *lclScratchBufferPtr - far pointer to buffer in which to build rect list;
*                                 the buffer must have a filled-in blitRcd at the top.
*                                 blitList is filled out in this module.
*                                 We don't just use the default blitRcd because:
*                                 a) this way the blitList and the blitRcd are both
*                                    in the same segment.
*                                 b) this way a caller can fill a convex polygon in any
*                                    arbitrary bitmap.
*
*    SIGNED lclScratchBufferSize - # of bytes in lclScratchBufferPtr (64K-1 max).
*                                  Size includes the blitRcd at the top of the buffer.
*
*    point * points              - far pointer to vertex list.
*
*    INT32 npoints               - # of points in points vertex list (must be > 3).
*
*    INT32 mode                  - coordModeOrigin (coordinates are absolute) or
*                                  coordModePrevious (coordinates after the first are relative
*                                  to the preceding one).
*
*    SIGNED lclXAdjust           - amount to add to X coordinates, used to perform local to
*                                  global coordinate conversion when needed, by being set to
*                                  portOrgn.X - portRect.Xmin; should be set to 0 
*                                  if coordinates are already in global coordinates.
*
*    SIGNED lclYAdjust           - amount to add to Y coordinates, used to perform local to
*                                  global coordinate conversion when needed, by being set to
*                                  portOrgn.Y - portRect.Ymin; should be set to 0 if coordinates
*                                  are already in global coordinates.
*
* OUTPUTS
*
*    INT16 - Returns 1 for success, 0 for failure (insufficient memory).
*
***************************************************************************/
VOID POLYGONS_rsScanAndDrawConvex( UINT8 *lclScratchBufferPtr, SIGNED lclScratchBufferSize,
                                   point * points, INT32 npoints, INT32 mode,
                                   SIGNED lclXAdjust, SIGNED lclYAdjust)
{
    INT16 Done = NU_FALSE;
    
    /* stores left edge scanning state for edges that are too long to scan in one pass */
    restartStruc leftRestartStruc;  
    
    /* ditto for the right edge */
    restartStruc rightRestartStruc; 
    
    /* local blit record pointer */
    blitRcd *lclBlitRcdPtr;         
    
    /* offset of next free entry in rect list for scanned out polygon */
    rect *scanListPtrR;             
    
    /* above offset as a point pointer for X values */
    point *scanListPtrP;            
    
    /* above offset as a point pointer for X values */
    point *scanListPtrPR;           
    
    /* maximum # of rect strucs that will fit into the working buffer */
    SIGNED maxRectCnt;              
    
    /* lowest Y coord in polygon */
    INT32 minPoint_Y;               
    
    /* highest Y coord in polygon */
    INT32 maxPoint_Y;               
    
    /* lowest X coord in poly */
    INT32 minX;                     
    
    /* highest X coord in poly */
    INT32 maxX;                     
    
    /* running total of relative points */
    point relPoint;                 
    
    /* pointer to point struc for leftmost point with Y coord of minPoint_Y */
    point *minIndexLPtr;            
    
    /* pointer to point struc for rightmost point with Y coord of minPoint_Y */
    point *minIndexRPtr;            
    
    /* copy of points pointer */
    point *lPoints;                 
    point *tmpMinIndexPtr;
    
    /* 1 if the top of the polygon is flat, 0 if it's pointed */
    INT32 topIsFlat;                
    
    /* Y coord of scan line we're currently processing */
    INT32 currentYScan;             
    
    /* # of scan lines in polygon */
    INT32 polyScans;                
    
    /* 1 to skip top scan line on left edge */
    INT32 skipFirstL;               
    
    /* likewise for right edge */
    INT32 skipFirstR;               
    
    /* 1 if left edge runs up through vertex list, -1 if it runs down */
    signed char leftEdgeDir;        
    
    /* 1 to skip filling a given burst of scan lines, 0 to fill */
    signed char skipFill;           
    
    /* in previous mode X coordinates of first point */
    INT32 leftEdgeStartX;           
    
    /* on each edge */
    INT32 rightEdgeStartX;          
    INT32 deltaXN;
    INT32 deltaYN;
    INT32 deltaXP;
    INT32 deltaYP;
    SIGNED deltaXPYN;
    SIGNED deltaXNYP;
    
    /* Clip rect */
    rect clpR;                      
    
    /* local copy of rectclip flag in the blitRcd */
    signed char rectClipFlag;       
    INT32 i;

    /* set the blitList pointer */
    lclBlitRcdPtr = (blitRcd *) ((SIGNED) lclScratchBufferPtr);
    lclBlitRcdPtr->blitList = (SIGNED) (lclBlitRcdPtr) + sizeof(blitRcd);

    /* copy clipping flags and clip rect (if enabled) to local vars
       ***region clipping is ignored at this level***/
    rectClipFlag = lclBlitRcdPtr->blitFlags & bfClipRect;
    if( rectClipFlag )
    {
        clpR.Xmin = lclBlitRcdPtr->blitClip->Xmin;
        clpR.Xmax = lclBlitRcdPtr->blitClip->Xmax;
        clpR.Ymin = lclBlitRcdPtr->blitClip->Ymin;
        clpR.Ymax = lclBlitRcdPtr->blitClip->Ymax;
    }

    /* scan the list to find the top and bottom of the polygon & figure out
       how many rects we can handle at time */
    maxRectCnt = (lclScratchBufferSize - sizeof(blitRcd)) / sizeof(rect);
    if( maxRectCnt <= 0 )
    {
        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* get first point */
        relPoint = *points;         

        /* initial min and max X coords */
        minX = relPoint.X;          
        maxX = minX;

        /* initial min and max Y coords */
        minPoint_Y = relPoint.Y;    
        maxPoint_Y = minPoint_Y;

        /* initial minIndexLPtr */
        minIndexLPtr = points;      

        /* X coord of the initial Ymin point */
        leftEdgeStartX = points->X; 

        /* get original pointer */
        lPoints = points;           

        /* point to the next vertex */
        lPoints++;                  

        /* count off first point, which we've already done */
        npoints--;                  
    
        /* which coord mode? */
        if( mode == coordModePrevious )
        {
            /* coord mode is previous (relative) */
            for( ; npoints >= 1; npoints--)
            {
                /* get the next vertex's X & Y coords */
                relPoint.X += lPoints->X; 

                /* as deltas from the last vertex and check min/max */
                relPoint.Y += lPoints->Y; 
                if( relPoint.X < minX )
                {
                    minX = relPoint.X;
                }
                else
                {
                    if( relPoint.X > maxX)
                    {
                        maxX = relPoint.X;
                    }
                }
                if( relPoint.Y < minPoint_Y )
                {
                    minPoint_Y = relPoint.Y;

                    /* update minIndexLPtr */
                    minIndexLPtr = lPoints;      

                    /* remember the X coord of the minimum Y point */
                    leftEdgeStartX = relPoint.X; 
                }
                else
                {
                    if( relPoint.Y > maxPoint_Y )
                    {
                        maxPoint_Y = relPoint.Y;
                    }
                }
                /* point to next vertex */
                lPoints++; 
            }

            /* remember the X & Y coordinates */
            lastPointX = relPoint.X; 

            /* of the last point */
            lastPointY = relPoint.Y; 
        } /* if( mode == coordModePrevious ) */
        else
        {
            /* coord mode is origin (absolute) */
            for( ; npoints >= 1; npoints-- )
            {
                if( lPoints->X < minX )
                {
                    minX = lPoints->X;
                }
                else
                {
                    if( lPoints->X > maxX)
                    {
                        maxX = lPoints->X;
                    }
                }
                if( lPoints->Y < minPoint_Y )
                {
                    minPoint_Y = lPoints->Y;

                    /* update minIndexLPtr */
                    minIndexLPtr = lPoints;      

                    /* remember the X coord of the minimum Y point */
                    leftEdgeStartX = lPoints->X; 
                }
                else
                {
                    if( lPoints->Y > maxPoint_Y )
                    {
                        maxPoint_Y = lPoints->Y;
                    }
                }
                /* point to next vertex */
                lPoints++;  
            }

        } /* else */

        if( minPoint_Y == maxPoint_Y )
        {
            Done = NU_TRUE;
        }
    } /* if( !Done ) */

    if( !Done )
    {
        /* remember where the vertex list ends */
        vertexListEnd = (SIGNED) lPoints; 

        /* reject the polygon if it's fully out of the visible area */
        if( rectClipFlag && (
			((minX + lclXAdjust) >= clpR.Xmax) ||
            ((maxX + lclXAdjust) <= clpR.Xmin) ||
            ((minPoint_Y + lclYAdjust) >= clpR.Ymax) ||
            ((maxPoint_Y + lclYAdjust) <= clpR.Ymin) ))
        {
            Done = NU_TRUE;
        }
    }

    if( !Done )
    {
        /* now find the two ends of the top edge */
        minIndexRPtr = minIndexLPtr;

        /* which coord mode? */
        if( mode == coordModePrevious )
        {
            /* scan in previous (relative) mode in ascending order to find
               the last top-edge point */
            /* initial X coord at Ymin point */
            relPoint.X = leftEdgeStartX;    
            while( !Done )
            {
                /* point to the next vertex */
                minIndexRPtr++;                   

                /* have we wrapped off the end? */
                if( (SIGNED) minIndexRPtr == vertexListEnd )  
                {
                    /* yes, wrap back to the start */
                    /* wrap back to the start */
                    minIndexRPtr = points;  

                    /* still at top? */
                    if( minIndexRPtr->Y != minPoint_Y )
                    {
                        /* no, wrap back to the end of the list */
                        minIndexRPtr = (point *) vertexListEnd;
                        
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                    else
                    {
                        /* initial point is absolute */
                        relPoint.X = minIndexRPtr->X;   
                    }
                    
                }
                else
                {
                    /* no , is this vertex still at the top? */
                    if( minIndexRPtr->Y != 0)
                    {
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                    else
                    {
                        /* keep track of the X coord */
                        relPoint.X += minIndexRPtr->X;  
                    }
                    
                }
            }

            /* Reset the Done flag for future use */
            Done = NU_FALSE;

            /* point back to the last vertex at the top */
            minIndexRPtr--; 
            rightEdgeStartX = relPoint.X;

            /* now scan in descending order to find the first top-edge point */
            /* initial X coord at min point */
            relPoint.X = leftEdgeStartX;    
            
            while ( !Done )
            {
                /* are we at the start of the list? */
                if( minIndexLPtr == points )
                {
                    /* yes, handle in origin mode */
                    /* is the last point in the list still at the minimum Y? */ 
                    if( lastPointY != minPoint_Y )
                    {
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;   
                    }
                    else
                    {
                        /* keep track of the X coord */
                        relPoint.X = lastPointX;                       
                        
                        /* point to the previous vertex */
                        minIndexLPtr = (point *) (vertexListEnd) - 1 ; 
                    }
                    
                }
                else
                {
                    /* no, handle in previous (relative) mode */
                    /* is the previous vertex still at the top? */
                    if( minIndexLPtr->Y != 0 )
                    {
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                    else
                    {
                        /* keep track of the X coord */
                        relPoint.X -= minIndexLPtr->X;  
                        
                        /* point to the previous vertex */
                        minIndexLPtr--;                 
                    }
                    
                }
            }

            /* Reset the Done flag for future use */
            Done = NU_FALSE;

            leftEdgeStartX = relPoint.X;
        }
        else
        {
            /* scan in origin (absolute) mode in ascending order to find
               the last top-edge point */
            while ( !Done )
            {
                /* point to the next vertex */
                minIndexRPtr++; 

                /* have we wrapped off the end? */
                if( (SIGNED) minIndexRPtr == vertexListEnd )
                {
                    /* yes, wrap back to the start */
                    /* wrap back to the start */
                    minIndexRPtr = points;  

                    /* still at top? */
                    if( minIndexRPtr->Y != minPoint_Y )
                    {
                        /* no, wrap back to the end of the list */
                        minIndexRPtr = (point *) vertexListEnd;
                        
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                }
                else
                {
                    /* no , is this vertex still at the top? */
                    if( minIndexRPtr->Y != minPoint_Y )
                    {
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                }
            }

            /* Reset the Done flag for future use */
            Done = NU_FALSE;

            /* point back to the last vertex at the top */
            minIndexRPtr--; 
            rightEdgeStartX = minIndexRPtr->X;

            /* now scan in descending order to find the first top-edge point */
            while ( !Done )
            {
                /* are we at the start of the list? */
                if( minIndexLPtr == points )
                {
                    /* yes, is the last point in the list still at the minimum Y? */    
                    /* point to the previous vertex */
                    minIndexLPtr = (point *) (vertexListEnd) - 1; 

                    if( minIndexLPtr->Y != minPoint_Y )
                    {
                        /* wrap back to start */
                        minIndexLPtr = points; 
                        
                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                }
                else
                {
                    /* no, is the previous vertex still at the top? */
                    /* point to the previous vertex */
                    minIndexLPtr--; 

                    if( minIndexLPtr->Y != minPoint_Y )
                    {
                        /* point back to the last vertex */
                        minIndexLPtr++; 

                        /* Set flag to exit the while loop */
                        Done = NU_TRUE;
                    }
                }
            }

            /* Reset the Done flag for future use */
            Done = NU_FALSE;

            leftEdgeStartX = minIndexLPtr->X;
        }

        /* Figure out which direction through the vertex list from the top
           vertex is the left edge and which is the right.  At this point,
           minIndexRPtr points to the last vertex on the top edge in the
           ascending direction through the vertex list, and minIndexLPtr points
           to the last vertex on the top edge in the descending direction */

        /* assume left edge runs down thru vertex list */
        leftEdgeDir = (signed char)-1; 

        /* assume that the top isn't flat */
        topIsFlat = 0;          

        /* which coord mode? */
        if( mode == coordModePrevious )
        {
            /* coord mode is previous (relative) */
            /* is top flat? */
            if( leftEdgeStartX == rightEdgeStartX )
            {
                /* the top isn't flat */
                /* point to the downward end of the first line of each of the
                   two edges down from the top; calculate X and Y lengths from
                   the top vertex to the end of the first line down each edge;
                   use those to compare slopes and see which line is leftmost */
                tmpMinIndexPtr = minIndexRPtr;

                /* point to the next vertex */
                minIndexRPtr++; 

                /* have we wrapped? */
                if( (SIGNED) minIndexRPtr == vertexListEnd )
                {
                    /* yes, wrap to start of the list */
                    minIndexRPtr = points;
                    deltaXN = minIndexRPtr->X;
                }
                else
                {
                    /* no, so calculate the X coord of the first point off the
                       right end of the top as a delta from the right-end point */
                    deltaXN = rightEdgeStartX + minIndexRPtr->X;
                }
                /* the first point down off the top edge incrementing through the list */
                deltaXN -= leftEdgeStartX;  

                /* unless this is the first point in the list */
                deltaYN = minIndexRPtr->Y;  
                if( minIndexRPtr == points )
                {
                    deltaYN -= minPoint_Y;
                }

                /* now, calculate deltas to the previous point (decrementing
                   through list) */
                /* restore top pointer */
                minIndexRPtr = tmpMinIndexPtr;  

                /* are we at the top of the list? */
                if( minIndexLPtr == points )
                {
                    /* yes */
                    deltaXP = lastPointX - leftEdgeStartX;
                    deltaYP = lastPointY - minPoint_Y;
                }
                else
                {
                    deltaXP = - minIndexLPtr->X;
                    deltaYP = - minIndexLPtr->Y;
                }

                deltaXPYN = deltaXP * deltaYN;
                deltaXNYP = deltaXN * deltaYP;
                if( deltaXNYP < deltaXPYN )
                {
                    leftEdgeDir = 1;
                }
            }
            else
            {
                /* the top is flat, so just see which of the ends is leftmost;
                   if it's the one we've already marked as left, we're all set,
                   otherwise swap the left and right edges */
                topIsFlat = 1;
                if( leftEdgeStartX > rightEdgeStartX )
                {
                    /* swap edges */
                    relPoint.X = leftEdgeStartX;
                    leftEdgeStartX = rightEdgeStartX;
                    rightEdgeStartX = relPoint.X;
                    tmpMinIndexPtr = minIndexLPtr;
                    minIndexLPtr = minIndexRPtr;
                    minIndexRPtr = tmpMinIndexPtr;
                    leftEdgeDir = 1;
                }
            }
        }
        else
        {
            /* coord mode is origin (absolute) */
            /* is top flat? */
            if( leftEdgeStartX == rightEdgeStartX )
            {
                /* the top isn't flat */
                /* point to the downward end of the first line of each of the
                   two edges down from the top; calculate X and Y lengths from
                   the top vertex to the end of the first line down each edge;
                   use those to compare slopes and see which line is leftmost */
                tmpMinIndexPtr = minIndexRPtr;
                
                /* point to the next vertex */
                minIndexRPtr++; 
                if( (SIGNED) minIndexRPtr == vertexListEnd )
                {
                    minIndexRPtr = points;
                }   

                deltaXN = minIndexRPtr->X - minIndexLPtr->X;
                deltaYN = minIndexRPtr->Y - minIndexLPtr->Y;

                /* save top pointer for later */
                minIndexRPtr = minIndexLPtr;    

                if( minIndexRPtr == points)
                {
                    minIndexRPtr = (point *) vertexListEnd;
                }
            
                /* point to the previous vertex */
                minIndexRPtr--;
                deltaXP = minIndexRPtr->X - minIndexLPtr->X;
                deltaYP = minIndexRPtr->Y - minIndexLPtr->Y;

                /* restore top pointer */
                minIndexRPtr = tmpMinIndexPtr;  

                deltaXPYN = deltaXP * deltaYN;
                deltaXNYP = deltaXN * deltaYP;
                if( deltaXNYP < deltaXPYN )
                {
                    leftEdgeDir = 1;
                }
            }
            else
            {
                /* the top is flat, so just see which of the ends is leftmost;
                   if it's the one we've already marked as left, we're all set,
                   otherwise swap the left and right edges */
                topIsFlat = 1;
                if( leftEdgeStartX > rightEdgeStartX )
                {
                    /* swap edges */
                    relPoint.X = leftEdgeStartX;
                    leftEdgeStartX = rightEdgeStartX;
                    rightEdgeStartX = relPoint.X;
                    tmpMinIndexPtr = minIndexLPtr;
                    minIndexLPtr = minIndexRPtr;
                    minIndexRPtr = tmpMinIndexPtr;
                    leftEdgeDir = 1;
                }
            }
        }

        /* set the # of scan lines in the polygon, skipping the bottom edge
           and also skipping the top vertex if the top isn't flat because in that
           case the top vertex has only a right edge component, and set the top
           scan line to draw, which is likewise the second line of the polygon
           unless the top is flat */

        /* do we do any clipping? */
        if( rectClipFlag == 0 )
        {
            /* no--valid coordinates are guaranteed at a higher level */
            polyScans = maxPoint_Y;
        }
        else
        {
            /* adjust to local coords */
            polyScans = (INT32) (clpR.Ymax - lclYAdjust); 
            if( polyScans > maxPoint_Y )
            {
                polyScans = maxPoint_Y;
            }
        }

        polyScans += (topIsFlat - minPoint_Y);
        polyScans -= 1;
        
        if( polyScans == 0 )
        {
            Done = NU_TRUE;
        }

    } /* if( !Done ) */

    if( !Done )
    {
        currentYScan = (INT32) (minPoint_Y + 1 - topIsFlat + lclYAdjust);

        /* scan down the left edge and then the right edge, in bursts of
           either the edge size or the max number of rects that will fit in
           the buffer */
        /* skip first point only if top isn't flat */
        skipFirstL = topIsFlat ^ 1; 
        skipFirstR = skipFirstL;

        /* first time through, no restart is in progress */
        leftRestartStruc.scanEdgeFirstPass = 1;
        rightRestartStruc.scanEdgeFirstPass = 1;
        do
        {
            /* scan loop */
            lclBlitRcdPtr->blitCnt = polyScans;

            /* assume we're not off the top of the clip rect */
            skipFill = 0;
			
			/* Folowing code segment doesn't seem to serve any purpse,
			   and it even disturbs the polygon drawing process when
			   Y co-ordinate of an edge is -ve.
			   This code has been excluded from the build for now,
			   we will revisit and modify the code to introduce an 
			   improvement to discard the polygon portion outside 
			   display area */
#if 0
            if( (rectClipFlag != 0) && (clpR.Ymin > currentYScan) )
            {
                /* clipping required */
                if((maxRectCnt + currentYScan) < 0)
                {
                    polyScans -= maxRectCnt;
                    lclBlitRcdPtr->blitCnt = polyScans;
                    
                    /* mark that we're off the clip rect top */
                    skipFill++; 
                    currentYScan += maxRectCnt; 

                }
                else
                {
                    polyScans -= clpR.Ymin - currentYScan;
                    lclBlitRcdPtr->blitCnt = clpR.Ymin - currentYScan;

                    /* mark that we're off the clip rect top */
                    skipFill++;                 

                    /* for next time */
                    currentYScan = clpR.Ymin;   
                }
            }
#endif			
            scanListPtrR  = (rect *) (((SIGNED) lclScratchBufferPtr) + sizeof(blitRcd));
            scanListPtrP  = (point *) (SIGNED) scanListPtrR;
            scanListPtrPR = scanListPtrP + 1;

            if( lclBlitRcdPtr->blitCnt > maxRectCnt )
            {
                lclBlitRcdPtr->blitCnt = (INT16) maxRectCnt;
            }


            /* set the initial pointer for storing scan converted left-edge coords */
            /* set the Y coords for all scan lines */
            /* are we off the top? */
            if( skipFill == 0 )
            {
                /* no */
                /* count the lines we'll do in this burst off from remaining total */
                polyScans -= (lclBlitRcdPtr->blitCnt + skipFirstL);
                for (i = 0; i < lclBlitRcdPtr->blitCnt; i++)
                {
                    scanListPtrR->Ymin = currentYScan;
                    currentYScan++;
                    scanListPtrR->Ymax = currentYScan;
                    scanListPtrR = scanListPtrR + 1;
                }
            }

            /* scan out as much of the left edge as possible */
            ScanBurst(lclBlitRcdPtr->blitCnt, skipFirstL,  lclXAdjust,(signed char) leftEdgeDir,
                &leftRestartStruc, &scanListPtrP, minIndexLPtr, &leftEdgeStartX,
                points, mode);

            /* don't skip the first point from now on */
            skipFirstL = 0; 

            /* scan out as much of the right edge as possible */
            /* first, set scan list pointer to point to Xmax values */
            ScanBurst(lclBlitRcdPtr->blitCnt, skipFirstR, lclXAdjust,(signed char) -leftEdgeDir,
                &rightRestartStruc, &scanListPtrPR, minIndexRPtr, &rightEdgeStartX,
                points, mode);

            /* don't skip the first point from now on */
            skipFirstR = 0; 

            /* skip the fill if all lines in this burst are above the top edge
               of the clip rect; else draw the rectangles */
            if( skipFill == 0 )
            {
                lclBlitRcdPtr->blitDmap->prFill(lclBlitRcdPtr);
            }

        } while( polyScans > 0 );

    } /* if( !Done ) */

}

/***************************************************************************
* FUNCTION
*
*    ScanBurst
*
* DESCRIPTION
*
*    Function ScanBurst scans out edges for POLYGONS_rsScanAndDrawConvex
*     - updates pointer to next vertex.
*
* INPUTS
*
*    INT32 scanCnt                 - # of scan lines to scan.
*
*    INT32 skipFirst               - # of scan lines drawn for this edge
*
*    INT16 xAdjust                 - Scan adjust.
*
*    signed char scanDir                  - Scan direction - ascending = 1.
*
*    restartStruc *restartStrucPtr - Pointer to the restartStruc structure.
*
*    point **scanListPtr           - Location of point.
*
*    point *currentVertex          - Pointer to the current vertex.
*
*    INT16 *currentXPtr            - Pointer to the next vertex.
*
*    point *points                 - Pointer to vertex list.
*
*    INT32 mode                    - coordModeOrigin (coordinates are absolute) or
*                                    coordModePrevious (coordinates after the first are
*                                    relative to the preceding one).
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID ScanBurst( INT32 scanCnt, INT32 skipFirst, INT32 xAdjust, signed char scanDir,
                restartStruc *restartStrucPtr, point **scanListPtr,
                point *currentVertex, INT32 *currentXPtr, point *points, INT32 mode)
{
    INT16 Done                    = NU_FALSE;
    INT16 JumpSBP_NextEdgeReentry = NU_FALSE;
    INT16 JumpSB_NextEdgeReentry  = NU_FALSE;

    /* used by previous mode code */
    INT32 tempXDelta; 
    INT32 tempYDelta = 0;
    INT32 startX = 0;
    INT32 endX = 0;
    point *nextVertex;

    /* # of scan lines to scan out for edge when doing restartable scanning */
    INT32 numScans;   

    /* get passed vertex pointer */
    nextVertex = currentVertex; 

    while( !Done )
    {
        /* are we restarting this edge? */
        if( (restartStrucPtr->scanEdgeFirstPass == 1) || JumpSB_NextEdgeReentry 
                                                      || JumpSBP_NextEdgeReentry)
        {
            /* no, draw from the start, scan convert each line in the edge from
               top to bottom, for as long as we have room to store the rects */
            if( !JumpSB_NextEdgeReentry && !JumpSBP_NextEdgeReentry )
            {
                restartStrucPtr->restartVector = 0;
            }

            /* which coord mode? */
            if( mode == coordModePrevious)
            {
                /* coord mode is previous (relative) */
                do
                {
                    if( !JumpSBP_NextEdgeReentry )
                    {
                        /* scan in ascending order? */
                        if( scanDir == 1 ) 
                        {
                            /* yes, point to the next vertex */
                            nextVertex = nextVertex + 1;
                            if( (SIGNED) nextVertex == vertexListEnd )
                            {
                                /* wrap back to the start */
                                nextVertex = points;

                                /* first point is absolute */
                                tempXDelta = nextVertex->X - lastPointX;
                                tempYDelta = nextVertex->Y - lastPointY;
                            }
                            else
                            {
                                tempXDelta = nextVertex->X;
                                tempYDelta = nextVertex->Y;
                            }
                        }
                        else
                        {
                            /* no, point to previous vertex */
                            if( nextVertex == points )
                            {
                                /* at the top of the list */
                                nextVertex = (point *) vertexListEnd;

                                /* first point is absolute */
                                tempXDelta = lastPointX - nextVertex->X;
                                tempYDelta = lastPointY - nextVertex->Y;
                            }
                            else
                            {
                                tempXDelta = - nextVertex->X;
                                tempYDelta = - nextVertex->Y;
                            }
                            nextVertex = nextVertex - 1;
                        }

                        /* absolute X of this vertex */
                        startX = *currentXPtr;

                        /* absolute X of next vertex */
                        endX = startX + tempXDelta; 
                        *currentXPtr  = endX;

                        /* # of scan lines drawn for this edge */
                        tempYDelta -= skipFirst;

                        /* any rects to draw? */
                        if( tempYDelta != 0 )
                        {
                            /* do we have that much room? */
                            if( (tempYDelta < 0) || (tempYDelta > scanCnt) )
                            {
                                /* no, we'll have to start and then suspend it */
                                /* undo earlier skipFirst to get true height */
                                scanCnt += skipFirst;
                                tempYDelta += skipFirst;
                                numScans = scanCnt;

                                /* # of scan lines left to process in this edge */
                                restartStrucPtr->restartRemaining = tempYDelta - scanCnt;
                                startX += xAdjust;
                                endX   += xAdjust;
                                POLYGONS_rsScanPolygonEdge(startX, endX, tempYDelta, scanListPtr,
                                                    skipFirst, restartStrucPtr, numScans);

                                /* Set the done flag to exit the loop */
                                Done = NU_TRUE;
                            }
                            else
                            {
                                /* yes, we can draw this whole edge in a single burst */
                                /* # of scan lines available after this */
                                scanCnt -= tempYDelta;   

                                /* undo earlier skipFirst to get true height */
                                tempYDelta += skipFirst; 
                                startX += xAdjust;
                                endX   += xAdjust;
                                POLYGONS_rsConvertEdges( startX, endX, tempYDelta, scanListPtr,
                                              skipFirst);
                                scanCnt += skipFirst;
                            }
                        } /* if( tempYDelta != 0 ) */

                    } /* if( !JumpSBP_NextEdgeReentry ) */

                    if (!Done)
                    {
                        JumpSBP_NextEdgeReentry = NU_TRUE;
                        
                        /* don't skip from now on */
                        skipFirst = 0; 
                    }
                    
                /* did this edge exactly finish out the buffer or complete the polygon? */
                } while( (scanCnt > 0) && ( !Done ) );

                /* Set the Done flag to exit the loop */
                Done = NU_TRUE;

            } /* if( (mode == coordModePrevious) && !JumpSB_NextEdgeReentry) */
            else
            {                
                /* coord mode is origin (absolute) */
                do
                {
                    if( !JumpSB_NextEdgeReentry )
                    {
                        /* get previous coords */
                        startX = nextVertex->X; 
                        tempYDelta = nextVertex->Y;

                        /* scan in ascending order? */
                        if( scanDir == 1 )
                        {
                            /* yes, point to the next vertex */
                            nextVertex = nextVertex + 1;
                            if( (SIGNED) nextVertex == vertexListEnd)
                            {
                                nextVertex = points;
                            }
                        }
                        else
                        {
                            /* no, point to previous vertex */
                            if( nextVertex == points )
                            {
                                nextVertex = (point *) vertexListEnd;
                            }
                            nextVertex = nextVertex - 1;
                        }

                        /* # of scan lines drawn for this edge */
                        /* X of next vertex */
                        endX = nextVertex->X;   
                        tempYDelta = nextVertex->Y - tempYDelta - skipFirst;

                        /* any rects to draw? */
                        if( tempYDelta != 0 )
                        {
                            /* do we have that much room? */
                            if( (tempYDelta < 0) || (tempYDelta > scanCnt) )
                            {
                                /* no, we'll have to start and then suspend it */
                                /* undo earlier skipFirst to get true height */
                                scanCnt += skipFirst;
                                tempYDelta += skipFirst;
                                numScans = scanCnt;

                                /* # of scan lines left to process in this edge */
                                restartStrucPtr->restartRemaining = tempYDelta - scanCnt;
                                startX += xAdjust;
                                endX += xAdjust;
                                POLYGONS_rsScanPolygonEdge(startX, endX, tempYDelta, scanListPtr,
                                                    skipFirst, restartStrucPtr, numScans);

                                /* Set the done flag to exit the loop */
                                Done = NU_TRUE;
                            }
                            else
                            {
                                /* yes, we can draw this whole edge in a single burst */
                                /* # of scan lines available after this */
                                scanCnt -= tempYDelta;   

                                /* undo earlier skipFirst to get true height */
                                tempYDelta += skipFirst; 
                                startX += xAdjust;
                                endX += xAdjust;
                                POLYGONS_rsConvertEdges(startX, endX, tempYDelta, scanListPtr, skipFirst);
                                scanCnt += skipFirst;
                            }

                        } /* if( tempYDelta != 0 */

                    } /* if( !JumpSB_NextEdgeReentry ) */

                    if (!Done)
                    {
                        JumpSB_NextEdgeReentry = NU_FALSE;
                        
                        /* don't skip from now on */
                        skipFirst = 0;  
                    }

                    /* did this edge exactly finish out the buffer or complete
                       the polygon? */
                } while( (scanCnt > 0) && (!Done) );

                /* Exit the while( !done ) loop */
                Done = NU_TRUE;

            } /* else */
            
        } /* if( (restartStrucPtr->scanEdgeFirstPass == 1) || JumpSB_NextEdgeReentry 
                                                           || JumpSBP_NextEdgeReentry) */
        else
        {
            /* yes, do we have that much room? */
            if( restartStrucPtr->restartRemaining <= scanCnt )
            {
                /* yes, so draw whatever remains and move on to the next edge */
                numScans = (INT32) restartStrucPtr->restartRemaining;

                /* # of scan lines available after this */
                scanCnt -= numScans;    

                /* next time we're starting a new edge, not restarting this one */
                restartStrucPtr->scanEdgeFirstPass = 1;

                /* scan in ascending order? */
                if( scanDir == 1 )
                {
                    /* yes, point to the next vertex */
                    nextVertex = nextVertex + 1;
                    if( (SIGNED) nextVertex == vertexListEnd )
                    {
                        nextVertex = points;
                    }
                }
                else
                {
                    /* no, point to previous vertex */
                    if( nextVertex == points )
                    {
                        nextVertex = (point *) vertexListEnd;
                    }
                    nextVertex = nextVertex - 1;
                }
                /* do the last restart */
                POLYGONS_rsScanPolygonEdge( startX, endX, tempYDelta, scanListPtr, skipFirst,
                                     restartStrucPtr, numScans);

                /* which coord mode? */
                if( mode == coordModePrevious )
                {
                    JumpSBP_NextEdgeReentry = NU_TRUE;
                }
                else
                {
                    JumpSB_NextEdgeReentry = NU_TRUE;
                }
            }
            else
            {
                /* no, we'll have to restart and then suspend it again */
                numScans = scanCnt;

                /* # of scan lines left to process in this edge */
                restartStrucPtr->restartRemaining -= scanCnt;
                POLYGONS_rsScanPolygonEdge( startX, endX, tempYDelta, scanListPtr, skipFirst,
                                     restartStrucPtr, numScans);

                /* done with this burst */
                Done = NU_TRUE; 
            }
        }

    } /* while( !Done ) */

}

/***************************************************************************
* FUNCTION
*
*    POLYGONS_rsScanPolygonEdge
*
* DESCRIPTION
*
*    Function POLYGONS_rsScanPolygonEdge is for use in scanning convex polygon
*    edges for which there is not adequate room in the rect list (need to
*    be restarted). This is the entry point the first time such an edge is
*    scanned; restartVector in the restart structure should be called to
*    restart the line thereafter.
*
*    Edges must not go bottom to top.
*
*    Updates scanListPtr to next element to set in array of rects.
*
* INPUTS
*
*    INT32 startX                   - Start X. 
*
*    INT32 endX                     - Start Y.
*
*    INT32 height                   - Height.
*
*    point **scanListPtrPtr         - Location of scan list pointer.
* 
*    INT32 skipFirst                - # of scan lines drawn for this edge
*
*    restartStruc *restartStrucPtr  - Pointer to the restartStruc structure.
* 
*    INT32 numScans                 - Number of scans.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID POLYGONS_rsScanPolygonEdge( INT32 startX, INT32 endX, INT32 height,
                          point **scanListPtrPtr, INT32 skipFirst,
                          restartStruc *restartStrucPtr, INT32 numScans)
{
    INT16 Done              = NU_FALSE;
    INT16 JumpVerticalLoopR = NU_FALSE;
    INT16 JumprestartEntry  = NU_FALSE;
    INT16 JumpDiagonalLoopR = NU_FALSE;
    INT16 JumpXMajorLoopR   = NU_FALSE;
    INT16 JumpYMajorLoopR   = NU_FALSE;

    INT32 errorTerm;
    INT32 advanceAmt;
    INT32 deltaX;
    INT32 advanceX;
    INT32 advanceErrorTerm;
    point *scanListPtr;

    /* get pointer to store data */
    scanListPtr = *scanListPtrPtr; 

    switch ( restartStrucPtr->restartVector )
    {
    /* entry points to load suspended scanning state */
    case VerticalRestart:
        startX = restartStrucPtr->restartSN;

        /* never need to skip first point on restart */
        JumpVerticalLoopR = NU_TRUE; 
        break;

    case DiagonalRestart:
        advanceAmt = restartStrucPtr->restartDE;
        startX     = restartStrucPtr->restartSN;
        JumpDiagonalLoopR = NU_TRUE;
        break;

    case XMajorRestart:
        advanceAmt       = restartStrucPtr->restartAdvanceAmt;
        height           = restartStrucPtr->restartHeight;
        advanceX         = restartStrucPtr->restartAE;
        errorTerm        = restartStrucPtr->restartCE;
        advanceErrorTerm = restartStrucPtr->restartDE;
        startX           = restartStrucPtr->restartSN;
        JumpXMajorLoopR = NU_TRUE;
        break;

    case YMajorRestart:
        deltaX     = restartStrucPtr->restartAE;
        errorTerm  = restartStrucPtr->restartCE;
        advanceAmt = restartStrucPtr->restartDE;
        startX     = restartStrucPtr->restartSN;
        height     = restartStrucPtr->restartBR;
        JumpYMajorLoopR = NU_TRUE;    
        break;

    default:
        JumprestartEntry = NU_TRUE;
    }


    if( JumpVerticalLoopR )
    {
        for(; numScans > 0; numScans--)
        {
            scanListPtr->X = startX;
            scanListPtr += 2;
        }
        /* update pointer for next time */
        *scanListPtrPtr = scanListPtr; 
        Done = NU_TRUE;
    }

    if( !Done && JumprestartEntry )
    {
        /* not the first pass after this */
        restartStrucPtr->scanEdgeFirstPass = 0;
        if( (deltaX = endX - startX) == 0 )
        {
            /* it's a vertical edge--special case it */
            /* restart vector for next time */
            restartStrucPtr->restartVector = VerticalRestart;

            /* this doesn't change from one pass to the next */
            restartStrucPtr->restartSN = startX; 
            numScans -= skipFirst;
            if( numScans == 0 )
            {
                /* no scan lines left after skipping 1st */
                Done = NU_TRUE; 
            }

            if( !Done )
            {
                for(; numScans > 0; numScans--)
                {
                    scanListPtr->X = startX;
                    scanListPtr += 2;
                }
                /* update pointer for next time */
                *scanListPtrPtr = scanListPtr;  
                Done = NU_TRUE;
            }

        } /* if( (deltaX = endX - startX) == 0 ) */

    } /* if( !Done && JumprestartEntry ) */

    if( !Done && JumprestartEntry )
    {
        if( deltaX < 0 )
        {
            /* move left as we draw */
            errorTerm = -height;
            advanceAmt = -1;

            /* width = abs(deltaX) */
            deltaX = -deltaX; 
        }
        else
        {
            errorTerm = -1;
            advanceAmt = 1;
        }
    }
    
    /* Figure out whether the edge is diagonal, X-major (more horizontal),
       or Y-major (more vertical) and handle appropriately.*/
    if( !Done && (JumprestartEntry || JumpDiagonalLoopR) )
    {
        if( deltaX == height )    
        {
            if( !JumpDiagonalLoopR )
            {
                /* it's a diagonal edge--special case */
                /* restart vector for next time */
                restartStrucPtr->restartVector = DiagonalRestart;

                /* this doesn't change from one pass to the next */
                restartStrucPtr->restartDE = advanceAmt; 
                if( skipFirst == 1 )
                {
                    /* skip the first point */
                    startX += advanceAmt;
                    numScans--;
                }
            } /* if( !JumpDiagonalLoopR ) */
         
            while( numScans > 0 )
            {
                scanListPtr->X = startX;
                scanListPtr += 2;
                startX += advanceAmt;
                numScans--;
            }
            restartStrucPtr->restartSN = startX;

            /* update pointer for next time */
            *scanListPtrPtr = scanListPtr;  
            Done = NU_TRUE;
        } 
    }


    if( !Done && (JumprestartEntry || JumpXMajorLoopR ) )
    {
        if( deltaX > height )
        {
            if( !JumpXMajorLoopR)
            {
                /* it's an X-major (more horz) edge */
                /* restart vector for next time */
                restartStrucPtr->restartVector     = XMajorRestart;
                restartStrucPtr->restartHeight     = height;
                restartStrucPtr->restartAdvanceAmt = advanceAmt;
                advanceX         = deltaX / height;
                advanceErrorTerm = deltaX % height;

                if( advanceAmt < 0 )
                {
                    advanceX = -advanceX;
                }
                restartStrucPtr->restartAE = advanceX;
                restartStrucPtr->restartDE = advanceErrorTerm;

                if( skipFirst == 1 )
                {
                    /* skip the first point */
                    startX += advanceX;
                    errorTerm += advanceErrorTerm;
                    if( errorTerm >= 0 )
                    {
                        /* time for X coord to advance one extra */
                        startX += advanceAmt;
                        errorTerm -= height;
                    }
                    numScans--;
                }
            }

            while( numScans > 0 )
            {
                scanListPtr->X = startX;
                scanListPtr += 2;
                startX += advanceX;
                errorTerm += advanceErrorTerm;
                if( errorTerm >= 0 )
                {
                    /* time for X coord to advance one extra */
                    startX += advanceAmt;
                    errorTerm -= height;
                }
                numScans--;
            }
            restartStrucPtr->restartCE = errorTerm;
            restartStrucPtr->restartSN = startX;

            /* update pointer for next time */
            *scanListPtrPtr = scanListPtr;  
            Done = NU_TRUE;
        }
    }

    if( !Done && (JumprestartEntry || JumpYMajorLoopR ) )
    {
        if( !JumpYMajorLoopR )
        {
            /* it's a Y-major (more vertical) edge */
            /* restart vector for next time */
            restartStrucPtr->restartVector = YMajorRestart;
            restartStrucPtr->restartAE     = deltaX;
            restartStrucPtr->restartDE     = advanceAmt;
            restartStrucPtr->restartBR     = height;
            if( skipFirst == 1 )
            {
                /* skip the first point */
                errorTerm += deltaX;
                if( errorTerm >= 0 )
                {
                    /* time for X coord to advance */
                    startX += advanceAmt;
                    errorTerm -= height;
                }
                numScans--;
            }
        }

        while( numScans > 0 )
        {
            scanListPtr->X = startX;
            scanListPtr += 2;
            errorTerm += deltaX;
            if( errorTerm >= 0 )
            {
                /* time for X coord to advance */
                startX += advanceAmt;
                errorTerm -= height;
            }
            numScans--;
        }
        restartStrucPtr->restartCE = errorTerm;
        restartStrucPtr->restartSN = startX;

        /* update pointer for next time */
        *scanListPtrPtr = scanListPtr;  

    }
}

/***************************************************************************
* FUNCTION
*
*    POLYGONS_rsConvertEdges
*
* DESCRIPTION
*
*    Function POLYGONS_rsConvertEdges scan converts an edge from (X1, Y1) to (X2, Y2),
*    not including the point at (X2, Y2). If skipFirst == 1, the point at (X1,
*    Y1) isn't drawn; if skipFirst == 0, it is. For each scan line, the pixel
*    closest to the scanned edge without being to the left of the scanned edge
*    is chosen. Scan-converted X coordinates are stored in the array of rects
*    pointed to by DI. Uses an all-integer approach for speed & precision.
*
*    POLYGONS_rsConvertEdges is for use in scanning convex polygon edges for which there
*    is adequate room in the rect list (don't need to be restarted).
*
*    Edges must not go bottom to top.
*
*    Updates scanListPtr to next element to set in array of rects.
*
* INPUTS
*
*    INT32 startX           - Starting X.
*
*    INT32 endX             - End value of X.
*
*    INT32 height           - Height (Y2 - Y1).
*
*    point **scanListPtrPtr  - Element to set in array of rects.
*
*    INT32 skipFirst         - 1: the point at (X1,Y1) isn't drawn.
*                             0: skip.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID POLYGONS_rsConvertEdges( INT32 startX, INT32 endX, INT32 height, point **scanListPtrPtr,
                   INT32 skipFirst)
{
    INT16 Done = NU_FALSE;
    INT32 errorTerm;
    INT16 advanceAmt;
    INT32 deltaX;
    INT32 advanceX;
    INT32 advanceErrorTerm;
    INT32 i;
    point *scanListPtr;

    /* get pointer to store data */
    scanListPtr = *scanListPtrPtr;  
    if( (deltaX = endX - startX) == 0 )
    {
        /* it's a vertical edge--special case it */
        height -= skipFirst;
        if( height == 0 )
        {
            Done = NU_TRUE;
        }

        if( !Done )
        {
            for( ; height > 0; height-- )
            {
                scanListPtr->X = startX;
                scanListPtr += 2;
            }
            /* update pointer for next time */
            *scanListPtrPtr = scanListPtr;  
            Done = NU_TRUE;
        }
    } /* if( (deltaX = endX - startX) == 0 ) */

    if( !Done )
    {
        if( deltaX < 0 )
        {
            /* move left as we draw */
            errorTerm = -height;
            advanceAmt = -1;

            /* width = abs(deltaX) */
            deltaX = -deltaX;   
        }
        else
        {
            errorTerm = -1;
            advanceAmt = 1;
        }

        /* Figure out whether the edge is diagonal, X-major (more horizontal),
           or Y-major (more vertical) and handle appropriately.*/
        if( deltaX == height )
        {
            /* it's a diagonal edge--special case */
            if( skipFirst == 1 )
            {
                /* skip the first point */
                startX += advanceAmt;
                height--;
            }
            while( height > 0 )
            {
                scanListPtr->X = startX;
                scanListPtr += 2;
                startX += advanceAmt;
                height--;
            }
            /* update pointer for next time */
            *scanListPtrPtr = scanListPtr;  
            Done = NU_TRUE;
        }

    } /* if( !Done ) */

    if( !Done )
    {
        if( deltaX > height )
        {
            /* it's an X-major (more horz) edge */
            i = height;
            advanceX = deltaX / height;
            advanceErrorTerm = deltaX % height;
            if( advanceAmt < 0 )
            {
                advanceX = -advanceX;
            }
            if( skipFirst == 1 )
            {
                /* skip the first point */
                startX += advanceX;
                errorTerm += advanceErrorTerm;
                if( errorTerm >= 0 )
                {
                    /* time for X coord to advance one extra */
                    startX += advanceAmt;
                    errorTerm -= height;
                }
                i--;
            }
            while( i > 0 )
            {
                scanListPtr->X = startX;
                scanListPtr += 2;
                startX += advanceX;
                errorTerm += advanceErrorTerm;
                if( errorTerm >= 0 )
                {
                    /* time for X coord to advance one extra */
                    startX += advanceAmt;
                    errorTerm -= height;
                }
                i--;
            }

            /* update pointer for next time */
            *scanListPtrPtr = scanListPtr;  

            Done = NU_TRUE;
        } /* if( deltaX > height ) */

    } /* if( !Done ) */

    if( !Done )
    {
        /* it's a Y-major (more vertical) edge */
        i = height;
        if( skipFirst == 1 )
        {
            /* skip the first point */
            errorTerm += deltaX;
            if( errorTerm >= 0 )
            {
                /* time for X coord to advance */
                startX += advanceAmt;
                errorTerm -= height;
            }
            i--;
        }
        while( i > 0 )
        {
            scanListPtr->X = startX;
            scanListPtr += 2;
            errorTerm += deltaX;
            if( errorTerm >= 0 )
            {
                /* time for X coord to advance */
                startX += advanceAmt;
                errorTerm -= height;
            }
            i--;
        }

        /* update pointer for next time */
        *scanListPtrPtr = scanListPtr; 

    } /* if( !Done ) */

}

/***************************************************************************
* FUNCTION
*
*    POLYGONS_rsScanEdgeList
*
* DESCRIPTION
*
*    Function POLYGONS_rsScanEdgeList scans out an edge list when there's 
*    too little memory for the GET.
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
static VOID POLYGONS_rsScanEdgeList(VOID)
{
    INT16     Done         = NU_FALSE;
    INT16     JumpAddEdges;
    INT16     grafErrValue;
    point     *saveVertexListPtr;
    INT32     saveLclNpointsparm;
    UINT8     *saveBuf1Ptr;
    lineEdgeV *AETPtr;
    lineEdgeV **tmpAETPtrPtr;
    lineEdgeV **newAETPtrPtr;
    lineEdgeV *tmpHighWater;

    /* point to bottom possible line; this can never
       be the starting line for an active edge */
    currentY = 0x7fff;  

    /* top possible line */
    maxY = 0x8000;      

    /* save the following for later use since the GET is built twice */
    saveLclNpointsparm = lclNpointsparm;
    saveVertexListPtr  = vertexListPtr;
    saveBuf1Ptr        = buf1Ptr;

    /* routine called to check each edge
       for the highest endpoint in the polyline */
    addEdgeVector = FindHighestEdge;   

    /* scan for the highest endpoint */
    updateAETVector();                  
    if( currentY > maxY )
    {
        /* skip if there are no edges to do */
        Done = NU_TRUE; 
    }

    if( !Done )
    {
        /* point to AETPtr */
        AETPtrPtr = ptrToGETPtr;

        /* now set up the fill record according to the settings in the current port */
        lclFillRcd = (blitRcd *) (( (SIGNED) AETPtrPtr) + SIZEOFF );

        /* Init local blitRcd from grafBlit */
        *lclFillRcd = grafBlit;                 
        lclFillRcd->blitList = (SIGNED) (lclFillRcd + 1);

        /* calculate the last address at which a rect can start in the blitRcd
           without being the last rect that can fit in the blitRcd */
        highWater = (rect *) ( ((SIGNED) AETPtrPtr) + bufCntr - 2 * sizeof(rect) );

        /* no rects yet */
        rectCount = 0;                              

        /* point to the first rect location */
        rectBase  = (rect *) lclFillRcd->blitList;  
        rectPtr   = rectBase;

        /* AETPtr = NULL */
        *AETPtrPtr = 0; 

        /* mark that the first line can't be the same as the preceding line */
        sameAsLast = 0; 

        /* add the first edges and go */
        JumpAddEdges = NU_TRUE;  

        do
        {
            if( JumpAddEdges == 0 )
            {
                /* Advance each edge in the AET by one scan line. Remove edges that
                   have been fully scanned compressing the AET so that as much room as
                   possible is always available for rects in the blitRcd. (Used only
                   for low-memory case.) */
                newAETPtrPtr = AETPtrPtr;

                /* skip if nothing in AET */
                while( (AETPtr = *newAETPtrPtr) != 0 )   
                {
                    /* decrement the y counter */
                    AETPtr->Count--;

                    /* check if done with edge */
                    if( AETPtr->Count == 0 )
                    {
                        /* Yes, remove it from the AET. If it's not the edge
                           nearest the high water mark, copy the edge nearest the
                           high water mark into the hole and relink it; then move
                           the high water mark up one edge */
                        sameAsLast = 0;
                        *newAETPtrPtr = AETPtr->NextEdge;
                        tmpAETPtrPtr = newAETPtrPtr;

                        /* now, if there's an edge at the high water mark, move
                           it into the hole we just made */
                        /* point to the lowest address at which an edge could start */
                        tmpHighWater = (lineEdgeV *) ((SIGNED) (highWater + 2));

                        /* was the edge we just removed */
                        if( AETPtr != tmpHighWater )
                        {
                            /* the one at the high water mark? */
                            /* no need to check for NULL
                               pointer; if there was only one edge in the AET and
                               we just removed it, it would have been at the high
                               water mark and we'd never get here */
                            tmpAETPtr = *AETPtrPtr; 
                            newAETPtrPtr = AETPtrPtr;
                            while( tmpAETPtr != tmpHighWater )
                            {
                                newAETPtrPtr = &tmpAETPtr->NextEdge;
                                tmpAETPtr = *newAETPtrPtr;
                            }

                            /* was the last edge the one we're moving? */
                            if( tmpAETPtr == (lineEdgeV *) tmpAETPtrPtr )
                            {
                                /* remember the edge's new location */
                                tmpAETPtrPtr = (lineEdgeV **) AETPtr;
                            }

                            /* relink the edge at its new location */
                            *newAETPtrPtr = AETPtr;
                            newAETPtrPtr = (lineEdgeV **) AETPtr;

                            /* Set NotUsed to remove warnings */
                            NU_UNUSED_PARAM(newAETPtrPtr);

                            /* move the data */
                            *AETPtr = *tmpAETPtr;
                        }

                        /* adjust the high water mark for the extra room we just
                           made available */
                        highWater = (rect *) ( ((SIGNED) highWater) + sizeof(lineEdgeV) );
                        newAETPtrPtr = tmpAETPtrPtr;
                    }
                    else
                    {
                        /* count off one scan line for this edge */
                        if( AETPtr->WholePixelXMoveV != 0 )
                        {
                            /* advance the edge's X coordinate by the minimum
                               # of pixels */
                            sameAsLast = 0;
                            AETPtr->CurrentX += AETPtr->WholePixelXMoveV;
                        }

                        AETPtr->ErrorTermV += AETPtr->ErrorTermAdjUpV;
                        if( AETPtr->ErrorTermV > 0 )
                        {
                            /* the error term turned over, so move X one more */
                            sameAsLast = 0;
                            AETPtr->CurrentX +=  AETPtr->XDirection;
                            AETPtr->ErrorTermV -= AETPtr->ErrorTermAdjDownV;
                        }

                        newAETPtrPtr = &AETPtr->NextEdge;
                    }
                }

                /* X sort the AET */
                XSortAET(AETPtrPtr);    
                numLinesUntilNext--;
            } /* JumpAddEdges == 0 ) */

            /* time to add another edge to the AET? */
            if( numLinesUntilNext == 0 || JumpAddEdges )
            {
                /* jump inside this loop the first time */
                JumpAddEdges = NU_FALSE;

                /* max value so will always be replaced
                   by correct value if there is a next edge to start; otherwise,
                   counts down for the remainder of this fill (no more edges
                   to start) */
                numLinesUntilNext = 0x7fff; 

                /* restore the previously saved data for the next GET build */
                lclNpointsparm = saveLclNpointsparm;
                vertexListPtr  = saveVertexListPtr;
                buf1Ptr        = saveBuf1Ptr;

                /* routine called to add each edge to the AET */
                addEdgeVector  = AddEdgeToAET;  

                /* add to the AET all edges that start */
                if( updateAETVector() == 0 )
                {
                    /* at the current scan line, and set new numLinesUntilNext */

                    /* there isn't even enough memory to hold the AET */
                    grafErrValue = c_PolyLine +  c_EdgeOflo;
                    nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                    Done = NU_TRUE;
                }

            } /* if( numLinesUntilNext == 0 || JumpAddEdges ) */

            if (!Done)
            {
                if( (AETPtr = *AETPtrPtr) != 0 )
                {
                    /* do winding rule scan out for this scan line */
                    ScanOutAET(AETPtr); 
                }

                /* continue if we haven't done the bottom line yet */
                currentY++; 
            }

        } while( (currentY <= maxY) && (!Done) );

    } /* if( !Done ) */

    if( !Done )
    {
        /* do any rects remain? */
        if( rectCount != 0 )
        {
            /* yes, do the remaining rects */
            /* set the count field */
            lclFillRcd->blitCnt = rectCount;            

            /* draw the rectangles */
            lclFillRcd->blitDmap->prFill(lclFillRcd);   
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    FindHighestEdge
*
* DESCRIPTION
*
*    Function FindHighestEdge keeps track of the highest and lowest
*    endpoints encountered, accounting for clipping.
*
*    Always returns a 1.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT16 - Returns 1.
*
***************************************************************************/
static INT16 FindHighestEdge(VOID)
{
    INT16 Done = NU_FALSE;
    INT32 tmpY;

    /* 0-height? */
    if( addStartY == addEndY )
    {
        Done = NU_TRUE;
    }

    if( !Done )
    {
        if( addStartY >  addEndY )
        {
            /* exchange endpoint's Y */
            tmpY = addStartY;
            addStartY = addEndY;
            addEndY = tmpY;
        }

        /* trivially clip-reject edges */
        if( (addStartY >= clpR.Ymax) || (addEndY <= clpR.Ymin) )
        {
            Done = NU_TRUE;
        }

    } /* if( !Done ) */

    if( !Done )
    {
        if( clpR.Ymin > addStartY )
        {
            addStartY = clpR.Ymin;
        }
        if( currentY  > addStartY )
        {
            currentY = addStartY;
        }
        if( clpR.Ymax < addEndY )
        {
            addEndY = clpR.Ymax;
        }

        if( maxY < addEndY )
        {
            maxY = addEndY;
        }

    } /* if( !Done ) */

    return(1);
}

/***************************************************************************
* FUNCTION
*
*    AddEdgeToAET
*
* DESCRIPTION
*
*    Function AddEdgeToAET adds the edge to the AET if it starts at currentY.
*    Returns 1 if a success and 0 if out of memory. 
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT16 -Returns 1 if a success and 0 if out of memory. 
*
***************************************************************************/
static INT16 AddEdgeToAET(VOID)
{
    INT16     Done          = NU_FALSE;
    INT16     value;
    INT16     JumpDoAddEdge = NU_FALSE;
    INT32     tmpDeltaX;
    INT32     tmpDeltaY;
    INT32     tmpXY;
    lineEdgeV *AETPtr;
    lineEdgeV **AETNextEdgePtr;
    lineEdgeV *tmpHighWater;

    /* 0-height? */
    if( addStartY == addEndY )
    {
        value = 1;
        Done  = NU_TRUE;
    }

    if( !Done )
    {
        /* which end is higher? */
        if( addStartY  < addEndY )
        {
            /* correct order */
            if( addStartY == currentY )
            {
                JumpDoAddEdge = NU_TRUE;
            }

            /* check if clipped edge will start here */
            if(    (addStartY  < currentY)
                && (clpR.Ymin == currentY)
                && (addEndY    > currentY) )
            {
                JumpDoAddEdge = NU_TRUE;
            }

            if( !JumpDoAddEdge )
            {
                /* how far from currentY to this edge? */
                tmpDeltaY = addStartY - currentY;

                /* ignore if edge is above */
                if( tmpDeltaY < 0 )
                {
                    Done  = NU_TRUE;
                }

                if( !Done )
                {
                    /* is this a new closest edge? */
                    if( tmpDeltaY < numLinesUntilNext )
                    {
                        numLinesUntilNext = tmpDeltaY;
                    }
                }

                value = 1;
                Done  = NU_TRUE;
            }

        } /* if( addStartY  < addEndY ) */
        else
        {
            /* start is greater than end so check end - start */
            if( addEndY == currentY )
            {
                JumpDoAddEdge = NU_TRUE;
            }
            /* check if clipped edge will start here */
            if(    (addEndY    < currentY)
                && (clpR.Ymin == currentY)
                && (addStartY  > currentY) )
            {
                JumpDoAddEdge = NU_TRUE;
            }

            if( !JumpDoAddEdge )
            {
                /* how far from currentY to this edge? */
                tmpDeltaY = addEndY - currentY;

                /* ignore if edge is above */
                if( tmpDeltaY < 0 )
                {
                    Done  = NU_TRUE;
                }
        
                if( !Done )
                {
                    /* is this a new closest edge? */
                    if( tmpDeltaY < numLinesUntilNext )
                    {
                        numLinesUntilNext = tmpDeltaY;
                    }
                }

                value = 1;
                Done  = NU_TRUE;

            } /* if( !JumpDoAddEdge ) */

        } /* else */

    } /* if( !Done ) */

    if( !Done )
    {
        /* add the edge */

        /* new high water mark plus room for one rect */
        tmpHighWater = ((lineEdgeV *) ((SIGNED) (highWater + 1))) - 1;

        /* is there currently room for this edge? */
        if( (SIGNED) tmpHighWater < (SIGNED) rectPtr )
        {
            /* no, clear the rects and try again */
            if( rectCount == 0 )
            {
                /* no room */
                value = 0; 
                Done  = NU_TRUE;
            }
        
            if( !Done )
            {
                /* set the count field */
                lclFillRcd->blitCnt = rectCount;

                /* draw the rectangles */
                lclFillRcd->blitDmap->prFill(lclFillRcd);

                /* reset the # of rects in the rect list to 0 */
                rectCount = 0;      

                /* the next rect goes at the start of the rect list */
                rectPtr = rectBase; 

                /* mark that this line can't be treated as the same
                   as the last line, because the last line's rectangles
                   aren't around any more */
                sameAsLast = 0;     

                if( (SIGNED) tmpHighWater < (SIGNED) rectPtr)
                {
                    /* still no room? */
                    value = 1; 
                    Done  = NU_TRUE;
                }

            } /* if( !Done ) */

        } /* if( (SIGNED) tmpHighWater < (SIGNED) rectPtr ) */

    } /* if( !Done ) */
    
    if( !Done )
    {
        /* yes, point to the new high water mark (last address at which
           there's room for two more rects) */
        highWater = ((rect *) ((SIGNED) tmpHighWater)) - 1;
        tmpHighWater = (lineEdgeV *) ((SIGNED) (highWater + 2));

        /* mark that this line isn't the same as the preceding
           line (because we are about to add an edge) */
        sameAsLast = 0; 

        /* make sure the edge runs top->bottom */
        if( addStartY < addEndY )
        {
            /* yes */
            tmpHighWater->TopToBottom = 1;
        }
        else
        {
            /* no, swap endpoints */
            tmpHighWater->TopToBottom = (signed char)-1;
            tmpXY     = addStartX;
            addStartX = addEndX;
            addEndX   = tmpXY;
            tmpXY     = addStartY;
            addStartY = addEndY;
            addEndY   = tmpXY;
        }

        tmpHighWater->CurrentX = addStartX;
        tmpHighWater->StartY   = addStartY;

        tmpDeltaY = addEndY - addStartY;

        tmpHighWater->Count = tmpDeltaY;
        tmpHighWater->ErrorTermAdjDownV = tmpDeltaY;

        tmpDeltaX = addEndX - addStartX;

        /* check X direction */
        if( tmpDeltaX >= 0 )
        {
            /* left->right */
            /* direction in which X moves */
            tmpHighWater->XDirection = 1;   

            /* initial error term */
            tmpHighWater->ErrorTermV = -1;  
        }
        else
        {
            /* right->left */
            /* abs(tmpDeltaX) */
            tmpDeltaX = -tmpDeltaX; 
            tmpHighWater->XDirection = (signed char)-1;
            tmpHighWater->ErrorTermV = -tmpDeltaY;
        }

        /* X major or Y major? */
        if( tmpDeltaY >= tmpDeltaX )
        {
            /* Y major */
            tmpHighWater->WholePixelXMoveV = 0;
            tmpHighWater->ErrorTermAdjUpV  = tmpDeltaX;
        }
        else
        {
            /* X major */
            tmpHighWater->ErrorTermAdjUpV = tmpDeltaX % tmpDeltaY;
            if( tmpHighWater->XDirection == 1 )
            {
                tmpHighWater->WholePixelXMoveV = tmpDeltaX / tmpDeltaY;
            }
            else
            {
                tmpHighWater->WholePixelXMoveV = - tmpDeltaX / tmpDeltaY;
            }
        }

        /* does this edge start above top clip? */
        if( tmpHighWater->StartY < clpR.Ymin )
        {
            /* yes, jump it ahead */
            /* clip count */
            tmpHighWater->Count -= (clpR.Ymin - tmpHighWater->StartY); 

            /* new start Y (at top clip) */
            tmpHighWater->StartY = clpR.Ymin;                          

            /* adjust error term for # of times we would have adjusted it in
               advancing that many points */
            tmpHighWater->ErrorTermV += (tmpHighWater->Count * tmpHighWater->ErrorTermAdjUpV);

            /* check if it turned over */
            if( tmpHighWater->ErrorTermV >= 0 )
            {
                /* yes it did so correct it */
                /* which direction? */
                if( tmpHighWater->XDirection == 1 )
                {
                    /* left->right */
                    tmpHighWater->CurrentX += ((tmpHighWater->ErrorTermV / 
                                        tmpHighWater->ErrorTermAdjDownV) + 1);
                }
                else
                {
                    /* right to left */
                    tmpHighWater->CurrentX -= ((tmpHighWater->ErrorTermV / 
                                            tmpHighWater->ErrorTermAdjDownV) + 1);
                }

                tmpHighWater->ErrorTermV = ( (tmpHighWater->ErrorTermV % 
                                      tmpHighWater->ErrorTermAdjDownV)
                                    - tmpHighWater->ErrorTermAdjDownV);
            } /* if( tmpHighWater->ErrorTermV >= 0 ) */

            tmpHighWater->CurrentX +=  (tmpDeltaX * tmpHighWater->WholePixelXMoveV);
        } /* if( tmpHighWater->StartY < clpR.Ymin ) */

        /* finally, link the new edge in so that the edge list is still sorted
           by X coordinate */
        AETNextEdgePtr = AETPtrPtr;
        while( ((AETPtr = *AETNextEdgePtr) != 0) && (!Done))
        {
            if( AETPtr->CurrentX >= tmpHighWater->CurrentX )
            {
                Done = NU_TRUE;
            }
            else
            {
                AETNextEdgePtr = &AETPtr->NextEdge;
            }
            
        }

        /* link in the edge before the next edge we just reached */
        tmpHighWater->NextEdge = AETPtr;
        *AETNextEdgePtr = tmpHighWater;

        value = 1;
    } /* if( !Done ) */

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    POLYGONS_rsSuperSetPolyLineDrawer
*
* DESCRIPTION
*
*    Function POLYGONS_rsSuperSetPolyLineDrawer is the superset polyline drawer. If the
*    memory pool is at least as large as STACK_BUFFER_SIZE, the edge tables
*    are maintained there; otherwise, the edge tables are maintained in a buffer
*    on the stack. If there's not enough memory to maintain the edge lists,
*    then the point list must be re-scanned frequently, and that's much slower.
*    Therefore, it behooves the caller to make as much memory available in the
*    memory pool as possible.
*
*    Wide lines are filled as polygons.
*
*    pnLoc is updated to the last point connected to.
*
*    Note: Important restriction! Currently assumes the pen is square and that
*    lines are wide!
*
* INPUTS
*
*    point *pointsparm - Pointer to array of points that defines the polyline.
*                        If the first and last points aren't the same, they're
*                        only joined if npointsparm < 0. If the first and last
*                        points are the same, they're drawn properly--just once.
*                        For wide lines, points where lines intersect are drawn just once;
*                        for thin lines, only common endpoints are taken care of,
*                        and intersection points are drawn multiple times.
*
*    INT32 npointsparm - # of elements in the points array. If < 0, then the absolute
*                        value is the # of elements, and the first and last points
*                        should be joined.
*
*    INT32 modeparm    - coordModeOrigin (coordinates are absolute) or coordModePrevious
*                        (coordinates after the first are relative to the preceding one).
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID POLYGONS_rsSuperSetPolyLineDrawer( point *pointsparm, INT32 npointsparm, INT32 modeparm)
{
    INT16 Done = NU_FALSE;

    INT32 i;

    /* nothing to do if no points */
    if( npointsparm == 0 )
    {
        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* local copy of modeparm */
        lclModeparm = modeparm;

        if( theGrafPort.pnLevel < 0 )
        {
            /* set the new pen location and done. */
            /* point to the first vertex (which is always absolute, even in
               previous mode) */
            if( npointsparm > 0 )
            {
                /* count is positive, so load the last point */
                /* previous mode? */
                if( modeparm == coordModePrevious )
                {
                    /* yes, must add our way through the list to the last point */
                    /* the first point is absolute, */
                    finalRawX = pointsparm->X;  

                    /* even in previous mode */
                    finalRawY = pointsparm->Y;  
                    pointsparm++;
                    i = 1;
                    while( i < npointsparm )
                    {
                        finalRawX += pointsparm->X;
                        finalRawY = pointsparm->Y;
                        i++;
                        pointsparm++;
                    }
                }
                else
                {
                    /* no, so the last point is absolute so point to the last */
                    /* point in the list */
                    pointsparm += npointsparm - 1;  
                    finalRawX = pointsparm->X;
                    finalRawY = pointsparm->Y;
                }
            }
            else
            {
                /* get first point */
                finalRawX = pointsparm->X;
                finalRawY = pointsparm->Y;
            }

            /* already in global? */
            if( globalLevel > 0 )
            {
                /* no */
                /* convert to global */
                U2GP(finalRawX, finalRawY, &finalGblX, &finalGblY, 1);
            }
            else
            {
                /* yes */
                finalGblX = finalRawX;
                finalGblY = finalRawY;
            }

            /* update global pen location */
            LocX = finalGblX;
            LocY = finalGblY;

            /* update user pen location */
            theGrafPort.pnLoc.X = finalRawX;
            theGrafPort.pnLoc.Y = finalRawY;
            thePort->pnLoc.X = finalRawX;
            thePort->pnLoc.Y = finalRawY;
            Done = NU_TRUE;
        }
    } /* if( !Done ) */

    if( !Done )
    {
        /* copy clip rectangle over */
        clpR = ViewClip;

        /* mark whether to connect first & last */
        if( npointsparm < 0 )
        {
            /* do connect first & last */
            finalWrap = 1;                  

            /* abs(npointsparm) */
            npointsparm = - npointsparm;    
        }
        else
        {
            /* don't connect first & last */
            finalWrap = 0;  
        }

        /* set the pen dimensions */
        /* round odd down */
        halfWidth[0] = - (theGrafPort.pnSize.X >> 1);      

        /* round odd up */
        halfWidth[1] = ((theGrafPort.pnSize.X + 1) >> 1);  

        /* round odd down */
        halfWidth[2] = - (theGrafPort.pnSize.Y >> 1);      

        /* round odd up */
        halfWidth[3] = ((theGrafPort.pnSize.Y + 1) >> 1);  

        /* decide which buffer to use, stack or memory pool */
        /* assume using general memory pool */
        buf1Ptr = mpWorkSpace;  
        bufCntr = (SIGNED) (mpWorkEnd - buf1Ptr);

#if     (WORKSPACE_BUFFER_SIZE < STACK_BUFFER_SIZE) 
        
        /* is the memory pool smaller than the stack buffer? */
        if( bufCntr <= STACK_BUFFER_SIZE )
        {                                
            /* set up to use the stack buffer */
            bufCntr = STACK_BUFFER_SIZE;
            buf1Ptr = &buf1[0];
        }

#endif  /* (WORKSPACE_BUFFER_SIZE < STACK_BUFFER_SIZE) */
        
        /* must leave room for at least one rect and a blitRcd - point to last
           byte at which an edge can start without overflowing the buffer */
        endAvailForGET = ( (SIGNED) buf1Ptr) + bufCntr
                            - sizeof(blitRcd)
                            - sizeof(rect)
                            - sizeof(lineEdge);
    
        /* remember where the GET pointer is */
        ptrToGETPtr = (lineEdgeV **) (SIGNED) buf1Ptr;

        /* set the GET to empty to start */
        *ptrToGETPtr = 0;               

        /* point past the GET and AET pointers */
        buf1Ptr += SIZEOFF * 2;         

        /* routine called to add each edge to the GET */
        addEdgeVector = AddEdgeToGET;  

        /* scan through the vertex list and put all non-0-height edges into the
           global edge table, sorted by increasing Y start coordinate */
        /* for counting down */
        lclNpointsparm = npointsparm;   

        /* remember where the first vertex is */
        vertexListPtr = pointsparm; 
        /* try to build the GET from the point list (success depends on whether
           enough memory is available) */
        if (BuildSquareWidePenPolyLineGET() == 1 )
        {
            /* scan out the global edge table */
            /* # of bytes left for blitRcd */
            bufCntr -= ((SIGNED) buf1Ptr - (SIGNED) ptrToGETPtr); 
            EDGES_rsScansAndFillsEdgeList( (VOID **)ptrToGETPtr, (blitRcd *)buf1Ptr, bufCntr, cmplx, 1, 1);
        }
        else
        {
            /* There's not enough memory for the GET, two pointers, a blitRcd,
               and at least one rect, so we'll have to scan the whole vertex list
               each time to maintain the AET. */
            /* reset the counter */
            lclNpointsparm = npointsparm;   
            vertexListPtr = pointsparm;

            /* vector used to update the AET by scanning vertices */
            updateAETVector = BuildSquareWidePenPolyLineGET; 

            /* set winding rule fill for ScanOutAET */
            lclFillRule = 1; 

            /* scan out the polygon */
            POLYGONS_rsScanEdgeList();   
        }

        /* update global pen location */
        LocX = finalGblX;
        LocY = finalGblY;

        /* update user pen location */
        theGrafPort.pnLoc.X = finalRawX;
        theGrafPort.pnLoc.Y = finalRawY;
        thePort->pnLoc.X = finalRawX;
        thePort->pnLoc.Y = finalRawY;

    } /* if( !Done ) */
    
}

/***************************************************************************
* FUNCTION
*
*    BuildSquareWidePenPolyLineGET
*
* DESCRIPTION
*
*    Function BuildSquareWidePenPolyLineGET builds a global edge table from
*    the point list for a square, wide pen. The GET is a linked list,  sorted
*    first by Y coordinate, then by X coordinate.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns 1 for success, 0 for failure (insufficient memory).
*
***************************************************************************/
static INT16 BuildSquareWidePenPolyLineGET(VOID)
{
    INT16 Done                   = NU_FALSE;
    INT16 finalWrapFlag          = NU_FALSE;
    INT16 JumpPrepareForNext     = NU_FALSE;
    INT16 JumpNormalConnectEdges = NU_FALSE;
    INT16 value = 1;

/* For various directions, offset directions to corners of square pen at
   which the edges parallel to the ideal line start and end. */
UINT8 StartEndSetTable[8][8] = {
    {PEN_LEFT,   PEN_DOWN,   PEN_LEFT,   PEN_UP,     /* Dir0StartEnd */
     PEN_RIGHT,  PEN_UP,     PEN_RIGHT,  PEN_DOWN},
    {PEN_LEFT,   PEN_UP,     PEN_LEFT,   PEN_UP,     /* Dir1StartEnd */
     PEN_RIGHT,  PEN_DOWN,   PEN_RIGHT,  PEN_DOWN},
    {PEN_LEFT,   PEN_UP,     PEN_RIGHT,  PEN_UP,     /* Dir2StartEnd */
     PEN_RIGHT,  PEN_DOWN,   PEN_LEFT,   PEN_DOWN},
    {PEN_RIGHT,  PEN_UP,     PEN_RIGHT,  PEN_UP,     /* Dir3StartEnd */
     PEN_LEFT,   PEN_DOWN,   PEN_LEFT,   PEN_DOWN},
    {PEN_RIGHT,  PEN_UP,     PEN_RIGHT,  PEN_DOWN,   /* Dir4StartEnd */
     PEN_LEFT,   PEN_DOWN,   PEN_LEFT,   PEN_UP},
    {PEN_RIGHT,  PEN_DOWN,   PEN_RIGHT,  PEN_DOWN,   /* Dir5StartEnd */
     PEN_LEFT,   PEN_UP,     PEN_LEFT,   PEN_UP},
    {PEN_RIGHT,  PEN_DOWN,   PEN_LEFT,   PEN_DOWN,   /* Dir6StartEnd */
     PEN_LEFT,   PEN_UP,     PEN_RIGHT,  PEN_UP},
    {PEN_LEFT,   PEN_DOWN,   PEN_LEFT,   PEN_DOWN,   /* Dir7StartEnd */
     PEN_RIGHT,  PEN_UP,     PEN_RIGHT,  PEN_UP}};

    INT32 xDirection;
    INT32 yDirection;
    INT32 directionDifference;
    
    /* local copy of vertexListPtr for wrap back */
    point *vrtxListPtr; 


    /* remember this X,Y for next time */
    rawLastX = vertexListPtr->X;
    rawLastY = vertexListPtr->Y;
    vrtxListPtr = vertexListPtr;

    /* already in global? */
    if( globalLevel > 0 )
    {
        /* no, convert to global */
        U2GP(rawLastX, rawLastY, &startX, &startY, 1);
    }
    else
    {
        startX = rawLastX;
        startY = rawLastY;
    }

    /* mark that this is the first line segment */
    firstSegment = 1;   

    do
    {
        /* is this the last vertex? */
        if( lclNpointsparm == 1 )
        {
            /* yes, so potentially wrap back to the first point. Also
               remember where the last point of the polyline is, in both
               user and global coords, for advancing the pen location at
               the end. This point is always reached at least once, even
               when polyline drawing fails, and isn't reached until the
               last point */

            /* do we wrap back from the end to the start? */
            if( finalWrap != 1 )
            {
                /* no, so we're done */
                /* remember where the pen */
                finalRawX = rawLastX;   

                /* location advances to when */
                finalRawY = rawLastY;   

                /* we're done */
                finalGblX = startX;     
                finalGblY = startY;
                
                Done = NU_TRUE;
                finalWrapFlag = NU_TRUE;
            }
            else
            {
                /* wrap back to the start of the vertex list */
                /* load the endpoint of this segment and convert it */
                /* to global coords */
                finalRawX = vertexListPtr->X;
                finalRawY = vertexListPtr->Y;

                /* already in global? */
                if( globalLevel > 0 )
                {
                    /* no, convert to global */
                    U2GP(finalRawX, finalRawY, &finalGblX, &finalGblY, 1);
                }
                else
                {
                    /* yes */
                    finalGblX = finalRawX;
                    finalGblY = finalRawY;
                }
                endX = finalGblX;
                endY = finalGblY;
            }
            
        }
        else
        {
            /* point to next vertex */
            vrtxListPtr++;

            /* previous mode? */
            if( lclModeparm == coordModePrevious )
            {
                /* yes, calculate new position as a delta
                   from the last X,Y in user coordinates */
                rawLastX += vrtxListPtr->X;
                rawLastY += vrtxListPtr->Y;
            }
            else
            {
                rawLastX = vrtxListPtr->X;
                rawLastY = vrtxListPtr->Y;
            }

            /* already in global? */
            if( globalLevel > 0 )
            {
                /* no, convert to global */
                U2GP(rawLastX, rawLastY, &endX, &endY, 1);
            }
            else
            {
                /* yes */
                endX = rawLastX;
                endY = rawLastY;
            }
        }

        if ( !Done )
        {
            xDirection = endX - startX;
            yDirection = endY - startY;

            /* vertical direction ? */
            if (xDirection == 0)
            {
                /* vertical direction */
                if( yDirection == 0 )
                {
                    /* no movement */
                    continue;       
                }
                if( yDirection < 0 )
                {
                    /* straight up */
                    direction = 0;  
                }
                else 
                {
                    /* straight down */
                    direction = 4;  
                }
            }
            else
            {
                /* moving left ? */
                if( xDirection < 0 )
                {
                    /* moving left */
                    if( yDirection == 0 )
                    {
                        /* straight left */
                        direction = 6; 
                    }
                    if( yDirection < 0 )
                    {
                        /* up and to the left */
                        direction = 7;  
                    }
                    else 
                    {
                        /* down and to the left */
                        direction = 5;  
                    }
                }
                else
                {
                    if( yDirection == 0 )
                    {
                        /* straight right */
                        direction = 2;  
                    }
                    if( yDirection < 0 )
                    {
                        /* up and to the right */
                        direction = 1;  
                    }
                    else 
                    {
                        /* down and to the right */
                        direction = 3;  
                    }
                }
            }

            /* build the two edges that are parallel to the ideal line */
            /* start->end edge start X */
            startOutX = startX + halfWidth[StartEndSetTable[direction][0]];

            /* start->end edge start Y */
            startOutY = startY + halfWidth[StartEndSetTable[direction][1]];
 
            /* start->end edge end X */
            endInX = endX + halfWidth[StartEndSetTable[direction][2]];

            /* start->end edge end Y */
            endInY = endY + halfWidth[StartEndSetTable[direction][3]];

            /* add start->end edge  and check if enough memory */
            addStartX = startOutX;
            addStartY = startOutY;
            addEndX   = endInX;
            addEndY   = endInY;

            if( addEdgeVector() != 1 )
            {
                /* fail if return */
                value = 0; 
                Done  = NU_TRUE;
            }

            if (!Done)
            {
                /* end->start edge start X */
                startInX = startX + halfWidth[StartEndSetTable[direction][6]];

                /* end->start edge start Y */
                startInY = startY + halfWidth[StartEndSetTable[direction][7]];

                /* end->start edge end X */
                endOutX = endX + halfWidth[StartEndSetTable[direction][4]];

                /* end->start edge end Y */
                endOutY = endY + halfWidth[StartEndSetTable[direction][5]];

                /* add end->start edge  and check if enough memory */
                addStartX = endOutX;
                addStartY = endOutY;
                addEndX = startInX;
                addEndY = startInY;

                if( addEdgeVector() != 1 )
                {
                    /* fail if return */
                    value = 0; 
                    Done  = NU_TRUE;
                }
            }

            /* is this the first line segment? */
            if( (firstSegment == 1) && (!Done ))
            {
                /* yes, just add an edge if necessary to close up the first segment */
                /* after this, it's not the first line segment */
                firstSegment = 0;   

                /* close up according to direction */
                if( (direction == 0) || (direction == 4) )
                {
                    JumpPrepareForNext = NU_TRUE;
                }
            
                if( !JumpPrepareForNext )
                {
                    if( direction > 4 )
                    {
                        /* direction 5-7 -- add a right edge */
                        addStartX = startX + halfWidthRight;
                        addStartY = startY + halfHeightUp;
                        addEndX   = addStartX;
                        addEndY   = startY + halfHeightDown;

                        if( addEdgeVector() != 1 )
                        {
                            /* fail if return */
                            value = 0; 
                            Done  = NU_TRUE;
                        }
                    }
                    else
                    {
                        /* direction 1-3 -- add a left edge */
                        addStartX = startX + halfWidthLeft;
                        addStartY = startY + halfHeightDown;
                        addEndX   = addStartX;
                        addEndY   = startY + halfHeightUp;
                        if( addEdgeVector() != 1 )
                        {
                            /* fail if return */
                            value = 0; 
                            Done  = NU_TRUE;
                        }
                    }
                } /* if( !JumpPrepareForNext ) */

                if (!Done)
                {
                    JumpPrepareForNext = NU_TRUE;
                }
                

            } /* if( firstSegment == 1 ) */

            if( (!JumpPrepareForNext) && ( !Done) )
            {
                /* Not a first segment so connect the ends of the last and current edges. */
                directionDifference = direction - lastDirection;

                if( directionDifference < 0 )
                {
                    directionDifference = -directionDifference;
                }
                /* are the lines in opposite dirs? */
                if( directionDifference == 4 )
                {
                    /* when the lines are in opposite dirs the far side of the pen
                       must be filled out unless the lines are horizontal or vertical */
                    if( !(lastDirection & 1) )
                    {
                        JumpNormalConnectEdges = NU_TRUE;
                    }

                    if( !JumpNormalConnectEdges )
                    {
                        /* if lastDirection < 4, then add two right pen edges; otherwise,
                           add two left pen edges */
                        if( lastDirection < 4 )
                        {
                            /* direction 1-3 */
                            addStartX = startX + halfWidthRight;
                            addStartY = startY + halfHeightUp;
                            addEndX   = addStartX;
                            addEndY   = startY + halfHeightDown;
                            if( addEdgeVector() != 1 )
                            {
                                /* fail if return */
                                value = 0; 
                                Done  = NU_TRUE;
                            }

                            if ( !Done )
                            {
                                addStartX = startX + halfWidthRight;
                                addStartY = startY + halfHeightUp;
                                addEndX   = addStartX;
                                addEndY   = startY + halfHeightDown;
                                if( addEdgeVector() != 1 )
                                {
                                    /* fail if return */
                                    value = 0; 
                                    Done  = NU_TRUE;
                                }
                            }
                        }
                        else
                        {
                            /* direction 5-7 */
                            addStartX = startX + halfWidthLeft;
                            addStartY = startY + halfHeightDown;
                            addEndX   = addStartX;
                            addEndY   = startY + halfHeightUp;

                            if( addEdgeVector() != 1 )
                            {
                                /* fail if return */
                                value = 0; 
                                Done  = NU_TRUE;
                            }
                                
                            if ( !Done )
                            {
                                addStartX = startX + halfWidthLeft;
                                addStartY = startY + halfHeightDown;
                                addEndX   = addStartX;
                                addEndY   = startY + halfHeightUp;
                                if( addEdgeVector() != 1 )
                                {
                                    /* fail if return */
                                    value = 0; 
                                    Done  = NU_TRUE;
                                }
                            }
                        } /* else */

                        if ( !Done )
                        {
                            JumpPrepareForNext = NU_TRUE;
                        }
                    } /* if( !JumpNormalConnectEdges ) */

                } /* if( directionDifference == 4 ) */

            } /* if( !JumpPrepareForNext ) */
    
            if( !JumpPrepareForNext && !JumpNormalConnectEdges && !Done )
            {
                /* Lines are not opposite so check to see if this is a right angle 
                   unless lines aren't horizontal or vertical. */
                if( ( !(lastDirection & 1)) && ( (directionDifference & 3) == 2) )
                {
                    /* Lines are at a right angle so add a square corner so that
                       for FrameRect, the edges don't change from one scan line to
                       the next.  If the horizontal edge goes right, add a left edge,
                       otherwise add a right edge */
                    if( lastDirection < 4 )
                    {
                        /* direction 1-3 */
                        addStartX = startX + halfWidthRight;
                        addStartY = startY + halfHeightUp;
                        addEndX   = addStartX;
                        addEndY   = startY + halfHeightDown;
                        if( addEdgeVector() != 1 )
                        {
                            /* fail if return */
                            value = 0; 
                            Done  = NU_TRUE;
                        }
                    }
                    else
                    {
                        /* direction 5-7 */
                        addStartX = startX + halfWidthLeft;
                        addStartY = startY + halfHeightDown;
                        addEndX   = addStartX;
                        addEndY   = startY + halfHeightUp;
                        if( addEdgeVector() != 1 )
                        {
                            /* fail if return */
                            value = 0; 
                            Done  = NU_TRUE;
                        }
                    }

                    if ( !Done )
                    {
                        JumpPrepareForNext = NU_TRUE;
                    }
                } /* if( ( !(lastDirection & 1)) && ( (directionDifference & 3) == 2) ) */

            } /* if( !JumpPrepareForNext && !JumpNormalConnectEdges ) ) */


            if( !JumpPrepareForNext && !Done )
            {
                /* Connect lastIn and startOut (if they differ) and startIn and lastOut */
                /* (if they differ). */

                JumpNormalConnectEdges = NU_FALSE;

                if( (lastInX != startOutX) || (lastInY != startOutY) )
                {
                    /* connect lastIn and startOut */
                    addStartX = lastInX;
                    addStartY = lastInY;
                    addEndX   = startOutX;
                    addEndY   = startOutY;
                    if( addEdgeVector() != 1 )
                    {
                        /* fail if return */
                        value = 0; 
                        Done  = NU_TRUE;
                    }
                }

                if( ((startInX != lastOutX) || (startInY != lastOutY)) && !Done )
                {
                    /* connect startIn and lastOut */
                    addStartX = startInX;
                    addStartY = startInY;
                    addEndX   = lastOutX;
                    addEndY   = lastOutY;
                    if( addEdgeVector() != 1 )
                    {
                        /* fail if return */
                        value = 0; 
                        Done  = NU_TRUE;
                    }
                }

            } /* if( !JumpPrepareForNext ) */

            if ( !Done )
            {
                /* prepare for the next line segment */
                JumpPrepareForNext = NU_FALSE;

                startX = endX;
                startY = endY;
                lastDirection = direction;
                lastInX = endInX;
                lastInY = endInY;
                lastOutX = endOutX;
                lastOutY = endOutY;
            }
        }
    } while( (--lclNpointsparm > 0) && (!Done) ); 

    if( finalWrapFlag )
    {
        /* Reset the done flag.  It should be True when we get here */
        Done = NU_FALSE;

        /* are we still on the first line segment? */
        if( firstSegment == 1 )
        {
            /* yes, so draw the pen around the one point */
            /* add an upward edge to the left */
            addStartX = startX + halfWidthLeft; 
            addStartY = startY + halfHeightDown;
            addEndX   = addStartX;
            addEndY   = startY + halfHeightUp;
            if( addEdgeVector() != 1 ) 
            {
                /* fail if return */
                value = 0; 
                Done  = NU_TRUE;
            }

            if( !Done )
            {
                /* add a downward edge to the right */
                addStartX = startX; 
                addStartY = startY;
            }
        }
        else
        {
            /* no, just close up the last segment */
            if( (!(direction & 3)) ) 
            {
                /* nothing to do */
                value = 1; 
                Done  = NU_TRUE;
            }
            
            if( !Done )
            {
                if( direction > 4 )
                {
                    /* direction 5-7 -- add an upward left */
                    addStartX = endX + halfWidthLeft;
                    addStartY = endY + halfHeightDown;
                    addEndX   = addStartX;
                    addEndY   = endY + halfHeightUp;
                    if( addEdgeVector() != 1 ) 
                    {
                        /* fail if return */
                        value = 0; 
                        Done  = NU_TRUE;
                    }
                    else
                    {
                        /* done */
                        value = 1; 
                        Done  = NU_TRUE;
                    }
                }

                if( !Done )
                {
                    /* add a downward edge to the right */
                    addStartX = endX;
                    addStartY = endY;
                }

            } /* if( !Done ) */

        } /* else */

        if( !Done )
        {
            addStartX = addStartX + halfWidthRight;
            addStartY = addStartY + halfHeightUp;
            addEndX   = addStartX;
            addEndY   = startY + halfHeightDown;
            if( addEdgeVector() != 1 ) 
            {
                /* fail if return */
                value = 0; 
            }
        }

    } /* if( !Done ) */

    return(value);
}
