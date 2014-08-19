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
*  edges.c                                                      
*
* DESCRIPTION
*
*  This file contains edge common functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  AddToGETYXSorted
*  ChkAngle
*  EDGES_rsSetupQuadrantArc
*  EDGES_rsSetupBottomQuadrantArc
*  EDGES_rsSetupVerticalLineEdge
*  EDGES_rsSetupStraightLineEdge
*  EDGES_rsScansAndFillsEdgeList
*  XSortAET
*  StepStraightEdge
*  StepVertical
*  StepQATopNativeSize
*  StepQABottomNativeSize
*  ScanOutAET
*  DrawBurst
*
* DEPENDENCIES
*
*  rs_base.h
*  edges.h
*  polygons.h
*  globalrsv.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/edges.h"
#include "ui/polygons.h"
#include "ui/globalrsv.h"
#include "ui/gfxstack.h"

/* pointer to line edge table */
static  lineEdgeV *leGETPtr;        

/* points to rect that started the last scan line */
static  rect *lastScanPtr;    

/* 1 if all edges in GET are lines, 0 if other entities are present */
static  signed char allLineEdges;

/* Functions with local scope. */
static UINT8 StepStraightEdge(lineEdgeV *CurrentEdge);
static UINT8 StepVertical(qarcState *CurrentEdge);
static UINT8 StepQATopNativeSize(qarcState *CurrentEdge);
static UINT8 StepQABottomNativeSize(qarcState *CurrentEdge);
static VOID DrawBurst(VOID);

/***************************************************************************
* FUNCTION
*
*    AddToGETYXSorted
*
* DESCRIPTION
*
*    Function AddToGETYXSorted is used to add edges to the global edge table.
*    edgePtr is the pointer to new edge to add.
*    GETPtr is the pointer to the global edge table.
*
* INPUTS
*
*    qarcState *newEdge - Pointer to the new edge.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID AddToGETYXSorted(qarcState *newEdge)
{
    qarcState **GETPtrPtr;
    qarcState *tempPtr;

    /* address of edge pointer */
    GETPtrPtr = &GETPtr; 
    while(1)
    {
        /* get edge pointer */
        tempPtr = *GETPtrPtr; 

        /* any more in list? */
        if( tempPtr == 0 )
        {
            break;
        }

        /* does current edge have higher Y than new edge? */
        if( tempPtr->StartY > newEdge->StartY)
        {
            break;
        }
    
        /* higher or equal X? */
        if( (tempPtr->StartY == newEdge->StartY) &&
            (tempPtr->CurrentX >= newEdge->CurrentX) )
        {
            break;
        }

        /* point to next edge field for next */
        GETPtrPtr = &tempPtr->NextEdge; 
    }

    /* point current edge to new edge */
    *GETPtrPtr = newEdge;   

    /* point new edge to next edge */
    newEdge->NextEdge = tempPtr;

}

/***************************************************************************
* FUNCTION
*
*    ChkAngle
*
* DESCRIPTION
*
*    Function ChkAngle checks and scales the angle to insure that it is between
*    0 and 360 degrees.
*
* INPUTS
*
*    INT32 rsangle - Angle to check.
*
* OUTPUTS
*
*    INT16 - Scaled angle value.
*
***************************************************************************/
INT16 ChkAngle(INT32 rsangle)
{
    while( rsangle < 0 )
    {
        rsangle += 3600;
    }

    return( (INT16)(rsangle % 3600) );
}

#ifdef FIXPOINT
/***************************************************************************
* FUNCTION
*
*    EDGES_rsSetupQuadrantArc
*
* DESCRIPTION
*
*    Function EDGES_rsSetupQuadrantArc sets up a top quadrant arc edge (a 90-degree arc edge
*    entity, either from 90 to 0 or from 90 to 180).
*
* INPUTS
*
*    qarcState *QArcEdge - Pointer to qarcState to set up.
*
*    INT32 QArcA         - X radius of arc ( >= 0 ).
*
*    INT32 QArcB         - Y radius of arc ( >= 0 ).
*
*    INT32 QArcCenterX   - X center point of arc.
*
*    INT32 QArcCenterY   - Y center point of arc.
*
*    INT32 QArcDirection - Set to +1 if arc goes from 90 to 0.
*                          Set to -1 if arc goes from 90 to 180.
*                          No other values should be used.
*
*    INT32 QArcTToB      - Set to +1 if arc is interpreted as top to bottom.
*                          Set to -1 if bottom to top.
*                          No other values should be used.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsSetupQuadrantArc( qarcState *QArcEdge, INT32 QArcA, INT32 QArcB, INT32 QArcCenterX,
                               INT32 QArcCenterY, INT32 QArcDirection, INT32 QArcTToB)
{
    dblFix dblTmp, dblTmp2;

    /* start Y = center Y - Y radius */
    QArcEdge->StartY = QArcCenterY - QArcB; 

    /* # of scan lines intersected by arc */
    QArcEdge->Count = QArcB + 1;    
    QArcEdge->TopToBottom = QArcTToB;
    QArcEdge->XDirection  = QArcDirection;

    /* X value when Y = 0 */
    QArcEdge->CurrentX    = QArcCenterX;   

    /* rightward arc? */
    if( QArcDirection < 0 )
    {
        /* no, final X is X radius distance to left of center */
        QArcEdge->qaFinalX = QArcCenterX - QArcA;
    }
    else
    {
        /* yes, final X is X radius distance to right of center */
        QArcEdge->qaFinalX = QArcCenterX + QArcA;
    }

    /* zero height? */
    if( QArcB == 0 )
    {
        /* yes, flat line, so go right to the end */
        QArcEdge->CurrentX =QArcEdge->qaFinalX;
    }

    /* is this a vertical line? */
    else if( QArcA == 0 )
    {
        /* stepping routine */
        QArcEdge->StepVector = StepVertical; 
    }
    else
    {
        /* X radius squared */
        QArcEdge->qaASq = QArcA * QArcA;                        

        /* X radius ** 2 * 2 */
        QArcEdge->qaASqX2 = QArcEdge->qaASq + QArcEdge->qaASq;  

        /* Y radius squared */
        QArcEdge->qaBSq = QArcB * QArcB;                        

        /* Y radius ** 2 * 2 */
        QArcEdge->qaBSqX2 = QArcEdge->qaBSq + QArcEdge->qaBSq;  

        /* set up scanning vector */
        QArcEdge->StepVector = StepQATopNativeSize;            

        /* initial xadjust = 0 */
        QArcEdge->qaXAdjust.fUpper = 0;                         
        QArcEdge->qaXAdjust.fLower = 0;

        /* Y adjust = 2*(XRadius**2)*YRadius */
        dblTmp.fUpper  = (QArcB >> 16);
        dblTmp.fLower  = (QArcB << 16);
        dblTmp2.fUpper = (QArcEdge->qaASqX2 >> 16);
        dblTmp2.fLower = (QArcEdge->qaASqX2 << 16);
        dFix_mul(&dblTmp2, &dblTmp, &QArcEdge->qaYAdjust);

        /* initial qaErrTerm = ((Xradius**2 + Yradius**2) / 4) - XRadius**2*YRadius */
        dblTmp.fUpper = (QArcEdge->qaASq + QArcEdge->qaBSq);
        dblTmp.fLower = (dblTmp.fUpper << 15);
        dblTmp.fUpper = (dblTmp.fUpper >> 17);
        dFix_sub(&dblTmp, &QArcEdge->qaYAdjust, &QArcEdge->qaErrTerm);
        QArcEdge->qaErrTerm.fLower = (((UNSIGNED)QArcEdge->qaErrTerm.fUpper) << 31) |
            (QArcEdge->qaErrTerm.fLower >> 1);
        QArcEdge->qaErrTerm.fUpper = (QArcEdge->qaErrTerm.fUpper >> 1);

        /* pre-compensate for adjustments StepQATopNativeSize will do on the initial
           scanning call */
        dFix_add(&QArcEdge->qaErrTerm, &QArcEdge->qaYAdjust, &QArcEdge->qaErrTerm);
        dFix_add(&QArcEdge->qaYAdjust, &dblTmp2, &QArcEdge->qaYAdjust);

        /* scan to the maximum extent on the first scan line of the arc */
        StepQATopNativeSize(QArcEdge);

    } /* else */

}

