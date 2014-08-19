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
***************************************************************************

**************************************************************************
*
* FILE NAME                                                         
*
* arcsd.c                                                       
*
* DESCRIPTION
*
*  All Arc draw functions.The main API for ARCS is included  
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Arc_Draw   - API call
*  ARCSD_FrameArc - support function for API
*  ARCSD_ArcDash  - support function for API
*
* DEPENDENCIES
*
*  rs_base.h
*  edges.h
*  global.h
*  globalrsv.h
*  arcsd.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/edges.h"
#include "ui/global.h"
#include "ui/globalrsv.h"
#include "ui/arcsd.h"
#include "ui/gfxstack.h"

/* Functions with local scope. */
static VOID ARCSD_FrameArc(rect *oval_rect, INT16 bgn_angle, INT16 arc_angle);

/***************************************************************************
* FUNCTION
*
*    RS_Arc_Draw
*
* DESCRIPTION
*
*    Functions for drawing ARC's.  The API allows you to PAINT, FRAME, ERASE
*    INVERT, and FILL an ARC.
*
* INPUTS
*
*    ObjectAction - This would be the action that would be performed on the 
*                   object, this will be a list of actions in an enumerated data type.  
*
*        EX: FRAME  = 0,
*            PAINT  = 1,
*            FILL   = 2,
*            ERASE  = 3,
*            INVERT = 4,  
*            POLY   = 5  (only used by BEZIER) 
*
*    rect *arg_rect  - This is the rectangle that will have the action performed on it.
*
*    INT16 bgn_rect  - Which is the beginning angle of the arc.
*
*    INT16 arc_angle - Which is the angle that the arc is supposed to be.
*
*    INT16 patt      - Fill pattern structure that contains 32 default values
*                      So the value is 0 to 31. -1 if not used.
*                      This can be user Defined
*
* OUTPUTS
*
*    STATUS status   - return either NU_SUCCESS for passing and ~NU_SUCCESS if failure. 
*
****************************************************************************/
STATUS RS_Arc_Draw( ObjectAction action, rect *arg_rect, INT16 bgn_angle, INT16 arc_angle, INT16 patt)
{
    INT16  done              = NU_FALSE;
    STATUS status            = ~(NU_SUCCESS);
    INT16  raster_op         = 0;
    INT16  the_pattern       = 0;
    INT16  sav_rop;
    INT16  sav_pat;
    rect   global_rect;
    rect   center_point_rect;
    INT32  line_height;
    INT32  end_angle;
    INT32  bgn_quad;
    INT32  end_quad;
    INT32  arc_type;
    INT32  type_ptr;
    INT32  i;

    /* begin angle intersection point */
    point  bgn_pt;                           

    /* end angle intersection point */
    point  end_pt;                           
    
    /* table of X, Y values for the edges */
    INT32  point_tbl[12];                    

    /* pointer to new edge to add to table */
    qarcState *new_edge;                     
    

    /* The following table defines where top quadrant arcs are drawn
       for each arc type (1=draw, 0=no draw). */
    UINT8 TOP_TBL[20] = {1,1,1,1,1, 1,1,1,1,1, 0,0,1,1,1, 0,1,1,1,1};

    /* The following table defines where bottom quadrant arcs are drawn
       for each arc type (1=draw, 0=no draw). */
    UINT8 BOT_TBL[20] = {0,0,1,1,1, 0,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1};

    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* Save the current blitrecord raster_op and pattern */
    sav_rop = grafBlit.blitRop;
    sav_pat = grafBlit.blitPat;

    if (arc_angle < 0)
    {
        bgn_angle+= arc_angle;
        arc_angle = 0 - arc_angle;
    }

    while( !done ) 
    {
        switch( action )
        {
            case FRAME:
                ARCSD_FrameArc( arg_rect, bgn_angle, arc_angle);
                status = NU_SUCCESS;
                done   = NU_TRUE;
                break;

            case PAINT:
                raster_op   = theGrafPort.pnMode;
                the_pattern = theGrafPort.pnPat; 
                break;

            case ERASE:
                the_pattern = theGrafPort.bkPat;
                break;

            case INVERT:
                raster_op   = zINVERTz;
                the_pattern = 1;
                break;

            case FILL:
                the_pattern = patt;
                break;
			case TRANS:
				grafBlit.blitRop = xAVGx;
				break;
            case POLY:
            default:
                status = NU_INVALID_DRAW_OP;
                done   = NU_TRUE;
                break;
        }

        if( done )
        {
            break; /* while( !Done ) */
        }
            
        if( theGrafPort.pnLevel < 0 )
        {
            /* There is nothing to do, so get out */
            status = NU_SUCCESS;
            done   = NU_TRUE;
            break; 
        }
        
        if(arc_angle == 0)
        {
            status = NU_INVALID_ANGLE;
            done   = NU_TRUE;
            break; 
        }

        /* get rectangle */
        global_rect = *arg_rect;  

        /* check and keep global from now on */
        globalLevel--;  
        if( globalLevel >= 0 )
        {
            /* convert from user to global */
            U2GR( global_rect, & global_rect, 0);
        }

        xRadius = ((global_rect.Xmax - global_rect.Xmin) >> 1);
        yRadius = ((global_rect.Ymax - global_rect.Ymin) >> 1);
        
        if( (xRadius == 0) || (yRadius == 0) )
        {
            globalLevel++;
            status = NU_SUCCESS;
            done   = NU_TRUE;
            break; 
        }
        else if( (xRadius < 0) || (yRadius < 0) )
        {
            globalLevel++;
            status = NU_INVALID_ANGLE;
            done   = NU_TRUE;
            break; 
        }

        for( i = 0; i < 12; i++)
        { 
            /* GET is initially empty */
            qaEdgeBuffer[i].NextEdge = 0;   
        }

        if( arc_angle < 0 )
        {    
            /* arc angle less than 0; fixup begin angle and make positive */
            if( arc_angle < -3600 )
            {
                arc_angle = -3600;
            }
            bgn_angle = bgn_angle + arc_angle;
            arc_angle = -arc_angle;
        }

        if( arc_angle > 3600 ) 
        {
            /* limit to +360.0 */
            arc_angle = 3600;    
        }

        /* insure ( 0 <= bgn_angle < 3600 ) */
        bgn_angle = ChkAngle(bgn_angle);  

        end_angle = bgn_angle + arc_angle;
        bgn_quad  = bgn_angle / 900;
        end_quad  = end_angle / 900;
        arc_type  = end_quad + (bgn_quad << 2);

        if (action != TRANS)
        {
            /* set the blitrecord raster_op and pattern to the requested */
            grafBlit.blitRop = raster_op;
            grafBlit.blitPat = the_pattern;
        }
            
        /* compute center point */
        center_point_rect.Xmin = global_rect.Xmin + xRadius;   
        center_point_rect.Ymin = global_rect.Ymin + yRadius;
        center_point_rect.Xmax = global_rect.Xmax - xRadius;
        center_point_rect.Ymax = global_rect.Ymax - yRadius;

        /* point of intersection for begin angle */
        OvalPt(&global_rect, bgn_angle, &bgn_pt);  

        /* point of intersection for end angle */
        OvalPt(&global_rect, end_angle, &end_pt);  
        GETPtr = &qaEdgeBuffer[0];
        new_edge = GETPtr;

        /* set up the top arcs first */
        /* Draw or no draw top arcs? */
        if( TOP_TBL[arc_type] != 0 )  
        {
            /* draw  - show as arc entry */
            new_edge->countNumber = 0;

            EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, center_point_rect.Xmin,
                                      center_point_rect.Ymin, -1, -1);

            /* point to next table entry */
            new_edge++;  

            EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, center_point_rect.Xmax,
                                      center_point_rect.Ymin, 1, 1);

            /* show as arc entry */
            new_edge->countNumber = 0;   

            /* insert the new edge in GET, YX sorted */
            AddToGETYXSorted( new_edge);

            /* point to next table entry */
            new_edge++;  
        }

        if(  ((TOP_TBL[arc_type] != 0) && (qaEdgeBuffer[0].Count != 1) ) 
            ||
              (TOP_TBL[arc_type] == 0)
          )
        {
            /* now add the bottom arcs, with the top points trimmed off to avoid  */
            /* overlap problems. Draw or no draw bottom arcs? */
            if( BOT_TBL[arc_type] != 0 )
            {
                /* draw */
                EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, center_point_rect.Xmin,
                                              ( center_point_rect.Ymax - 1), 1, -1);

                /* show as arc entry */
                new_edge->countNumber = 0;   

                /* advance Y by 1 (skip first point) */
                new_edge->StepVector( new_edge);   

                /* skip the bottom most point */
                new_edge->Count--;   

                /* shift down 1 */
                new_edge->StartY++;  

                /* skip add if first one */
                if( new_edge != &qaEdgeBuffer[0] )    
                {
                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge);
                }
        
                /* point to next table entry */
                new_edge++;  

                EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, center_point_rect.Xmax, 
                                              ( center_point_rect.Ymax - 1), -1, 1);

                /* show as arc entry */
                new_edge->countNumber = 0;

                /* advance Y by 1 (skip first point) */
                new_edge->StepVector( new_edge);

                /* skip the bottom most point */
                new_edge->Count--;   

                /* shift down 1 */
                new_edge->StartY++;

                /* insert the new edge in GET, YX sorted */
                AddToGETYXSorted( new_edge);              
    
                /* point to next table entry */
                new_edge++;      
            }
        }

        /* Ready to process line insertions */

        /* set up point table */
        point_tbl[0]  = center_point_rect.Xmin;
        point_tbl[1]  = center_point_rect.Ymin;
        point_tbl[2]  = center_point_rect.Xmax;
        point_tbl[3]  = center_point_rect.Ymax;
        point_tbl[4]  = global_rect.Xmin;
        point_tbl[5]  = global_rect.Ymin;
        point_tbl[6]  = global_rect.Xmax;
        point_tbl[7]  = global_rect.Ymax;
        point_tbl[8]  = bgn_pt.X;
        point_tbl[9]  = bgn_pt.Y;
        point_tbl[10] = end_pt.X;
        point_tbl[11] = end_pt.Y;

        for( type_ptr = 0; type_ptr < 5; type_ptr++)
        {
            /* vertical or line edge entry? */
            if( type_tbl[arc_type][type_ptr][AETYPE] == VT )        
            {
                /* Enter vertical edge */
                line_height = (  point_tbl[type_tbl[arc_type][type_ptr][AEENDY]]
                              - point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]] );

                /* skip if 0 */
                if( line_height > 0 )
                {
                    /* insert end line edge */
                    EDGES_rsSetupVerticalLineEdge( (Vedge *)new_edge, 
                                            point_tbl[type_tbl[arc_type][type_ptr][AEBGNX]],
                                            point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]],
                                            line_height,
                                            type_tbl[arc_type][type_ptr][AEDIR] );

                    new_edge->countNumber = 1;

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted(new_edge);
                    new_edge++;
                }
            }
            else
            {
                /* Enter line edge */
                /* skip if bgnY == endY */
                if(    point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]]
                    != point_tbl[type_tbl[arc_type][type_ptr][AEENDY]]
                  )
                {
                    /* insert the line edge */
                    EDGES_rsSetupStraightLineEdge( (lineEdgeV *)new_edge, 
                                                    point_tbl[type_tbl[arc_type][type_ptr][AEBGNX]],
                                                    point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]],
                                                    point_tbl[type_tbl[arc_type][type_ptr][AEENDX]],
                                                    point_tbl[type_tbl[arc_type][type_ptr][AEENDY]],
                                                    type_tbl[arc_type][type_ptr][AEDIR] );

                    new_edge->countNumber = 1;

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge);
                    new_edge++;
                }
            }

            if( type_tbl[arc_type][type_ptr][AEEND] == TEND )
            {
                break; /* for( ) loop */
            }

        } /* for (type_ptr = 0; type_ptr < 5; type_ptr++) */

        /* draw the arc and done! */
        EDGES_rsScansAndFillsEdgeList( (VOID **)&GETPtr, (blitRcd *)&rFillRcd, fillRcdSize, cmplx, -1, 0);
        globalLevel++;

        /* restore the blitrecord raster_op and pattern */
        grafBlit.blitRop = sav_rop;
        grafBlit.blitPat = sav_pat;

        status = NU_SUCCESS;
        done   = NU_TRUE;
        break;  
    } 

    /* to remove paradigm warning */
    (VOID)done;
    
    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
    
    return(status);
}


