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
*  edges.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for edges.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _EDGES_H_
#define _EDGES_H_

/* The following define specifies the size of the buffer used in polygon 
   rasterization. This statically allocated buffer is used only if the 
   workspace allocated buffer size is smaller than the static buffer size. */
#define STACK_BUFFER_SIZE   4096

/* Required elements in any GET/AET edge structure. Must come first in */
/* structure! */
typedef struct _edge
{
    /* pointer to next edge in linked list (ignored for hardwired lines) */
    struct _edge *NextEdge;   

    /* X coord of edge (initial X at first, then current X) */
    INT32 CurrentX;           

    /* initial Y coord of edge (at top) */
    INT32 StartY;             

    /* # of scan lines on which this edge is active */
    INT16 Count;              

    /* amount by which to advance X when error term turns over */
    signed char  XDirection;  

    /* 1 if this edge ran top to bottom as found in the vertex list, -1 for */
    /* bottom to top (used for winding rule) */
    signed char  TopToBottom; 

} edge;


/* State of one straight edge in a GET or AET, used by polygon filler.  (Suitable */
/* only for hardwired straight edge filler, flagged by allLineEdges.)  Note that in */
/* 386 protected mode entries are still words, to make it possible to handle as many */
/* edges as possible. */
typedef struct _lineEdge 
{
    /* pointer to next edge in linked list (ignored for hardwired lines) */
    struct _lineEdge *NextEdge; 

    /* X coord of edge (initial X at first, then current X) */
    INT32 CurrentX;             
    
    /* initial Y coord of edge (at top) */
    INT32 StartY;               

    /* # of scan lines on which this edge is active */
    INT16 Count;                

    /* amount by which to advance X when error term turns over */
    signed char  XDirection;    

    /* 1 if this edge ran top to bottom as found in the vertex list, -1 for */
    /* bottom to top (used for winding rule) */
    signed char  TopToBottom;   

    /* minimum amount to advance X by from one scan line to the next */
    INT16 WholePixelXMove;      

    /* just what it says */
    INT16 ErrorTerm;            

    /* amount to add to error term from one line to the next */
    INT16 ErrorTermAdjUp;       

    /* amount to subtract from error term when it turns over */
    INT16 ErrorTermAdjDown;     

} lineEdge;


/* State of one quadrant arc edge in GET or AET. */
typedef struct _qarcState
{
    /* pointer to next edge in linked list (ignored for hardwired lines) */
    struct _qarcState *NextEdge; 

    /* X coord of edge (initial X at first, then current X) */
    INT32 CurrentX;              
 
    /* initial Y coord of edge (at top) */
    INT32 StartY;                

    /* # of scan lines on which this edge is active */
    INT16 Count;                 
    
    /* amount by which to advance X when error term turns over */
    signed char  XDirection;     

    /* 1 if this edge ran top to bottom as found in the vertex list, */
    /* -1 for bottom to top (used for winding rule) */
    signed char TopToBottom;     

    /* address of routine to use to step the edge (ignored for hardwired lines) */
    UINT8 (*StepVector)(struct _qarcState *);    

    /* 0 if this is a 0-count edge, 1 if it's a 1-count edge.  Used */
    /* only for double winding rule scanning */
    SIGNED countNumber;          

    /* X radius squared */
    SIGNED qaASq;                

    /* Y radius squared */
    SIGNED qaBSq;                

    /* X radius squared times 2 */
    SIGNED qaASqX2;              

    /* Y radius squared times 2 */
    SIGNED qaBSqX2;              

#ifdef FIXPOINT 
    dblFix qaXAdjust;
    dblFix qaYAdjust;
    dblFix qaErrTerm;
#else
    double qaXAdjust;
    double qaYAdjust;
    double qaErrTerm;
#endif
    SIGNED qaFinalX;

} qarcState;