#else
/***************************************************************************
* FUNCTION
*
*    EDGES_rsSetupQuadrantArc
*
* DESCRIPTION
*
*    Function EDGES_rsSetupQuadrantArc sets up a top quadrant arc edge (a 90-degree arc edge
*    entity, either from 90 to 0 or from 90 to 180).
*
* INPUTS
*
*    qarcState *QArcEdge - Pointer to qarcState to set up.
*
*    INT32 QArcA         - X radius of arc ( >= 0 ).
*
*    INT32 QArcB         - Y radius of arc ( >= 0 ).
*
*    INT32 QArcCenterX   - X center point of arc.
*
*    INT32 QArcCenterY   - Y center point of arc.
*
*    INT32 QArcDirection - Set to +1 if arc goes from 90 to 0.
*                          Set to -1 if arc goes from 90 to 180.
*                          No other values should be used.
*
*    INT32 QArcTToB      - Set to +1 if arc is interpreted as top to bottom.
*                          Set to -1 if bottom to top.
*                          No other values should be used.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsSetupQuadrantArc( qarcState *QArcEdge, INT32 QArcA, INT32 QArcB, INT32 QArcCenterX,
                               INT32 QArcCenterY, INT32 QArcDirection, INT32 QArcTToB)
{
    /* start Y = center Y - Y radius */
    QArcEdge->StartY = QArcCenterY - QArcB; 

    /* # of scan lines intersected by arc */
    QArcEdge->Count = QArcB + 1;            
    QArcEdge->TopToBottom = QArcTToB;
    QArcEdge->XDirection = QArcDirection;

    /* X value when Y = 0 */
    QArcEdge->CurrentX = QArcCenterX;       

    /* rightward arc? */
    if( QArcDirection < 0 )
    {
        /* no, final X is X radius distance to left of center */
        QArcEdge->qaFinalX = QArcCenterX - QArcA;
    }
    else
    {
        /* yes, final X is X radius distance to right of center */
        QArcEdge->qaFinalX = QArcCenterX + QArcA;
    }

    /* zero height? */
    if( QArcB == 0 )
    {
        /* yes, flat line, so go right to the end */
        QArcEdge->CurrentX =  QArcEdge->qaFinalX;
    }

    /* is this a vertical line? */
    else if( QArcA == 0 )
    {
        /* stepping routine */
        QArcEdge->StepVector = &StepVertical;   
    }
    else
    {
        /* X radius squared */
        QArcEdge->qaASq   = QArcA * QArcA;                      

        /* X radius ** 2 * 2 */
        QArcEdge->qaASqX2 = QArcEdge->qaASq + QArcEdge->qaASq;  

        /* Y radius squared */
        QArcEdge->qaBSq   = QArcB * QArcB;                      

        /* Y radius ** 2 * 2 */
        QArcEdge->qaBSqX2 = QArcEdge->qaBSq + QArcEdge->qaBSq;  

        /* set up scanning vector */
        QArcEdge->StepVector = &StepQATopNativeSize;            

        /* initial xadjust = 0 */
        QArcEdge->qaXAdjust = 0;                                

        /* Y adjust = 2*(XRadius**2)*YRadius */
        QArcEdge->qaYAdjust = QArcEdge->qaASqX2 * QArcB;

        /* initial qaErrTerm = ((Xradius**2 + Yradius**2) / 4) - XRadius**2*YRadius */
        QArcEdge->qaErrTerm = ((((QArcEdge->qaASq + QArcEdge->qaBSq) / 2.0) -
            QArcEdge->qaYAdjust) / 2.0);

        /* pre-compensate for adjustments StepQATopNativeSize will do on the initial
           scanning call */
        QArcEdge->qaErrTerm += QArcEdge->qaYAdjust;
        QArcEdge->qaYAdjust += QArcEdge->qaASqX2;

        /* scan to the maximum extent on the first scan line of the arc */
        StepQATopNativeSize(QArcEdge);

    } /* else */

}
#endif