/***************************************************************************
* FUNCTION
*
*    ARCSD_FrameArc
*
* DESCRIPTION
*
*    Draws an outline of the elliptical arc contained within rectangle oval_rect
*    starting at bgn_angle degrees with a total of arc_angle degrees. The 
*    current "pen" position is not affected by ARCSD_FrameArc.
*
* INPUTS
*
*    rect * oval_rect - the Oval to Frame
*    INT32 bgn_angle  - This is bgn_angle, which is the beginning angle of 
*                       the arc.
*    INT32 arc_angle  - This is the arc_angle, which is the angle that the 
*                       arc is supposed to be.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
static VOID ARCSD_FrameArc(rect *oval_rect, INT16 bgn_angle, INT16 arc_angle)
{
   
    /* base rectangle */
    rect      g_base_rect;        

    /* global outer rectangle */
    rect      g_rect;            

    /* global inner rectangle */
    rect      inner_rect;            

    /* center point of rectangle */
    rect      center_rect;            

    /* begin angle intersection point */
    point     bgn_pt;         

    /* end angle intersection point */
    point     end_pt;         

    /* error value */
    INT16     graf_err_value;  

    /* offsets to edges of square pen */
    INT32     half_width_edge; 

    /* offsets to edges of square pen */
    INT32     half_height;    

    /* table of X, Y values for the edges */
    INT32     point_tbl[12];  

    /* pointer to new edge to add to table */
    qarcState *new_edge;      
    INT32     line_height;
    INT32     end_angle;
    INT32     bgn_quad;
    INT32     end_quad;
    INT32     arc_type;
    INT32     type_ptr;    
    INT32     i;
    INT16     done   = NU_FALSE;
   
    /* initialize edge buffer to 0 */
    for( i = 0; i < 12; i++)
    {
        /* GET is initially empty */
        qaEdgeBuffer[i].NextEdge = 0;
    }

    while( !done ) 
    {
        if( theGrafPort.pnLevel < 0 )
        {
            done = NU_TRUE;
        }

#ifdef  DASHED_LINE_SUPPORT
        
        /* is it dashed? */
        if( (theGrafPort.pnFlags & pnDashFlg) && ( !done ) )
        {
            /* yes, handle it elsewhere */
            ARCSD_ArcDash( oval_rect, bgn_angle, arc_angle);
            done = NU_TRUE;
        }

#endif  /* DASHED_LINE_SUPPORT */
        
        /* set up angles */
        if( (arc_angle == 0) && (!done) )
        {
            done = NU_TRUE;
        }

        if ( !done )
        {
            if( arc_angle < 0 )
            {
                /* arc angle less than 0; fixup begin angle and make positive */
                if( arc_angle < -3600 )
                {
                    arc_angle = -3600;
                }
                bgn_angle = bgn_angle + arc_angle;
                arc_angle = -arc_angle;
            }

            /* limit to +360.0 */
            if( arc_angle > 3600 )
            {
                arc_angle = 3600;
            }

            /* insure ( 0 <= bgn_angle < 3600 ) */
            bgn_angle = ChkAngle(bgn_angle);

            end_angle = bgn_angle + arc_angle;
            bgn_quad  = bgn_angle / 900;
            end_quad  = end_angle / 900;
            arc_type  = end_quad + (bgn_quad << 2);

            /* get rectangle */
            g_rect    = *oval_rect;
        
            /* check and keep global from now on */
            globalLevel--;
            if( globalLevel >= 0 )
            {
                /* convert from user to global */
                U2GR( g_rect, &g_rect, 1);
            }

            /* thin or wide line? */
            if( !(theGrafPort.pnFlags & pnSizeFlg) )
            {
                /* thin */
                xRadius = ( (g_rect.Xmax - g_rect.Xmin) >> 1 );
                yRadius = ( (g_rect.Ymax - g_rect.Ymin) >> 1 );

                if( (xRadius == 0) || (yRadius == 0) )
                {
                    globalLevel++;

                    done = NU_TRUE;
                }
                else if( (xRadius < 0) || (yRadius < 0) )
                {
                    /* bad rectangle */
                    graf_err_value = c_FrameArc +  c_NullRect; 

                    /* report error */
                    nuGrafErr(graf_err_value, __LINE__, __FILE__);                 
                    globalLevel++;

                    done = NU_TRUE;
                }

                if (!done)
                {
                    /* compute center point */
                    center_rect.Xmin = g_rect.Xmin + xRadius;
                    center_rect.Ymin = g_rect.Ymin + yRadius;
                    center_rect.Xmax = g_rect.Xmax - xRadius;
                    center_rect.Ymax = g_rect.Ymax - yRadius;
            
                    /* point of intersection for begin angle */
                    OvalPt( &g_rect, bgn_angle, &bgn_pt);  

                    /* point of intersection for end angle */
                    OvalPt( &g_rect, end_angle, &end_pt);  

                    /* set up the outer edges first */
                    GETPtr = &qaEdgeBuffer[0];
                    new_edge = GETPtr;

                    /* show as arc entry */
                    new_edge->countNumber = 0;   
                    EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, center_rect.Xmin, center_rect.Ymin, -1, -1);

                    new_edge = &qaEdgeBuffer[1];
                    EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, center_rect.Xmax, center_rect.Ymin, 1, 1);            

                    /* show as arc entry */
                    new_edge->countNumber = 0;   

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge); 

                    new_edge = &qaEdgeBuffer[2];
                    EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, center_rect.Xmin, (center_rect.Ymax - 1), 1, -1);

                    /* show as arc entry */
                    new_edge->countNumber = 0;   

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge); 

                    new_edge = &qaEdgeBuffer[3];
                    EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, center_rect.Xmax, (center_rect.Ymax - 1), -1, 1);

                    /* show as arc entry */
                    new_edge->countNumber = 0;   

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge); 

                    /* point to next table entry */
                    new_edge = &qaEdgeBuffer[4]; 

                    /* if the outer arcs are flat, the current GET is all we need */
                    if( qaEdgeBuffer[0].Count != 1 )
                    {
                        /* need inner edges too */
                
                        /* copy the outer arcs to the inner arcs */
                        qaEdgeBuffer[4] = qaEdgeBuffer[0];
                        qaEdgeBuffer[5] = qaEdgeBuffer[1];
                        qaEdgeBuffer[6] = qaEdgeBuffer[2];
                        qaEdgeBuffer[7] = qaEdgeBuffer[3];

                        /* flip arc top->bottom so this can work with the outer arc */
                        new_edge = &qaEdgeBuffer[4];
                        qaEdgeBuffer[4].TopToBottom = -qaEdgeBuffer[4].TopToBottom;

                        /* shift 1 to the right */
                        qaEdgeBuffer[4].CurrentX++; 

                        /* shift down 1 */
                        qaEdgeBuffer[4].StartY++;   

                        /* skip the bottom most point */
                        qaEdgeBuffer[4].Count--;    

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted( new_edge); 

                        new_edge = &qaEdgeBuffer[5];
                        qaEdgeBuffer[5].TopToBottom = -qaEdgeBuffer[5].TopToBottom;

                        /* shift 1 to the left */
                        qaEdgeBuffer[5].CurrentX--; 

                        /* shift down 1 */
                        qaEdgeBuffer[5].StartY++;   

                        /* skip the bottom most point */
                        qaEdgeBuffer[5].Count--;    

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted( new_edge); 

                        new_edge = &qaEdgeBuffer[6];
                        qaEdgeBuffer[6].TopToBottom = -qaEdgeBuffer[6].TopToBottom;

                        /* shift 1 to the right */
                        qaEdgeBuffer[6].CurrentX++;    

                        /* advance Y by 1 (skip first point) */
                        new_edge->StepVector( new_edge); 

                        /* skip the bottom most point */
                        qaEdgeBuffer[6].Count--;       

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted( new_edge);    

                        new_edge = &qaEdgeBuffer[7];
                        qaEdgeBuffer[7].TopToBottom = -qaEdgeBuffer[7].TopToBottom;

                        /* shift 1 to the left */
                        qaEdgeBuffer[7].CurrentX--;   

                        /* advance Y by 1 (skip first point) */
                        new_edge->StepVector( new_edge); 

                        /* skip the bottom most point */
                        qaEdgeBuffer[7].Count--;       

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted( new_edge);    

                        /* point to next table entry */
                        new_edge = &qaEdgeBuffer[8];    
                    }
                }
            }
            else
            {
                /* wide */

                /* Draws a wide-line framed oval. The approach is the same as for thin
                   framed ovals, except that the outer arcs are moved out by the X and Y
                   pen radii, and the inner arcs are moved in similarly. */

                g_base_rect = g_rect;

                /* For odd pen dimensions, we can be symmetric about the thin edge. For
                   even, we bias the extra pixel to the outside of the frame. */

                /* set up the four outer arcs first and rounded up */
                half_width_edge = (theGrafPort.pnSize.X >> 1);
                g_rect.Xmin -= half_width_edge;
                g_rect.Xmax += half_width_edge;

                half_height = (theGrafPort.pnSize.Y >> 1);
                g_rect.Ymin -= half_height;
                g_rect.Ymax += half_height;

                xRadius = ((g_rect.Xmax - g_rect.Xmin) >> 1);
                yRadius = ((g_rect.Ymax - g_rect.Ymin) >> 1);

                if( (xRadius < 0) || (yRadius < 0) )
                {
                    /* bad rectangle */
                    graf_err_value = c_FrameArc +  c_NullRect;  

                    /* report error */
                    nuGrafErr( graf_err_value, __LINE__, __FILE__);                 
                    globalLevel++;
                
                    done = NU_TRUE;

                }
                else
                {
                    /* compute center point */
                    center_rect.Xmin = g_rect.Xmin + xRadius;
                    center_rect.Ymin = g_rect.Ymin + yRadius;
                    center_rect.Xmax = g_rect.Xmax - xRadius;
                    center_rect.Ymax = g_rect.Ymax - yRadius;

                    /* round up rect for frame */
                    g_rect.Xmax++;
                    g_rect.Ymax++;

                    /* point of intersection for begin angle */
                    OvalPt( &g_rect, bgn_angle, &bgn_pt); 

                    /* point of intersection for end angle */
                    OvalPt( &g_rect, end_angle, &end_pt); 

                    /* set up the outer edges first */
                    GETPtr = &qaEdgeBuffer[0];
                    new_edge = GETPtr;

                    /* show as arc entry */
                    new_edge->countNumber = 0;   
                    EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, center_rect.Xmin, center_rect.Ymin, -1, -1);

                    new_edge = &qaEdgeBuffer[1];
                    EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, (center_rect.Xmax +1), center_rect.Ymin, 1, 1);

                    /* show as arc entry */
                    new_edge->countNumber = 0;   

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge); 

                    new_edge = &qaEdgeBuffer[2];
                    EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, center_rect.Xmin, center_rect.Ymax, 1, -1);

                    /* show as arc entry */
                    new_edge->countNumber = 0;   

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge); 

                    new_edge = &qaEdgeBuffer[3];
                    EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, (center_rect.Xmax +1), center_rect.Ymax, -1, 1);

                    /* show as arc entry */
                    new_edge->countNumber = 0;   

                    /* insert the new edge in GET, YX sorted */
                    AddToGETYXSorted( new_edge); 

                    new_edge = &qaEdgeBuffer[4];

                    /* if the outer arcs are flat, the current GET is all we need */
                    if( qaEdgeBuffer[0].Count != 1 )
                    {
                        /* need inner edges too */

                        /*Inner arcs are pulled in from frame edge by pen radii. For odd
                          pen dimensions, we can be symmetric between inner and outer edge
                          displacements.  For even pen dimensions, we bias the extra pixel
                          to the outside. */
                        half_width_edge = ( (theGrafPort.pnSize.X - 1) >> 1);
                        inner_rect.Xmin = g_base_rect.Xmin + half_width_edge;
                        inner_rect.Xmax = g_base_rect.Xmax - half_width_edge;

                        half_height = ( (theGrafPort.pnSize.Y - 1) >> 1);
                        inner_rect.Ymin = g_base_rect.Ymin + half_height;
                        inner_rect.Ymax = g_base_rect.Ymax - half_height;

                        /* width */
                        rXWidth = inner_rect.Xmax - inner_rect.Xmin;    

                        /* height */
                        rXHeight = inner_rect.Ymax - inner_rect.Ymin;   
                        if( (rXWidth > 0) && (rXHeight > 0) )
                        {
                            xRadius = rXWidth >> 1;
                            yRadius = rXHeight >> 1;

                            /* left edge + height */
                            thisLeftEdge   = inner_rect.Xmin + xRadius;    

                            /* top edge + height */
                            nextTopEdge    = inner_rect.Ymin + yRadius;    

                            /* shift right by 1 to compensate for the filler not drawing right edges */
                            thisRightEdge  = (inner_rect.Xmax - xRadius) + 1;

                            /* bottom edge - height */
                            nextBottomEdge = inner_rect.Ymax - yRadius;    

                            /* set up the upper left arc */
                            new_edge = &qaEdgeBuffer[4];
                            EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, thisLeftEdge, nextTopEdge, -1, 1);

                            /* set up the upper right arc */
                            new_edge = &qaEdgeBuffer[5];
                            EDGES_rsSetupQuadrantArc( new_edge, xRadius, yRadius, thisRightEdge, nextTopEdge, 1, -1);

                            /* set up the lower left arc */
                            new_edge = &qaEdgeBuffer[6];
                            EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, thisLeftEdge, nextBottomEdge, 1, 1);

                            /* set up the upper right arc */
                            new_edge = &qaEdgeBuffer[7];
                            EDGES_rsSetupBottomQuadrantArc( new_edge, xRadius, yRadius, thisRightEdge, nextBottomEdge, -1, -1);

                            /* Now shift the edges down and right 1, to compensate for filler
                               characteristics and to make 1 wide be really 1 wide, and add to the GET. */
                            new_edge = &qaEdgeBuffer[4];

                            /* shift 1 to the right */
                            new_edge->CurrentX++;        

                            /* shift down 1 */
                            new_edge->StartY++;          

                            /* skip the bottom most point */
                            new_edge->Count--;           

                            /* show as arc entry */
                            new_edge->countNumber = 0;   

                            /* insert the new edge in GET, YX sorted */
                            AddToGETYXSorted( new_edge); 

                            new_edge = &qaEdgeBuffer[5];

                            /* shift 1 to the left */
                            new_edge->CurrentX--;         

                            /* shift down 1 */
                            new_edge->StartY++;           
                            new_edge->Count--;   

                            /* show as arc entry */
                            new_edge->countNumber = 0;    

                            /* insert the new edge in GET, YX sorted */
                            AddToGETYXSorted( new_edge);  

                            new_edge = &qaEdgeBuffer[6];

                            /* shift 1 to the right */
                            new_edge->CurrentX++;           

                            /* advance Y by 1 (skip first point) */
                            new_edge->StepVector( new_edge); 
                            new_edge->Count--;

                            /* show as arc entry */
                            new_edge->countNumber = 0;      

                            /* insert the new edge in GET, YX sorted */
                            AddToGETYXSorted( new_edge);    

                            new_edge = &qaEdgeBuffer[7];

                            /* shift 1 to the left */
                            new_edge->CurrentX--;            

                            /* advance Y by 1 (skip first point) */
                            new_edge->StepVector( new_edge);  

                            /* skip the topmost point */
                            new_edge->Count--;               

                            /* show as arc entry */
                            new_edge->countNumber = 0;       

                            /* insert the new edge in GET, YX sorted */
                            AddToGETYXSorted( new_edge);     

                            /* point to next table entry */
                            new_edge = &qaEdgeBuffer[8];     
                        }
                    }
                }
            }
        }

        /* Ready to process line insertions */
        if ( !done )
        {
            /* set up point table */
            point_tbl[0]  = center_rect.Xmin;
            point_tbl[1]  = center_rect.Ymin;
            point_tbl[2]  = center_rect.Xmax;
            point_tbl[3]  = center_rect.Ymax;
            point_tbl[4]  = g_rect.Xmin;
            point_tbl[5]  = g_rect.Ymin;
            point_tbl[6]  = g_rect.Xmax;
            point_tbl[7]  = g_rect.Ymax;
            point_tbl[8]  = bgn_pt.X;
            point_tbl[9]  = bgn_pt.Y;
            point_tbl[10] = end_pt.X;
            point_tbl[11] = end_pt.Y;

            for( type_ptr = 0; type_ptr < 5; type_ptr++)
            {
                /* vertical or line edge entry? */
                if( type_tbl[arc_type][type_ptr][AETYPE] == VT )
                {
                    /* Enter vertical edge */
                    line_height = ( point_tbl[type_tbl[arc_type][type_ptr][AEENDY]]
                                 - point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]] );

                    /* skip if 0 */
                    if( line_height > 0 )
                    {
                        /* insert end line edge */
                        EDGES_rsSetupVerticalLineEdge( (Vedge *)new_edge, point_tbl[type_tbl[arc_type][type_ptr][AEBGNX]],
                                                       point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]],
                                                       line_height,type_tbl[arc_type][type_ptr][AEDIR] );

                        new_edge->countNumber =1;

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted( new_edge); 
                        new_edge++;
                    }
                }
                else
                {
                    /* Enter line edge */
                    /* skip if bgnY == endY */
                    if( point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]]
                        !=
                        point_tbl[type_tbl[arc_type][type_ptr][AEENDY]] )
                    {
                        /* insert the line edge */
                        EDGES_rsSetupStraightLineEdge( (lineEdgeV *)new_edge,
                                                        point_tbl[type_tbl[arc_type][type_ptr][AEBGNX]],
                                                        point_tbl[type_tbl[arc_type][type_ptr][AEBGNY]],
                                                        point_tbl[type_tbl[arc_type][type_ptr][AEENDX]],
                                                        point_tbl[type_tbl[arc_type][type_ptr][AEENDY]],
                                                        type_tbl[arc_type][type_ptr][AEDIR] );

                        new_edge->countNumber =1;

                        /* insert the new edge in GET, YX sorted */
                        AddToGETYXSorted( new_edge);  
                        new_edge++;
                    }
                }

                if( type_tbl[arc_type][type_ptr][AEEND] == TEND )
                {
                    break; /* for( ) loop */
                }
            }

            /* draw the arc and done! */
            EDGES_rsScansAndFillsEdgeList( (VOID **)&GETPtr, (blitRcd *) &rFillRcd, fillRcdSize, cmplx, -1, 0);
            globalLevel++;

            done = NU_TRUE;
        }

    } 
}


