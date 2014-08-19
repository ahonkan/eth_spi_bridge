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
*  xypt.c                                                       
*
* DESCRIPTION
*
*  This file contains Point operation functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  AddPt
*  SubPt
*  SetPt
*  DupPt
*  EqualPt
*  ScalePt
*  MapPt
*  PtOnPoly
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  xypt.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/xypt.h"

/***************************************************************************
* FUNCTION
*
*    AddPt
*
* DESCRIPTION
*
*    Function AddPt adds the srcPt coordinates to the dstPt coordinates.
*
* INPUTS
*
*    point *srcPt - Pointer to source point.
*
*    point *dstPt - Pointer to destination point.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID AddPt( point *srcPt, point *dstPt)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    dstPt->X = dstPt->X + srcPt->X;
    dstPt->Y = dstPt->Y + srcPt->Y;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SubPt
*
* DESCRIPTION
*
*    Function SubPt subtracts the srcPt coordinates to the dstPt coordinates.
*
* INPUTS
*
*    point *srcPt - Pointer to source point.
*
*    point *dstPt - Pointer to destination point.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SubPt( point *srcPt, point *dstPt)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    dstPt->X = dstPt->X - srcPt->X;
    dstPt->Y = dstPt->Y - srcPt->Y;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SetPt
*
* DESCRIPTION
*
*    Function SetPt sets the dstPt equal to the specified X,Y coordinate values.
*
* INPUTS
*
*    point *arg  - Pointer to point created.
*
*    INT32 argX - Point x coordinate.
*
*    INT32 argY - Point y coordinate.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetPt( point *arg, INT32 argX, INT32 argY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    arg->X = argX;
    arg->Y = argY;
    
    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SetPt
*
* DESCRIPTION
*
*    Function DupPt copies the contents of the source point structure, srcPt,
*    into the destination point structure, dstPt.
*
* INPUTS
*
*    point *srcPt - Pointer to source point.
*
*    point *dstPt - Pointer to destination point.
*
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DupPt( point *srcPt, point *dstPt)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    dstPt->X = srcPt->X;
    dstPt->Y = srcPt->Y;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    EqualPt
*
* DESCRIPTION
*
*    Function EqualPt compares the two point variables and RETURNs TRUE(1) if they
*    are the same, and FALSE(0) if they are not.
*
* INPUTS
*
*    point *Pt1 - Pointer to one point.
*
*    point *Pt2 - Pointer to other point.
*
*
* OUTPUTS
*
*    INT32 - Returns 1 if same.
*            Returns 0 if not same.
*
***************************************************************************/
INT32 EqualPt( point *Pt1, point *Pt2)
{
    INT32 value = 0;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (Pt1->X == Pt2->X) && (Pt1->Y == Pt2->Y) )
    {
        value = 1;
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    ScalePt
*
* DESCRIPTION
*
*    Function ScalePt scales the specified point value, by multiplying the X,Y 
*    coordinates values by the width and height ratios of the two rectangles.
*
* INPUTS
*
*    point *PT  - Pointer to the point.
*
*    rect *srcR - Pointer to the first rectangle.
*
*    rect *dstR - Pointer to the second rectangle.
*
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ScalePt( point *PT, rect *srcR, rect *dstR)
{
    INT16  Done = NU_FALSE;
    SIGNED dst_X, src_X;
    SIGNED dst_Y, src_Y;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Take the Delta X and add one for odd integer division case */
    /* Error checking also is included for each single case       */
    while( !Done )
    {

        /* over_flow */
        dst_X = dstR->Xmax - dstR->Xmin ; 

        /* destination point */
        dst_X +=  1; 
        dst_X = PT->X * dst_X;         

        /* over_flow */
        src_X = srcR->Xmax - srcR->Xmin; 

        /* source point */
        src_X += 1; 

        /* Multiply width and  height ratio */    
        /* for the new X coordinate point   */
        src_X = PT->X * dst_X / src_X;  
        PT->X = src_X;

        /* over_flow */
        dst_Y = dstR->Ymax - dstR->Ymin; 

        /* destination point */
        dst_Y += 1; 

        dst_Y = PT->Y * dst_Y;

        /* over_flow */
        src_Y = srcR->Ymax - srcR->Ymin; 

        /* source point */
        src_Y += 1; 

        /* Multiply width and  height ratio */
        /* for the new Y coordinate point   */
        src_Y = PT->Y * dst_Y / src_Y;  

        break; /* while( !Done ) */
    } /* while( !Done ) */

    if( !Done )
    {
        PT->Y = src_Y;
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    MapPt
*
* DESCRIPTION
*
*    Procedure MapPt maps the specified point, PT, within the source rectangle 
*    (srcR) to a similarly located point within the destination rectangle (dstR).
*
* INPUTS
*
*    point *PT  - Pointer to the point.
*
*    rect *srcR - Pointer to the source rectangle.
*
*    rect *dstR - Pointer to the destination rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MapPt( point *PT, rect *srcR , rect *dstR)
{
    INT32 X, Y;
    INT32 src_dltX, dst_dltX;
    INT32 src_dltY, dst_dltY;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    dst_dltX = dstR->Xmax - dstR->Xmin; 
    
    src_dltX = srcR->Xmax - srcR->Xmin; 
    if( src_dltX > 0 )
    {
        X = PT->X - srcR->Xmin;

        X = X * (dst_dltX / src_dltX);

        PT->X = X + dstR->Xmin;

        dst_dltY = dstR->Ymax - dstR->Ymin;

        src_dltY = srcR->Ymax - srcR->Ymin;
    
        if( src_dltY > 0 )
        {
            Y = PT->Y - srcR->Ymin;
            Y = Y * (dst_dltY / src_dltY);

            PT->Y = Y + dstR->Ymin;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    PtOnPoly
*
* DESCRIPTION
*
*    Function PtOnPoly simply replaces the filler with 
*    POLYGONS_rsSuperSetPolyLineDrawer, which detects whether there is 
*    a match or not. This allows perfect coincidence with the drawing 
*    routine, because the drawer routine is actually used.
*
* INPUTS
*
*    INT32 POLYCNT     - Number of points
*
*    polyHead *POLYHDR - Pointer to the polyHead structure. 
*
*    point *POLYPTS    - Pointer to the point array.
*
* OUTPUTS
*
*    INT32             - Returns NU_SUCCESS if successful.
*
***************************************************************************/
INT32 PtOnPoly( INT32 rspolycnt , polyHead *rspolyhdrs, point *rspolypts)
{

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    
    VectSetup();    
    
    /* smart link square pen PolyLine */
    lineSqPolyIDV = (SIGNED) POLYGONS_rsSuperSetPolyLineDrawer;  
    PtRslt = RS_Polygon_Draw( FRAME, rspolycnt, rspolyhdrs, rspolypts, -1);
    VectRestore();

    /* Return to user mode */
    NU_USER_MODE();

    return(PtRslt);
}