#ifdef FIXPOINT
/***************************************************************************
* FUNCTION
*
*    EDGES_rsSetupBottomQuadrantArc
*
* DESCRIPTION
*
*    Function EDGES_rsSetupBottomQuadrantArc sets up a bottom quadrant arc edge
*    (a 90-degree arc edge entity, either from 270 to 180 or from 270 to 360).
*
* INPUTS
*
*    qarcState *QArcEdge - Pointer to qarcState to set up.
*
*    INT32 QArcA         - X radius of arc ( >= 0 ).
*
*    INT32 QArcB         - Y radius of arc ( >= 0 ).
*
*    INT32 QArcCenterX   - X center point of arc.
*
*    INT32 QArcCenterY   - Y center point of arc.
*
*    INT32 QArcDirection - Set to +1 if arc goes from 270 to 180.
*                          Set to -1 if arc goes from 270 to 360.
*                          No other values should be used.
*
*    INT32 QArcTToB      - Set to +1 if arc is interpreted as top to bottom.
*                          Set to -1 if bottom to top.
*                          No other values should be used.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsSetupBottomQuadrantArc( qarcState *QArcEdge, INT32 QArcA, INT32 QArcB, INT32 QArcCenterX,
                                     INT32 QArcCenterY, INT32 QArcDirection, INT32 QArcTToB)
{
    dblFix dblTmp, dblTmp2;

    /* start Y = center Y */
    QArcEdge->StartY = QArcCenterY;     

    /* # of scan lines intersected by arc */
    QArcEdge->Count = QArcB + 1;        
    QArcEdge->TopToBottom = QArcTToB;
    QArcEdge->XDirection = QArcDirection;

    /* rightward arc? */
    if( QArcDirection < 0 )
    {
        /* no, current X is X radius distance to right of center */
        QArcEdge->CurrentX = QArcCenterX + QArcA;
    }
    else
    {
        /* yes, current X is X radius distance to left of center */
        QArcEdge->CurrentX = QArcCenterX - QArcA;
    }

    /* is this a vertical line? */
    if( QArcA == 0 )
    {
        /* stepping routine */
        QArcEdge->StepVector = StepVertical;   
    }
    else
    {
        /* X radius squared */
        QArcEdge->qaASq   = QArcA * QArcA;                      

        /* X radius ** 2 * 2 */
        QArcEdge->qaASqX2 = QArcEdge->qaASq + QArcEdge->qaASq;  

        /* Y radius squared */
        QArcEdge->qaBSq   = QArcB * QArcB;                      

        /* Y radius ** 2 * 2 */
        QArcEdge->qaBSqX2 = QArcEdge->qaBSq + QArcEdge->qaBSq;  

        /* set up scanning vector */
        QArcEdge->StepVector = StepQABottomNativeSize;         

        /* initial yadjust = 0 */
        QArcEdge->qaYAdjust.fUpper = 0;                         
        QArcEdge->qaYAdjust.fLower = 0;

        /* X adjust = 2*(YRadius**2)*XRadius */
        dblTmp.fUpper  = (QArcA >> 16);
        dblTmp.fLower  = (QArcA << 16);
        dblTmp2.fUpper = (QArcEdge->qaBSqX2 >> 16);
        dblTmp2.fLower = (QArcEdge->qaBSqX2 << 16);
        dFix_mul(&dblTmp2, &dblTmp, &QArcEdge->qaXAdjust);

        /* initial qaErrTerm = ((Xradius**2 + Yradius**2) / 4) - YRadius**2*XRadius */
        dblTmp.fUpper = (QArcEdge->qaASq + QArcEdge->qaBSq);
        dblTmp.fLower = (dblTmp.fUpper << 15);
        dblTmp.fUpper = (dblTmp.fUpper >> 17);
        dFix_sub(&dblTmp, &QArcEdge->qaXAdjust, &QArcEdge->qaErrTerm);
        QArcEdge->qaErrTerm.fLower = (((UNSIGNED)QArcEdge->qaErrTerm.fUpper) << 31) |
            (QArcEdge->qaErrTerm.fLower >> 1);
        QArcEdge->qaErrTerm.fUpper = (QArcEdge->qaErrTerm.fUpper >> 1);
    } /* else */

}

#else
/***************************************************************************
* FUNCTION
*
*    EDGES_rsSetupBottomQuadrantArc
*
* DESCRIPTION
*
*    Function EDGES_rsSetupBottomQuadrantArc sets up a bottom quadrant arc edge
*    (a 90-degree arc edge entity, either from 270 to 180 or from 270 to 360).
*
* INPUTS
*
*    qarcState *QArcEdge - Pointer to qarcState to set up.
*
*    INT32 QArcA         - X radius of arc ( >= 0 ).
*
*    INT32 QArcB         - Y radius of arc ( >= 0 ).
*
*    INT32 QArcCenterX   - X center point of arc.
*
*    INT32 QArcCenterY   - Y center point of arc.
*
*    INT32 QArcDirection - Set to +1 if arc goes from 270 to 180.
*                          Set to -1 if arc goes from 270 to 360.
*                          No other values should be used.
*
*    INT32 QArcTToB      - Set to +1 if arc is interpreted as top to bottom.
*                          Set to -1 if bottom to top.
*                          No other values should be used.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsSetupBottomQuadrantArc( qarcState *QArcEdge, INT32 QArcA, INT32 QArcB, INT32 QArcCenterX,
                                     INT32 QArcCenterY, INT32 QArcDirection, INT32 QArcTToB)
{
    /* start Y = center Y */
    QArcEdge->StartY = QArcCenterY;     

    /* # of scan lines intersected by arc */
    QArcEdge->Count = QArcB + 1;        
    QArcEdge->TopToBottom = QArcTToB;
    QArcEdge->XDirection = QArcDirection;

    /* rightward arc? */
    if( QArcDirection < 0 )
    {
        /* no, current X is X radius distance to right of center */
        QArcEdge->CurrentX = QArcCenterX + QArcA;
    }
    else
    {
        /* yes, current X is X radius distance to left of center */
        QArcEdge->CurrentX = QArcCenterX - QArcA;
    }

    /* is this a vertical line? */
    if( QArcA == 0 )
    {
        /* stepping routine */
        QArcEdge->StepVector = &StepVertical;   
    }
    else
    {
        /* X radius squared */
        QArcEdge->qaASq   = QArcA * QArcA;                     

        /* X radius ** 2 * 2 */
        QArcEdge->qaASqX2 = QArcEdge->qaASq + QArcEdge->qaASq; 

        /* Y radius squared */
        QArcEdge->qaBSq   = QArcB * QArcB;                     

        /* Y radius ** 2 * 2 */
        QArcEdge->qaBSqX2 = QArcEdge->qaBSq + QArcEdge->qaBSq; 

        /* set up scanning vector */
        QArcEdge->StepVector = &StepQABottomNativeSize;        

        /* initial yadjust = 0 */
        QArcEdge->qaYAdjust = 0;                               

        /* X adjust = 2*(YRadius**2)*XRadius */
        QArcEdge->qaXAdjust = QArcEdge->qaBSqX2 * QArcA;

        /* initial qaErrTerm = ((Xradius**2 + Yradius**2) / 4) - YRadius**2*XRadius */
        QArcEdge->qaErrTerm = ((((QArcEdge->qaASq + QArcEdge->qaBSq) / 2.0) -
        QArcEdge->qaXAdjust) / 2.0);
    } /* else */

}
#endif

/***************************************************************************
* FUNCTION
*
*    EDGES_rsSetupVerticalLineEdge
*
* DESCRIPTION
*
*    Function EDGES_rsSetupVerticalLineEdge sets up a vertical line edge.
*
* INPUTS
*
*    Vedge *VLEdge  - Pointer to Vedge-compatible edge.
*
*    INT32 VLX      - Constant X, in global coords.
*
*    INT32 VLY      - Initial Y, in global coords.
* 
*    INT32 VLHeight - # of pixels to draw.
*
*    INT32 VLDir    - Set to -1 for left edge.
*                     Set to +1 for right edge.
*
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsSetupVerticalLineEdge( Vedge *VLEdge, INT32 VLX, INT32 VLY, INT32 VLHeight, INT8 VLDir)
{
    /* stepping routine */
    VLEdge->StepVector = StepVertical; 

    /* constant X value */
    VLEdge->CurrentX = VLX;             
    VLEdge->StartY = VLY;               

    /* initial Y */
    VLEdge->Count = VLHeight;
    VLEdge->TopToBottom = VLDir;
}