#ifdef  DASHED_LINE_SUPPORT
/***************************************************************************
* FUNCTION
*
*    ARCSD_ArcDash
*
* DESCRIPTION
*
*    Draws a dashed arc.  The circumference of the arc oval is computed and
*    used to calculate dash lengths in degrees. Multiple ARCSD_FrameArc 
*    calls are used to draw the dash sequences.
*
* INPUTS
*
*    rect * bound_rect - bounding rectangle
*
*    INT32 bgn_angle   - beginning angle 
*
*    INT32 arc_angle   - arc angle
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID ARCSD_ArcDash( rect *bound_rect, INT16 bgn_angle, INT16 arc_angle)
{
    INT16 end_ang;
    INT32 pix_degree;

    /* elements in dash on/off table */
    INT32 dash_list_size;    

    /* pointer to on/off byte list */
    const UINT8 *dash_on_off_list;  

    /* current index into on/off table */
    INT16 cur_dash_index;      

    /* current state of dashing on/off */
    INT16 cur_dash_flag;      

    /* current position in dash sequence */
    INT16 cur_dash_count;      

    /* dash sequence count */
    INT16 dash_sequence_count;      

    /* turn off dash flag so we don't call ourselves again */
    theGrafPort.pnFlags &= ~pnDashFlg;

    /* # of dash elements in dashList */
    dash_list_size = theGrafPort.pnDashRcd[theGrafPort.pnDash].dashSize;

    /* point to the dash list */
    dash_on_off_list = theGrafPort.pnDashRcd[theGrafPort.pnDash].dashList;

    cur_dash_index = theGrafPort.pnDashNdx;
    cur_dash_flag = theGrafPort.pnFlags & pnDashState;
    cur_dash_count = theGrafPort.pnDashCnt;

    /* compute end_angle */
    end_ang = bgn_angle + arc_angle;

    /* Calculate how many pixels (times 100) per degree around the circumference.
       Average the X and Y radius to simplify the calculations for an oval. */
    pix_degree = 314 * (((bound_rect->Xmax - bound_rect->Xmin) + (bound_rect->Ymax - bound_rect->Ymin)) >> 1)
                / 3600;

    /* protect from later div by 0 */
    if( pix_degree == 0 )
    {
        pix_degree = 1;
    }

    /* pick up first partial dash sequence */
    dash_sequence_count = *(dash_on_off_list + cur_dash_index);
    if( dash_sequence_count > cur_dash_count )
    {
        dash_sequence_count -= cur_dash_count;
    }

    /* until arc complete */
    do  
    {
        /* compute dash length in degrees */
        dash_sequence_count *= 100; 

        /* divide by pixels per degree (times 100) */
        dash_sequence_count /= pix_degree;    

        /* if the size of the next segment goes past the end of the arc, draw
           to the end of the arc. */
        if( (dash_sequence_count + bgn_angle) > end_ang )
        {
            /* compute partial dash sequence done */
            dash_sequence_count = end_ang - bgn_angle;
            cur_dash_count = (INT16)(dash_sequence_count * pix_degree / 100);
        }

        /* which dash state are we in? */
        if( cur_dash_flag == 0 )
        {
            /* draw a foreground dash sequence */
            ARCSD_FrameArc( bound_rect, bgn_angle, dash_sequence_count);
        }
        else
        {
            /* draw a background dash sequence */
            /* test double-dash */
            if( theGrafPort.pnFlags & pnDashStyle)
            {
                /* draw in back pattern */
                grafBlit.blitPat = theGrafPort.bkPat;
                ARCSD_FrameArc( bound_rect, bgn_angle, dash_sequence_count);
                grafBlit.blitPat = theGrafPort.pnPat;
            }
        }

        /* continue from where we left off */
        bgn_angle += dash_sequence_count;

        /* no, so flip dash state */
        cur_dash_flag ^= pnDashState;  

        /* advance to next dash sequence */
        cur_dash_index++;

        /* end of list? */
        if( cur_dash_index >= dash_list_size )
        {
            /* yes, so start list over */
            cur_dash_index = 0;
        }

        dash_sequence_count = *(dash_on_off_list + cur_dash_index);

    } while( bgn_angle < end_ang ); /* done? */

    /* update user port */
    thePort->pnDashNdx = cur_dash_index;
    thePort->pnFlags   = cur_dash_flag | pnDashFlg;
    thePort->pnDashCnt = cur_dash_count;

    /* update the dash record */
    theGrafPort.pnDashNdx = cur_dash_index;
    theGrafPort.pnFlags  |= thePort->pnFlags;
    theGrafPort.pnDashCnt = cur_dash_count;

}

#endif  /* DASHED_LINE_SUPPORT */


