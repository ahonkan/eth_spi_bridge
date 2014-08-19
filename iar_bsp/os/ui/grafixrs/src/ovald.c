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
*  ovald.c                                                      
*
* DESCRIPTION
*
*  Contains the API function RS_Oval_Draw.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Oval_Draw
*  AddOuterEdgesO
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  edges.h
*  ovald.h
*  globalrsv.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/edges.h"
#include "ui/ovald.h"
#include "ui/globalrsv.h"
#include "ui/gfxstack.h"

static VOID AddOuterEdgesO(qarcState *newEdge);

/***************************************************************************
* FUNCTION
*
*    RS_Oval_Draw
*
* DESCRIPTION
*
*    API Function RS_Oval_Draw is used to draw ovals.
*    NOTE: The graphics port must already be set up and it should be global.  
*
* INPUTS
*
*    ObjectAction action - This would be the action that would be performed on the object, 
*                          this will be a list of actions in an enumerated data type.  
*
*                          EX: FRAME  = 0
*                              PAINT  = 1
*                              FILL   = 2
*                              ERASE  = 3
*                              INVERT = 4
*                              POLY   = 5  (only used by BEZIER)
*
*    rect *argRect       - This is the rectangle that will have the action performed on it.
*
*    INT32 patt          - This is the Pattern, fill pattern structure that contains 32 default
*                          values.  So the value is 0 to 31. -1 if not used.
                           This can be user Defined.
*
* OUTPUTS
*
*    INT32 minDistIndex - Returns closest index.
*
***************************************************************************/
STATUS RS_Oval_Draw( ObjectAction action, rect *argRect, INT32 patt)
{
    STATUS  status = ~NU_SUCCESS;
    
    /* base rectangle */
    rect    rXminBase;     

    /* offsets to edges of square pen */
    INT32   halfWidth;     
    INT32   halfHeight;
    INT32   i;
    INT32   exitEarlyFlag = NU_FALSE;

    /* pointer to new edge to add to table */
    qarcState *newEdge = 0; 

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    for( i = 0; i < 12; i++) 
    {   
        /* GET is initially empty */
        qaEdgeBuffer[i].NextEdge = 0;
    }

    if( theGrafPort.pnLevel < 0 )
    {
        status = NU_SUCCESS;
        exitEarlyFlag = NU_TRUE;
    }

    /* get rectangle */
    rXmin = *argRect;

    if( (status != NU_SUCCESS) && (action == FRAME) && (exitEarlyFlag == NU_FALSE) )
    {

#ifdef  DASHED_LINE_SUPPORT
        
        /* is it dashed? */
        if( theGrafPort.pnFlags & pnDashFlg )
        {
            /* yes, handle it elsewhere */
            ARCSD_ArcDash( argRect, 0, 3600);
            return NU_SUCCESS;
        }

#endif  /* DASHED_LINE_SUPPORT */

        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(rXmin, &rXmin, 1);
        }

        /* thin or wide line? */
        if( !(theGrafPort.pnFlags & pnSizeFlg) )
        {
            /* thin width */

            rXWidth = rXmin.Xmax - rXmin.Xmin;
            if( rXWidth < 0 )
            {
                status = NU_INVALID_WIDTH;
            }

            if( status != NU_INVALID_WIDTH )
            {
                /* height */
                rXHeight = rXmin.Ymax - rXmin.Ymin;
                if( rXHeight < 0 )
                {
                    /* bad rectangle */
                    status = NU_INVALID_HEIGHT;
                }
            }

            if( status != NU_INVALID_HEIGHT )
            {
                /* calculate radii */
                xRadius = (rXWidth >> 1);   
                yRadius = (rXHeight >> 1);

                /* set up the outer edges first */
                AddOuterEdgesO(newEdge);

                /* if the outer arcs are flat, the current GET is all we need */
                if( qaEdgeBuffer[0].Count != 1 )
                {
                    /* need inner edges too */
                    /* copy the outer arcs */
                    qaEdgeBuffer[4] = qaEdgeBuffer[0];

                    /* to the inner arcs */
                    qaEdgeBuffer[5] = qaEdgeBuffer[1];
                    qaEdgeBuffer[6] = qaEdgeBuffer[2];
                    qaEdgeBuffer[7] = qaEdgeBuffer[3];

                    /* flip arc top->bottom so this can work with the outer arc */
                    newEdge = &qaEdgeBuffer[4];
                    qaEdgeBuffer[4].TopToBottom = -qaEdgeBuffer[4].TopToBottom;

                    /* shift 1 to the right */
                    qaEdgeBuffer[4].CurrentX++;

                    /* shift down 1 */
                    qaEdgeBuffer[4].StartY++;

                    /* skip the bottommost point */
                    qaEdgeBuffer[4].Count--;

                    /* insert the new edge in GET,YX sorted */
                    AddToGETYXSorted(newEdge);

                    newEdge = &qaEdgeBuffer[5];
                    qaEdgeBuffer[5].TopToBottom = -qaEdgeBuffer[5].TopToBottom;

                    /* shift 1 to the left */
                    qaEdgeBuffer[5].CurrentX--;

                    /* shift down 1 */
                    qaEdgeBuffer[5].StartY++;

                    /* skip the bottommost point */
                    qaEdgeBuffer[5].Count--;

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted(newEdge);
                    newEdge = &qaEdgeBuffer[6];
                    qaEdgeBuffer[6].TopToBottom = -qaEdgeBuffer[6].TopToBottom;

                    /* shift 1 to the right */
                    qaEdgeBuffer[6].CurrentX++;

                    /* advance Y by 1 (skip first point) */
                    newEdge->StepVector(newEdge);

                    /* skip the bottommost point */
                    qaEdgeBuffer[6].Count--;

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted(newEdge);
                    newEdge = &qaEdgeBuffer[7];
                    qaEdgeBuffer[7].TopToBottom = - qaEdgeBuffer[7].TopToBottom;

                    /* shift 1 to the left */
                    qaEdgeBuffer[7].CurrentX--;

                    /* advance Y by 1 (skip first point) */
                    newEdge->StepVector(newEdge);

                    /* skip the bottommost point */
                    qaEdgeBuffer[7].Count--;

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted(newEdge);

                    status  = NU_SUCCESS;

                } /* if (qaEdgeBuffer[0].Count != 1) */

            } /* if(status != NU_INVALID_HEIGHT) */

        } /* if(!(grafPort.pnFlags & pnSizeFlg)) ---- thin or wide line? */
        
        else /* wide width */
        {
            /* Draws a wide-line framed oval.
               The approach is the same as for thin framed ovals,
               except that the outer arcs are moved out by the X and Y pen radii,
               and the inner arcs are moved in similarly. */

            rXminBase = rXmin;

            /* For odd pen dimensions, we can be symmetric about the thin edge.
               For even, we bias the extra pixel to the outside of the frame. */

            /* set up the four outer arcs first and rounded up */
            halfWidth = (theGrafPort.pnSize.X >> 1);
            rXmin.Xmin -= halfWidth;
            rXmin.Xmax += halfWidth;
            halfHeight = (theGrafPort.pnSize.Y >> 1);
            rXmin.Ymin -= halfHeight;
            rXmin.Ymax += halfHeight;

            /* width */
            rXWidth = rXmin.Xmax - rXmin.Xmin;
            if( rXWidth < 0 )
            {
                /* bad rectangle */
                status = NU_INVALID_WIDTH;
            }

            if( status != NU_INVALID_WIDTH )
            {
                /* height */
                rXHeight = rXmin.Ymax - rXmin.Ymin;
            }

            if( rXHeight < 0 )
            {
                /* bad rectangle */
                status = NU_INVALID_HEIGHT;
            }

            if( status != NU_INVALID_HEIGHT )
            {
                /* calculate radii */
                xRadius = (rXWidth >> 1);
                yRadius = (rXHeight >> 1);

                /* set up the outer edges first */
                AddOuterEdgesO(newEdge);    

                /* if the outer arcs are flat, the current GET is all we need */
                if( qaEdgeBuffer[0].Count != 1 )
                {
                    /* need inner edges too */
                    /* Inner arcs are pulled in from frame edge by pen radii.
                       For odd pen dimensions, we can be symmetric between inner
                       and outer edged placements.  
                       For even pen dimensions, we bias the extra pixel to the outside. */
                    halfWidth  = ((theGrafPort.pnSize.X - 1) >> 1);
                    rXmin.Xmin = rXminBase.Xmin + halfWidth;
                    rXmin.Xmax = rXminBase.Xmax - halfWidth;
                    halfHeight = ((theGrafPort.pnSize.Y - 1) >> 1);
                    rXmin.Ymin = rXminBase.Ymin + halfHeight;
                    rXmin.Ymax = rXminBase.Ymax - halfHeight;

                    /* width */
                    rXWidth = rXmin.Xmax - rXmin.Xmin;  

                    /* height */
                    rXHeight = rXmin.Ymax - rXmin.Ymin; 
                    if( (rXWidth > 0) && (rXHeight > 0) )
                    {
                        /* calculate radii */
                        xRadius = (rXWidth >> 1);   
                        yRadius = (rXHeight >> 1);

                        /* left edge + height */
                        thisLeftEdge = rXmin.Xmin + 
                        xRadius;    

                        /* top edge + height */
                        nextTopEdge = rXmin.Ymin + 
                        yRadius;

                        /* shift right by 1 to compensate for the filler not drawing 
                           right edges */
                        thisRightEdge = rXmin.Xmax - xRadius + 1;   

                        /* bottom edge - height */
                        nextBottomEdge = rXmin.Ymax - yRadius;  

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
                           characteristics and to make 1 wide 
                           be really 1 wide, and add to the GET. */
                        newEdge = &qaEdgeBuffer[4];
                
                        /* shift 1 to the right */
                        newEdge->CurrentX++;

                        /* shift down 1 */
                        newEdge->StartY++;

                        /* skip the bottommost point */
                        newEdge->Count--;      

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted(newEdge);

                        newEdge = &qaEdgeBuffer[5];

                        /* shift 1 to the left */
                        newEdge->CurrentX--;

                        /* shift down 1 */
                        newEdge->StartY++;
                        newEdge->Count--;  

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted(newEdge);

                        newEdge = &qaEdgeBuffer[6];

                        /* shift 1 to the right */
                        newEdge->CurrentX++;

                        /* advance Y by 1 (skip first point) */
                        newEdge->StepVector(newEdge);   
                        newEdge->Count--;

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
                        status = NU_SUCCESS;

                    } /*if((rXWidth > 0) && (rXHeight > 0)) */
                
                } /* (qaEdgeBuffer[0].Count != 1) */

            } /* if(status != NU_INVALID_HEIGHT) */

        } /* wide width */

    } /* if (status != NU_SUCCESS && action == FRAME && exitEarlyFlag == NU_FALSE) */

    else if( (status != NU_SUCCESS) && (exitEarlyFlag == NU_FALSE) )
    {
        if( patt > 32 )
        {
            /* Reset to default pattern */
            patt = 1;
        }

        switch(action)
        {       
            case ERASE:
                grafBlit.blitRop = zREPz;
                grafBlit.blitPat = theGrafPort.bkPat;
                break;

            case FILL:
                grafBlit.blitRop = zREPz;
                grafBlit.blitPat = patt;
                break;

            case INVERT:
                grafBlit.blitRop = zINVERTz;
                break;

            case PAINT:
                break;
			case TRANS:
				grafBlit.blitRop = xAVGx;

            default:
                break;
        }

        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(rXmin, &rXmin, 0);
        }

        /* width */
        rXWidth = rXmin.Xmax - rXmin.Xmin;  

        if( rXWidth < 0 )
        {
            /* bad rectangle */
            status = NU_INVALID_WIDTH;
        }

        if( status != NU_INVALID_WIDTH )
        {
            /* height */
            rXHeight = rXmin.Ymax - rXmin.Ymin; 
            if( rXHeight < 0 )
            {
                /* bad rectangle */
                status = NU_INVALID_HEIGHT;
            }

            if( status != NU_INVALID_HEIGHT )
            {
                xRadius = (rXWidth >> 1);
                yRadius = (rXHeight >> 1);

                /* left edge + height */
                thisLeftEdge = rXmin.Xmin + xRadius;

                /* top edge + height */
                nextTopEdge = rXmin.Ymin + yRadius;

                /* right edge + height */
                thisRightEdge = rXmin.Xmax - xRadius;

                /* bottom edge - height - 1 for filling */
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

                /* now add the bottom arcs, with the top points trimmed off
                   to avoid overlap problems */

                /*are arcs only one scan line high? */
                if( qaEdgeBuffer[0].Count != 1 )
                {   
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

                    /* advance bottom arcs by one scan line to avoid overlap problems
                       (skip the top scan line, which can stick out too far and always overlaps
                       with the top quadrant arcs anyway) */

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

                } /* if (qaEdgeBuffer[0].Count != 1) */

                status = NU_SUCCESS;

            } /* if(status != NU_INVALID_HEIGHT) */

        } /* if(status != NU_INVALID_WIDTH) */

    } /* else if(status != NU_SUCCESS && action != PAINT && exitEarlyFlag == NU_FALSE) */

    if( (status == NU_SUCCESS) && (exitEarlyFlag == NU_FALSE) )
    {
        /* draw the oval and done! */
        EDGES_rsScansAndFillsEdgeList((VOID **)&GETPtr, (blitRcd *)&rFillRcd, fillRcdSize, cmplx, 1, 0);

        /* restore default blit record */
        if( (action == ERASE) || (action == INVERT) )
        {
            grafBlit.blitRop = theGrafPort.pnMode;
        }

        if( (action == ERASE) || (action == FILL) )
        {
            grafBlit.blitPat = theGrafPort.pnPat;
        }
    }

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(status);
}

/***************************************************************************
* FUNCTION
*
*    AddOuterEdgesO
*
* DESCRIPTION
*
*    Function AddOuterEdgesO builds a GET containing a set of 4 outer edges.
*
* INPUTS
*
*    qarcState *newEdge - Pointer to qarcState.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID AddOuterEdgesO(qarcState *newEdge)
{
    /* left edge + height */
    thisLeftEdge = rXmin.Xmin + xRadius; 

    /* top edge + height  */
    nextTopEdge  = rXmin.Ymin + yRadius; 
    
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
}