/***************************************************************************
* FUNCTION
*
*    EDGES_rsSetupStraightLineEdge
*
* DESCRIPTION
*
*    Function EDGES_rsSetupStraightLineEdge sets up a straight line edge.
*
* INPUTS
*
*    lineEdgeV *LineEdge - Pointer to lineEdgeV to set up.
*
*    INT32 XStart        - X start coordinate of line.
*
*    INT32 YStart        - Y start coordinate of line.
*
*    INT32 XEnd          - X end coordinate of line.
*
*    INT32 YEnd          - Y end coordinate of line.
*
*    INT32 LineEdgeTToB  - Set to +1 if line is interpreted as top to bottom.
*                          Set to -1 if bottom to top.
*                          No other values should be used.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsSetupStraightLineEdge( lineEdgeV *LineEdge, INT32 XStart, INT32 YStart,
                                   INT32 XEnd, INT32 YEnd,INT32 LineEdgeTToB)
{
    INT32 startX;
    INT32 startY;
    INT32 endX;
    INT32 endY;
    INT32 deltaX;
    INT32 deltaY;

    /* stepping routine */
    LineEdge->StepVector = StepStraightEdge;   
    LineEdge->TopToBottom = LineEdgeTToB;

    /* make sure the edge runs top->bottom */
    if (YStart < YEnd )
    {
        startX = XStart;
        startY = YStart;
        endX = XEnd;
        endY = YEnd;
    }
    else
    {
        startX = XEnd;
        startY = YEnd;
        endX = XStart;
        endY = YStart;
    }

    LineEdge->CurrentX = startX;
    LineEdge->StartY = startY;

    /* Count & ErrorTermAdjDown = DeltaY */
    deltaY = endY - startY; 
    LineEdge->Count = deltaY;
    LineEdge->ErrorTermAdjDownV = deltaY;

    deltaX = endX - startX;
    if( deltaX < 0 )
    {
        LineEdge->XDirection = (signed char)-1;
        LineEdge->ErrorTermV = -deltaY;
        deltaX = -deltaX;
    }
    else
    {
        LineEdge->XDirection = 1;
        LineEdge->ErrorTermV = -1;
    }

    /* X major or Y major? */
    if( deltaY > deltaX )
    {
        /* Y major */
        LineEdge->WholePixelXMoveV = 0;

        /* ErrorTermAdjUp = Width */
        LineEdge->ErrorTermAdjUpV = deltaX; 
    }
    else
    {
        /* X major */
        LineEdge->ErrorTermAdjUpV = deltaX % deltaY;
        if( LineEdge->XDirection == 1 )
        {
            LineEdge->WholePixelXMoveV = deltaX / deltaY;
        }
        else
        {
            LineEdge->WholePixelXMoveV = - deltaX / deltaY;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    EDGES_rsScansAndFillsEdgeList
*
* DESCRIPTION
*
*    Function EDGES_rsScansAndFillsEdgeList scans and fills an edge list.
*
* INPUTS
*
*    VOID **GETPtrPtr       - far pointer to near pointer to first edge in global edge table.
*                             The near pointer must be in the same segment as the GET, and the
*                             pointer to the GET must be followed by space for a pointer to the AET.
*
*    blitRcd *fillRcd       - far pointer to start of fillRcd buffer.
*                             Not filled out by caller; filled out in this module.
*
*    INT32 scanRcdSize      - # of bytes in fillRcd buffer.
*                             This includes space for the fillRcd itself.
*                             Must be room for one blitRcd and at least one rect.
*
*    INT32 shape            - Set to convex, non-convex, or complex.
*
*    INT32 fillRule         - Set to 0 if odd/even fill desired.
*                             Set to 1 if winding fill desired.
*                             Set to -1 if double winding rule fill desired.
*                                 (intersection of two sets of edges,used for arcs).
*
*    INT32 allEdgesAreLines - Set to 1 if all edges in the GET are line edges.
*                             Set to 0 if some edges are not lines; for example: quadrant arcs.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EDGES_rsScansAndFillsEdgeList( VOID **GETThePtr, blitRcd *fillRcd, INT32 scanRcdSize, INT32 shape,
                INT32 fillRule, INT32 allEdgesAreLines)
{
    lineEdgeV **AETEdgeLink;
    lineEdgeV *AETPtr;
    lineEdgeV *TempEdge;

    /* to remove paradigm warning */
    (VOID)shape;

    /* pick up passed parameters */
    lclFillRule = fillRule; 
    allLineEdges = allEdgesAreLines;
    lclFillRcd = fillRcd;

    /* now set up the fill record according to the settings in the current port */
    /* Init local blitRcd from grafBlit */
    *lclFillRcd = grafBlit; 

    /* Point blitList to area after local blitRcd */
    lclFillRcd->blitList = (SIGNED)lclFillRcd + sizeof(blitRcd);

    /* calculate the last address at which a rect can start in the blitRcd
       without being the last rect that can fit in the blitRcd */
    highWater = (rect *) ((SIGNED) lclFillRcd + scanRcdSize - 2 * sizeof(rect));

    /* no rects yet */
    rectCount = 0;  

    /* point to the first rect location */
    rectBase = (rect *) lclFillRcd->blitList;   
    rectPtr = rectBase;

    leGETPtr = (lineEdgeV *)*GETThePtr;

    AETEdgeLink = (lineEdgeV **) (&AETPtrPtr);

    if( leGETPtr != 0 )
    {
        /* AETPtr = NULL */
        *AETEdgeLink = 0;      

        /* initial scan line */
        currentY = leGETPtr->StartY; 

        /* mark that the first line can't be the same as the preceding line */
        sameAsLast = 0;              

        /* Moves all edges that start at the specified Y coordinate from the GET to
           the AET, placing them in the AET so as to maintain the X sorting of the AET.
           The GET is Y sorted. Any edges that start at the desired Y coordinate will
           be first in the GET, so we'll move edges from the GET to AET until the first
           edge left in the GET is no longer at the desired Y coordinate. Also, the GET
           is X sorted within each Y coordinate, so each successive edge we add to the
           AET is guaranteed to belong later in the AET than the one we just added. */
        do{
            while( leGETPtr != 0 )
            {
                if( leGETPtr->StartY != currentY )
                {
                    /* done if we've found all the edges that start on this scan line */
                    break; 
                }

                /* mark that this line isn't the same as the preceding
                   line (because we just added another edge) */
                sameAsLast = 0; 

                /* now link the new edge into the AET so that the AET is still sorted by 
                   X coordinate */
                while( (AETPtr = *AETEdgeLink) != 0 )
                {
                    if( AETPtr->CurrentX >= leGETPtr->CurrentX )
                    {
                        /* add here if new X is less than or equal to the new edge's X */
                        break;
                    }
                    AETEdgeLink = &AETPtr->NextEdge;
                }
                /* we've found the link location; unlink the edge from the GET and link
                   it into the AET */
                TempEdge           = leGETPtr->NextEdge;
                *AETEdgeLink       = leGETPtr;
                AETEdgeLink        = &leGETPtr->NextEdge;
                leGETPtr->NextEdge = AETPtr;
                leGETPtr           = TempEdge;
            }
            /* update leGETPtr */
            *GETThePtr = leGETPtr;  
            AETEdgeLink = (lineEdgeV **) (&AETPtrPtr);

            /* skip the remaining functions if the AET is empty */
            if( (AETPtr = *AETEdgeLink) != 0 )
            {
                ScanOutAET(AETPtr);

                /* Advance each edge in the AET by one scan line. Remove edges that
                   have been fully scanned. */
                /* skip if nothing in AET */
                while( (AETPtr = *AETEdgeLink) != 0 )
                {
                    /* decrement the y counter */
                    AETPtr->Count--; 

                    /* check if done with edge */
                    if( AETPtr->Count <= 0 )
                    {
                        /* yes, remove it from the AET */
                        sameAsLast   = 0;
                        *AETEdgeLink = AETPtr->NextEdge;
                    }
                    else
                    {
                        /* count off one scan line for this edge */
                        /* are all the edges lines? */
                        if( allLineEdges != 0 )
                        {
                            /* yes, handle directly. Note that this loop handles
                               sameAsLast directly */
                            if( AETPtr->WholePixelXMoveV != 0 )
                            {
                                /* advance the edge's X coordinate by the minimum # of pixels */
                                sameAsLast = 0;
                                AETPtr->CurrentX += AETPtr->WholePixelXMoveV;
                            }

                            AETPtr->ErrorTermV += AETPtr->ErrorTermAdjUpV;
                            if( AETPtr->ErrorTermV > 0 )
                            {
                                /* the error term turned over, so move X one more */
                                sameAsLast = 0;
                                AETPtr->CurrentX += AETPtr->XDirection;
                                AETPtr->ErrorTermV -= AETPtr->ErrorTermAdjDownV;
                            }
                        }
                        else
                        {
                            /* Use the StepVector field to call each edge's stepping
                               routine to handle the case where not all edges are lines. */
                            sameAsLast &= AETPtr->StepVector(AETPtr);
                        }

                        AETEdgeLink = &AETPtr->NextEdge;
                    }
                }

                AETEdgeLink = (lineEdgeV **) (&AETPtrPtr);
                XSortAET(AETEdgeLink);
            }

            currentY++;
   
        /* continue while any edges remain in either the AET or the GET */
        } while ( (*GETThePtr != 0) || (*AETEdgeLink != 0) );

        /* do any rects remain? */
        if( rectCount != 0 )
        {
            /* yes, do the remaining rects */
            /* set the count field in the blitRcd */
            lclFillRcd->blitCnt = rectCount; 

            /* draw the rectangles */
            lclFillRcd->blitDmap->prFill(lclFillRcd);
        }

    } /* if(  ) */
}

/***************************************************************************
* FUNCTION
*
*    XSortAET
*
* DESCRIPTION
*
*    Function XSortAET sorts all edges currently in the active edge table into
*    ascending order of current X coordinates.
*
* INPUTS
*
*    lineEdgeV **CurrentEdgePtr - far pointer to near pointer to current edge.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID XSortAET( lineEdgeV **CurrentEdgePtr)
{
    /* flag that a swap occurred */
    UINT8 didSwap;   
    lineEdgeV *NextEdgePtr;
    lineEdgeV *TempEdge;
    lineEdgeV *newEdge;
    lineEdgeV **tempCurrentEdgePtr;

    /* scan through the AET and swap any adjacent edges for which the second
       edge is at a lower current X coord than the first edge. Repeat until no
       further swapping is needed. */
    if( *CurrentEdgePtr != 0 )
    {
        do
        {
            /* mark that no swap has yet occurred */
            didSwap = 0;    
            newEdge = *CurrentEdgePtr;
            tempCurrentEdgePtr = CurrentEdgePtr;

            while( (NextEdgePtr = newEdge->NextEdge) != 0 )
            {
                /*if there is no next edge after the current one, we've traversed
                  the entire list */
                if( newEdge->CurrentX > NextEdgePtr->CurrentX )
                {
                    /* the second edge has a lower X than the first; reconnect the
                       links so that the two edges switch order in the AET */
                    TempEdge = NextEdgePtr->NextEdge;
                    *tempCurrentEdgePtr = NextEdgePtr;
                    NextEdgePtr->NextEdge = newEdge;
                    newEdge->NextEdge = TempEdge;

                    /* mark that a swap occurred */
                    didSwap = 1;    
                }

                tempCurrentEdgePtr = &(*tempCurrentEdgePtr)->NextEdge;
                newEdge = *tempCurrentEdgePtr;
            }
        } while( didSwap != 0 );

    } /* if( *CurrentEdgePtr != 0 ) */
}

/***************************************************************************
* FUNCTION
*
*    StepStraightEdge
*
* DESCRIPTION
*
*    The function StepStraightEdge is one of four functions which are the vectored
*    AET stepping routines.
*    Steps X from current scan line to next.
*    These functions are called through the StepVector function.
*    Returns a 1 if this edge's X didn't change from last time, 0 if it did change.
*
* INPUTS
*
*    lineEdgeV *CurrentEdge - Pointer to the CurrentEdge.
*
* OUTPUTS
*
*    UINT8 - Returns 1 if this edge's X did not change from last time.
*            Returns 0 if this edge's X changed.
*
***************************************************************************/
static UINT8 StepStraightEdge(lineEdgeV *CurrentEdge)
{
    UINT8 chngSts;

    /* assume edge doesn't change from last time */
    chngSts = 1;    

    /* advance the edge's X coordinate by the minimum # of pixels */
    if( CurrentEdge->WholePixelXMoveV != 0 )
    {
        /* mark that this line isn't the same as on the preceding line */
        chngSts               = 0;
        CurrentEdge->CurrentX += CurrentEdge->WholePixelXMoveV;
    }

    /* determine whether it's time for X to advance one extra */
    CurrentEdge->ErrorTermV += CurrentEdge->ErrorTermAdjUpV;
    if( CurrentEdge->ErrorTermV > 0 )
    {
        /* the error term turned over, so move X one more */
        chngSts                 = 0;
        CurrentEdge->CurrentX   += CurrentEdge->XDirection;
        CurrentEdge->ErrorTermV -= CurrentEdge->ErrorTermAdjDownV;
    }

    /* return line-change status */
    return (chngSts); 
}

/***************************************************************************
* FUNCTION
*
*    StepVertical
*
* DESCRIPTION
*
*    The function StepVertical is one of four functions which are the vectored AET stepping routines.
*    It steps the X Vertical edge.
*    These functions are called through the StepVector function.
*    Returns a 1 if this edge's X didn't change from last time, 0 if it did change.
*
* INPUTS
*
*    qarcState *CurrentEdge - Pointer to  the CurrentEdge.
*
* OUTPUTS
*
*    UINT8 - Returns 1 if this edge's X did not change from last time.
*            Returns 0 if this edge's X changed.
*
***************************************************************************/
static UINT8 StepVertical(qarcState *CurrentEdge)
{
    /* Set NotUsed with the unused parameter to remove warnings */
    NU_UNUSED_PARAM(CurrentEdge);

    /* X never changes from last time */
    return (1); 
}

#ifdef FIXPOINT
/***************************************************************************
* FUNCTION
*
*    StepQATopNativeSize
*
* DESCRIPTION
*
*    The function StepQATopNativeSize is one of four functions which are the 
*    vectored AET stepping routines.
*    Top-half quadrant arc stepper
*    Steps y by 1, adjusts yadjust and error term accordingly, then steps x as far as possible.
*
*    These functions are called through the StepVector function.
*    Returns a 1 if this edge's X didn't change from last time, 0 if it did change.
*
* INPUTS
*
*    qarcState *CurrentEdge - Pointer to  the CurrentEdge.
*
* OUTPUTS
*
*    UINT8 - Returns 1 if this edge's X did not change from last time.
*            Returns 0 if this edge's X changed.
*
***************************************************************************/
static UINT8 StepQATopNativeSize(qarcState *CurrentEdge)
{
    UINT8 value = 0;

    dblFix stepY, tempDF;

    /* increment the y change in the error to the value for the next y */
    tempDF.fUpper = (CurrentEdge->qaASqX2 >> 16);
    tempDF.fLower = (CurrentEdge->qaASqX2 << 16);
    dFix_sub(&CurrentEdge->qaYAdjust, &tempDF, &stepY);

    if( (stepY.fLower == 0) && (stepY.fUpper == 0) )
    {
        /* we're at the final (bottom) point on the arc; handle specially
           because otherwise the maximum extent of the arc at Y==0 isn't reached */
        /* use x-axis intercept */
        CurrentEdge->CurrentX = CurrentEdge->qaFinalX;  

        /* we don't really know, so assume edge is not previous edge */
        value = 0; 
    }
    else
    {
        /* remember new yadjust */
        CurrentEdge->qaYAdjust = stepY; 

        /* error term -= yadjust */
        dFix_sub(&CurrentEdge->qaErrTerm, &stepY, &CurrentEdge->qaErrTerm); 
        if( CurrentEdge->qaErrTerm.fUpper >= 0 )
        {
            /* can't advance x at all */
            value = 1; 
        }
        else
        {
            /* advance x until error term >= 0 (at which point x will be as far as it
               can go for this y) */
            tempDF.fUpper = (CurrentEdge->qaBSqX2 >> 16);
            tempDF.fLower = (CurrentEdge->qaBSqX2 << 16);

            do
            {
                CurrentEdge->CurrentX += CurrentEdge->XDirection;
                dFix_add(&CurrentEdge->qaXAdjust, &tempDF, &CurrentEdge->qaXAdjust);
                dFix_add(&CurrentEdge->qaErrTerm, &CurrentEdge->qaXAdjust,
                    &CurrentEdge->qaErrTerm);

            /* continue until the error term turns over */
            } while( CurrentEdge->qaErrTerm.fUpper < 0 ); 
        } /* else */

    } /* else */

    /* edge isn't same this time as last time */
    return (value); 
}

#else
/***************************************************************************
* FUNCTION
*
*    StepQATopNativeSize
*
* DESCRIPTION
*
*    The function StepQATopNativeSize is one of four functions which are the vectored AET stepping routines.
*    Top-half quadrant arc stepper
*    Steps y by 1, adjusts yadjust and error term accordingly, then steps x as far as possible.
*
*    These functions are called through the StepVector function.
*    Returns a 1 if this edge's X didn't change from last time, 0 if it did change.
*
* INPUTS
*
*    qarcState *CurrentEdge - Pointer to  the CurrentEdge.
*
* OUTPUTS
*
*    UINT8 - Returns 1 if this edge's X did not change from last time.
*            Returns 0 if this edge's X changed.
*
***************************************************************************/
static UINT8 StepQATopNativeSize(qarcState *CurrentEdge)
{
    UINT8 value = 0;

    double stepY;

    /* increment the y change in the error to the value for the next y */
    stepY = CurrentEdge->qaYAdjust - CurrentEdge->qaASqX2;
    if( stepY == 0 )
    {
        /* we're at the final (bottom) point on the arc; handle specially
           because otherwise the maximum extent of the arc at Y==0 isn't reached */
        /* use x-axis intercept */
        CurrentEdge->CurrentX = CurrentEdge->qaFinalX;  

        /* we don't really know, so assume edge is not previous edge */
        value = 0; 
    }
    else
    {
        /* remember new yadjust */
        CurrentEdge->qaYAdjust = stepY;     

        /* error term -= yadjust */
        CurrentEdge->qaErrTerm -= stepY;    
        if( CurrentEdge->qaErrTerm >= 0 )
        {
            /* can't advance x at all */
            value = 1; 
        }
        else
        {
            /* advance x until error term >= 0 (at which point x will be as far as it
               can go for this y) */
            do
            {
                CurrentEdge->CurrentX +=CurrentEdge->XDirection;
                CurrentEdge->qaXAdjust += CurrentEdge->qaBSqX2;
                CurrentEdge->qaErrTerm += CurrentEdge->qaXAdjust;

            /* continue until the error term turns over */
            } while( CurrentEdge->qaErrTerm < 0 );   
        }
    
    } /* else */

    /* edge isn't same this time as last time */
    return (value); 
}
#endif

#ifdef FIXPOINT
/***************************************************************************
* FUNCTION
*
*    StepQABottomNativeSize
*
* DESCRIPTION
*
*    The function StepQABottomNativeSize is one of four functions which are 
*    the vectored AET stepping routines.
*    Bottom-half quadrant arc stepper.
*    Steps y by 1, adjusts yadjust and error term accordingly, then steps x
*    as far as possible.
*
*    These functions are called through the StepVector function.
*    Returns a 1 if this edge's X didn't change from last time, 0 if it did change.
*
* INPUTS
*
*    qarcState *CurrentEdge - Pointer to  the CurrentEdge.
*
* OUTPUTS
*
*    UINT8 - Returns 1 if this edge's X did not change from last time.
*            Returns 0 if this edge's X changed.
*
***************************************************************************/
static UINT8 StepQABottomNativeSize(qarcState *CurrentEdge)
{
    UINT8 value = 0;

    dblFix stepY, tempDF;

    /* need later */
    stepY = CurrentEdge->qaYAdjust; 

    /* increment the y change in the error to the value for the next y */
    tempDF.fUpper = (CurrentEdge->qaASqX2 >> 16);
    tempDF.fLower = (CurrentEdge->qaASqX2 << 16);

    dFix_add(&CurrentEdge->qaYAdjust, &tempDF, &CurrentEdge->qaYAdjust);
    dFix_add(&CurrentEdge->qaErrTerm, &stepY, &CurrentEdge->qaErrTerm);

    if( (CurrentEdge->qaErrTerm.fUpper < 0) || ((CurrentEdge->qaXAdjust.fLower == 0 )
        && (CurrentEdge->qaXAdjust.fUpper == 0)))
    {
        /* can't advance x any farther */
        value = 1; 
    }
    else
    {
        /* advance x until error term < 0 (at which point x will be as far as it
           can go for this y) */
        tempDF.fUpper = (CurrentEdge->qaBSqX2 >> 16);
        tempDF.fLower = (CurrentEdge->qaBSqX2 << 16);
        do
        {
            CurrentEdge->CurrentX += CurrentEdge->XDirection;
            dFix_sub(&CurrentEdge->qaXAdjust, &tempDF, &CurrentEdge->qaXAdjust);
            if( (CurrentEdge->qaXAdjust.fLower == 0) &&
                (CurrentEdge->qaXAdjust.fUpper == 0) )
            {
                break; 
            }
            dFix_sub(&CurrentEdge->qaErrTerm, &CurrentEdge->qaXAdjust, &CurrentEdge->qaErrTerm);

        /* continue until the error term turns over */
        } while( !(CurrentEdge->qaErrTerm.fUpper < 0) ); 

    } /* else */

    /* edge isn't same this time as last time */
    return (value); 
}

#else
/***************************************************************************
* FUNCTION
*
*    StepQABottomNativeSize
*
* DESCRIPTION
*
*    The function StepQABottomNativeSize is one of four functions which are the vectored AET stepping routines.
*    Bottom-half quadrant arc stepper.
*    Steps y by 1, adjusts yadjust and error term accordingly, then steps x
*    as far as possible.
*
*    These functions are called through the StepVector function.
*    Returns a 1 if this edge's X didn't change from last time, 0 if it did change.
*
* INPUTS
*
*    qarcState *CurrentEdge - Pointer to  the CurrentEdge.
*
* OUTPUTS
*
*    UINT8 - Returns 1 if this edge's X did not change from last time.
*            Returns 0 if this edge's X changed.
*
***************************************************************************/
static UINT8 StepQABottomNativeSize(qarcState *CurrentEdge)
{
    UINT8 value = 0;

    double stepY;

    /* need later */
    stepY = CurrentEdge->qaYAdjust; 

    /* increment the y change in the error to the value for the next y */
    CurrentEdge->qaYAdjust += CurrentEdge->qaASqX2;

    /* error term += old yadjust */
    CurrentEdge->qaErrTerm += stepY;    
    if( (CurrentEdge->qaErrTerm < 0) || (CurrentEdge->qaXAdjust == 0) )
    {
        /* can't advance x any farther */
        value = 1; 
    }
    else
    {
        /* advance x until error term < 0 (at which point x will be as far as it
           can go for this y) */
        do{
            CurrentEdge->CurrentX += CurrentEdge->XDirection;
            CurrentEdge->qaXAdjust -= CurrentEdge->qaBSqX2;
            if( CurrentEdge->qaXAdjust == 0 )
            {
                break; 
            }
            CurrentEdge->qaErrTerm -= CurrentEdge->qaXAdjust;

        /* continue until the error term turns over */
        } while( CurrentEdge->qaErrTerm >= 0 ); 

    }

    /* edge isn't same this time as last time */
    return (value); 
}
#endif

/***************************************************************************
* FUNCTION
*
*    ScanOutAET
*
* DESCRIPTION
*
*    Function ScanOutAET draws the scan line described by the current AET at
*    the current Y coordinate, according to lclFillRule.
*
* INPUTS
*
*    VOID *AETPtr - Pointer to the AET.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ScanOutAET( VOID *AETPtr)
{
    INT16 Done                    = NU_FALSE;
    signed char  JumpScanOutDoubleRecord = NU_FALSE;
    signed char  JumpScanOutDoubleLoop0  = NU_FALSE;
    signed char  JumpScanOutDoubleLoop1  = NU_FALSE;

    /* running total of TopToBottom/BottomToTop */
    signed char  sumTopToBottom;   

    /* 0-count counter */
    signed char  zeroCount;        

    /* 1-count counter */
    signed char  oneCount;  

    /* 0 or 1 count indicator */
    UINT8 zero_one;         

    /* error value */
    INT16 grafErrValue;     

    /* current Y + 1 */
    INT32 currentYP1;       
    Vedge *newEdge;
    rect  *tempLastScanPtr;

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* does this scan line have the same edges in the same locations as the
       last scan line? */
    if( sameAsLast == 1 )
    {
        /* yes, so enlarge last time's rects */
        tempLastScanPtr = lastScanPtr;
        do
        {    
            /* advance the end of the rects to the current scan line */
            /* make it X-style */
            tempLastScanPtr->Ymax = currentY + 1;   

            /* point to the next rect */
            tempLastScanPtr++;  

        /* continue if there are any more rects */
        } while( tempLastScanPtr < rectPtr );

        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* no, so assume that the next line has the same edges as this line */
        sameAsLast = 1;

        /* remember where this scan line's rect list starts */
        lastScanPtr = rectPtr;      

        /* set up pointer */
        newEdge = (Vedge *)AETPtr;  

        /* scan through the AET, building a rect list from the line segments as
           each pair of edge crossings is encountered (or each matched pair, for the
           winding rule). The nearest pixel on or to the right of left edges is drawn,
           and the nearest pixel to the left of but not on right edges is drawn. */
        currentYP1 = currentY + 1;

    } /* if( !Done ) */

    /* test kind of fill */
    if( !Done && lclFillRule > 0 )
    {
        /* normal winding rule fill */
        while( newEdge != 0 )
        {
            /* set start Y in rect */
            rectPtr->Ymin = currentY;              

            /* set end Y in rect */
            rectPtr->Ymax = currentYP1;             

            /* set start X in rect */
            rectPtr->Xmin = newEdge->CurrentX;      

            /* get the top->bottom status for this edge */
            sumTopToBottom = newEdge->TopToBottom;  

            if (newEdge->NextEdge != 0)
            {
                /* scan across until equal numbers of up and down edges have been crossed */
                do
                {
                    newEdge = newEdge->NextEdge;

                    if (newEdge != 0)
                    {
                        /* get the top->bottom status for this edge & update the 
                           running sum of top->bottom and bottom->top edges*/
                        sumTopToBottom += newEdge->TopToBottom; 
                    }

                /* keep going if the edge count hasn't balanced yet */
                } while( (sumTopToBottom != 0) && (newEdge != 0) );  
                if (newEdge != 0)
                {
                    /* set end X in rect */
                    rectPtr->Xmax = newEdge->CurrentX;  

                    /* count this rect */
                    rectCount++;                        

                    /* is that all the rects that'll fit in */
                    /*  the rect list? */
                    if( rectPtr > highWater )
                    {
                        /* yes, so draw the burst and clear the rect list */
                        DrawBurst();
                    }
                    else
                    {
                        /* point to the next rect */
                        rectPtr++;  
                    }
                }
            }

            if (newEdge != 0)
            {
                newEdge = newEdge->NextEdge;
            }
        }

        Done = NU_TRUE;
    }

    if( !Done && lclFillRule == 0 )
    {
        /* odd/even fill */
        while( newEdge != 0 )
        {
            /* set start Y in rect */
            rectPtr->Ymin = currentY;           

            /* set end Y in rect */
            rectPtr->Ymax = currentYP1;         

            /* set start X in rect */
            rectPtr->Xmin = newEdge->CurrentX;  
            newEdge = newEdge->NextEdge;

            if (newEdge != 0)
            {
                /* set end X in rect */
                rectPtr->Xmax = newEdge->CurrentX;  

                /* count this rect */
                rectCount++;                        

                /* is that all the rects that'll fit in */
                /* the rect list? */
                if (rectPtr > highWater)
                {
                    /* yes, so draw the burst and clear the rect list */
                    DrawBurst();
                }
                else
                {
                    /* point to the next rect */
                    rectPtr++;  
                }
                newEdge = newEdge->NextEdge;
            }
        }

        Done = NU_TRUE;
    }

    /* double winding rule fill (intersection of two sets of edges)
       Scan out with double winding rule (counts 0 and 1 (called 0-count and
       1-count) both have to be positive to draw; this gives the intersection of
       two edge lists). Note that this can only handle a maximum of 256 stacked
       (pending same-direction) edges of either type. */

    /* initialize 0-count and 1-count counters to 0 */
    zeroCount = 0;  
    oneCount  = 0;

    while( !Done )
    {
        do
        {
            /* break if Done */
            if( Done ) 
            {
                break; 
            }
            /* is initial edge 0-count? */
            if( (newEdge->countNumber == 0 || JumpScanOutDoubleLoop0 == 1) && JumpScanOutDoubleLoop1 !=1 )
            {
                /* We have an active 0-count; look for an active 1-count to start
                   drawing, or count the 0-count down to 0, to get back to the initial
                   condition. */
                if( JumpScanOutDoubleLoop0 != 1 && JumpScanOutDoubleLoop1 != 1)
                {
                    zeroCount = newEdge->TopToBottom;
                }

                JumpScanOutDoubleLoop0 = NU_FALSE;
                if( JumpScanOutDoubleLoop1 != 1 )
                {
                    do
                    {
                        if (newEdge != 0)
                        {
                            newEdge = newEdge->NextEdge;

                            if (newEdge != 0)
                            {
                                /* 1-count? */
                                if( newEdge->countNumber == 1 )
                                {
                                    /* yes, we have a drawable area */
                                    /* set indicator */
                                    zero_one = 1;                       

                                    /* set the 1-count */
                                    oneCount = newEdge->TopToBottom;    
                                    JumpScanOutDoubleRecord = NU_TRUE;
                                    break; 
                                }

                                /* no, 0-count; 1-count is still 0 */
                                /* update the 0-count top->bottom status for this edge */
                                zeroCount += newEdge->TopToBottom;
                            }
                        }

                    } while( (zeroCount != 0) && (newEdge != 0) ); /* keep going if the edge count hasn't
                                                                      balanced yet */
                }
            
            } /* if( newEdge->countNumber == 0 ) */
            else
            {
                /* We have an active 1-count; look for an active 0-count to start
                   drawing, or count the 1-count down to 0, to get back to the initial
                   condition. */
                if( JumpScanOutDoubleLoop0 != 1 && JumpScanOutDoubleLoop1 != 1)
                {
                    oneCount = newEdge->TopToBottom;
                }

                JumpScanOutDoubleLoop1 = NU_FALSE;
                do
                {
                    if (newEdge != 0)
                    {
                        newEdge = newEdge->NextEdge;

                        if (newEdge != 0)
                        {
                            /* 0-count? */
                            if( newEdge->countNumber == 0 )
                            {   
                                /* yes, we have a drawable area */
                                /* set indicator */
                                zero_one = 0;   

                                /* set the 0-count */
                                zeroCount = newEdge->TopToBottom;   
                                JumpScanOutDoubleRecord = NU_TRUE;
                                break; /* do-while */
                            }

                            /* no, 1-count; 0-count is still 0 */
                            /* update the 1-count top->bottom status for this edge */
                            oneCount += newEdge->TopToBottom;  
                        }
                    }

                /* keep going if the edge count hasn't balanced yet */   
                } while ((oneCount != 0) && (newEdge != 0)); 

             } /* else */
            if( !JumpScanOutDoubleRecord )
            {
                /* both counts are down to 0; do we have any more edges to process? */
                if (newEdge != 0)
                {
                    newEdge = newEdge->NextEdge;
                }
            }

            if( JumpScanOutDoubleRecord )
            {
                break; /* while( newEdge != 0 ); do-while */
            }

        } while( newEdge != 0 ); /* do-while */

        if( !JumpScanOutDoubleRecord )
        {
            Done = NU_TRUE;
        }
        
        if( !Done && JumpScanOutDoubleRecord)
        {
            JumpScanOutDoubleRecord = NU_FALSE;
            /* We were working with an active 0 or 1-count, and found the other count
               edge; this is the start of a drawable area. Count the other count and start
               looking for either count to go inactive. */
            /* set start Y in rect */
            rectPtr->Ymin = currentY;           

            /* set end Y in rect */
            rectPtr->Ymax = currentYP1;         

            if (newEdge != 0)
			{
                /* set start X in rect */
                rectPtr->Xmin = newEdge->CurrentX;
			}  

            while (1)
            {
                if( newEdge == 0 )
                {
                    grafErrValue = c_FillPoly + 99;
                    nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                    Done = NU_TRUE;
                    break; 
                }

                newEdge = newEdge->NextEdge;

                if (newEdge != 0)
                {
                    /* 1-count? */
                    if( newEdge->countNumber == 1 )
                    {
                        /* update the 1-count top->bottom status for this edge */
                        oneCount += newEdge->TopToBottom;

                        /* set indicator */
                        zero_one = 1;   
                    }
                    else
                    {
                        /* update the 0-count top->bottom status for this edge */
                        zeroCount += newEdge->TopToBottom;

                        /* set indicator */
                        zero_one = 0;   
                    }

                    if( (zeroCount == 0) || (oneCount == 0) )
                    {
                        /* stop if either count = 0 */
                        break; 
                    }
                }

            } /* while (1) */

            if (newEdge == 0)
			{
				break;
			}
			
            /* we've ended the 0/1-count span, so that's the end of the drawable span */
            /* check if 0-width case? */
            if( newEdge->CurrentX != rectPtr->Xmin )
            {
                /* no */
                /* set end X in rect */
                rectPtr->Xmax = newEdge->CurrentX;  

                /* count this rect */
                rectCount++;                        

                /* is that all the rects that'll fit in */
                /*  the rect list? */
                if( rectPtr > highWater )
                {
                    /* yes, so draw the burst and clear the rect list */
                    DrawBurst();
                }
                else
                {
                    /* point to the next rect */
                    rectPtr++; 
                }
            }
            /* we have only one active 0/1-count now; scan for other count or end of
               this count */
            if( zero_one == 0 )
            {
                JumpScanOutDoubleLoop1 = NU_TRUE;
                JumpScanOutDoubleLoop0 = NU_FALSE;
            }
            else
            {
                JumpScanOutDoubleLoop0 = NU_TRUE;
                JumpScanOutDoubleLoop1 = NU_FALSE;
            }

        } /* if( !Done ) */

    } /* while( !Done )  Done */

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();
}

/***************************************************************************
* FUNCTION
*
*    DrawBurst
*
* DESCRIPTION
*
*    Function DrawBurst draws the current fill burst and clears the rect list.
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
static VOID DrawBurst(VOID)
{
    /* set the count field in the blitRcd */
    lclFillRcd->blitCnt = rectCount;
    
    /* draw filled rectangle */
    lclFillRcd->blitDmap->prFill(lclFillRcd);

    /* reset the # of rects in the rect */
    rectCount = 0;      

    /* the next rect goes at the start of the rect list */
    rectPtr = rectBase; 

    /* the next line can't be treated as the same as this line,
       because this line's rectangles aren't around any more */
    sameAsLast = 0;     
}