/* Required elements in any vectored stepping GET/AET edge structure. Must come first */
/* in structure! */
typedef struct _Vedge
{
    /* pointer to next edge in linked list (ignored for hardwired lines) */
    struct _Vedge *NextEdge;  

    /* X coord of edge (initial X at first, then current X) */
    INT32 CurrentX;           

    /* initial Y coord of edge (at top) */
    INT32 StartY;             

    /* # of scan lines on which this edge is active */
    INT16 Count;              

    /* amount by which to advance X when error term turns over */
    signed char  XDirection;  

    /* 1 if this edge ran top to bottom as found in the vertex list, -1 */
    /* for bottom to top (used for winding rule) */
    signed char  TopToBottom; 

    /* address of routine to use to step the edge (ignored for hardwired lines) */
    UINT8 (*StepVector)(struct _qarcState *); 
    
    /* 0 if this is a 0-count edge, 1 if it's a 1-count edge.  Used only for */
    /* double winding rule scanning */
    SIGNED countNumber;       

} Vedge;


/* State of one straight edge in a GET or AET, used by vectored stepping handler. */
typedef struct _lineEdgeV
{
    /* pointer to next edge in linked list (ignored for hardwired lines) */
    struct _lineEdgeV *NextEdge; 

    /* X coord of edge (initial X at first, then current X) */
    INT32 CurrentX;              

    /* initial Y coord of edge (at top) */
    INT32 StartY;                

    /* # of scan lines on which this edge is active */
    INT16 Count;                 

    /* amount by which to advance X when error term turns over */
    signed char  XDirection;     

    /* 1 if this edge ran top to bottom as found in the vertex list, -1 */
    /* for bottom to top (used for winding rule) */
    signed char  TopToBottom;    

    /* address of routine to use to step the edge (ignored for hardwired lines) */
    UINT8 (*StepVector)(struct _lineEdgeV *); 
    
    /* 0 if this is a 0-count edge, 1 if it's a 1-count edge.  Used only */
    /* for double winding rule scanning */
    SIGNED countNumber;       

    /* minimum amount to advance X by from one scan line to the next */
    SIGNED WholePixelXMoveV;  

    /* just what it says */
    SIGNED ErrorTermV;        

    /* amount to add to error term from one line to the next */
    SIGNED ErrorTermAdjUpV;   

    /* amount to subtract from error term when it turns over */
    SIGNED ErrorTermAdjDownV; 

} lineEdgeV;


#define fillRcdSize (sizeof(blitRcd) + 4*sizeof(rect))

typedef union _rFillRcdType 
{
    SIGNED force_alignment;
    UINT8 fillRcd[fillRcdSize];
} rFillRcdType;



/* Local Functions  */
VOID AddToGETYXSorted(qarcState *newEdge);
INT16 ChkAngle(INT32 rsangle);
VOID EDGES_rsSetupQuadrantArc( qarcState *QArcEdge, INT32 QArcA, INT32 QArcB, INT32 QArcCenterX,
             INT32 QArcCenterY, INT32 QArcDirection, INT32 QArcTToB);
VOID EDGES_rsSetupBottomQuadrantArc( qarcState *QArcEdge, INT32 QArcA, INT32 QArcB, INT32 QArcCenterX,
             INT32 QArcCenterY, INT32 QArcDirection, INT32 QArcTToB);
VOID EDGES_rsSetupVerticalLineEdge( Vedge *VLEdge, INT32 VLX, INT32 VLY, INT32 VLHeight, INT8 VLDir);
VOID EDGES_rsSetupStraightLineEdge( lineEdgeV *LineEdge, INT32 XStart, INT32 YStart, INT32 XEnd, INT32 YEnd,
            INT32 LineEdgeTToB);
VOID EDGES_rsScansAndFillsEdgeList( VOID **GETThePtr, blitRcd *fillRcd, INT32 scanRcdSize, INT32 shape,
                INT32 fillRule, INT32 allEdgesAreLines);
VOID XSortAET( lineEdgeV **CurrentEdgePtr);
VOID ScanOutAET(VOID *AETPtr);

#endif /* _EDGES_H_*/






