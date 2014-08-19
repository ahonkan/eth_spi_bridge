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
*  polygond.c                                                   
*
* DESCRIPTION
*
*  Contains the API function: RS_Polygon_Draw.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Polygon_Draw
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  polygond.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/polygond.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    RS_Polygon_Draw
*
* DESCRIPTION
*
*    The API function RS_Polygon_Draw is called to draw various type of polygons.
*    NOTE: The graphics port must already be set up and it should be global.  
*
* INPUTS
*
*    ObjectAction - This would be the action that would be performed on the object, 
*                   this will be a list of actions in an enumerated data type.  
*                       EX: FRAME  = 0,
*                       PAINT  = 1,
*                       FILL   = 2,
*                       ERASE  = 3,
*                       INVERT = 4,  
*                       POLY   = 5  (only used by BEZIER)
*
*    INT32      - This is point count.
*
*    polyHead * - Pointer to the polygon header structure.
*
*    point *    - This is the pointer to the points to use for the polygon manipulation
*
*    INT32      - This is the Pattern, fill pattern structure that contains 32 default values 
*                 For patterns. So the value is 0 to 31. -1 if not used.
*                 This can be user Defined.
*
* OUTPUTS
*
*    INT32 minDistIndex - Returns closest index.
*
***************************************************************************/
STATUS RS_Polygon_Draw( ObjectAction action, INT32 polyPtsCount, polyHead *polyHeader,
                        point *polyPts , INT32 patt)
{
    STATUS  status = ~NU_SUCCESS;
    
    /* clip rectangle in user coordinates */
    rect    usrC;           

    /* number of points in the polygon */
    INT32   ptCnt;          

    /* size of polygon header */
    point   *bgnPtAds;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    if( action == FRAME )
    {
        for( ;polyPtsCount > 0; polyPtsCount--)
        {
            ptCnt = polyHeader->polyEnd - polyHeader->polyBgn;
            if( ptCnt <= 0 ) 
            {
                break;
            }
            ptCnt++;
            bgnPtAds = &polyPts[polyHeader->polyBgn];
            RS_Line_Draw( (INT16)-ptCnt, (point *) bgnPtAds);

            polyHeader++;
        }

        status = NU_SUCCESS;
    } /* end of: if (action == FRAME) */
    else 
    {
        if( patt  > 32 )
        {
            patt = 1; 
        }

        if( theGrafPort.pnLevel < 0 ) 
        {
            status = NU_SUCCESS;
        }

        if( (status != NU_SUCCESS) && (polyPtsCount <= 0) ) 
        {
            status = NU_SUCCESS;
        }

        if( status != NU_SUCCESS )
        {
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
				case TRANS:
					grafBlit.blitRop = xAVGx;
					break;
                default:
                    break;
            }

            /* set up the clip rectangle in user coordinates */
            usrC = ViewClip;

            /* already in global? */
            if( globalLevel > 0 )
            {
                /* convert from global to user */
                G2UR(usrC, &usrC);
            }

            for( ; polyPtsCount > 0; polyPtsCount--)
            {
                /* perform trivial clip rejection */
                if( (polyHeader->polyRect.Xmin <= usrC.Xmax) &&
                    (polyHeader->polyRect.Xmax >  usrC.Xmin) &&
                    (polyHeader->polyRect.Ymin <= usrC.Ymax) &&
                    (polyHeader->polyRect.Ymax >  usrC.Ymin) )
                {
                    ptCnt = polyHeader->polyEnd - polyHeader->polyBgn;
                    if( ptCnt <= 0 )
                    {
                        break;
                    }

                    ptCnt++;
                    
                    /* point to the start vertex for this poly */
                    bgnPtAds = &polyPts[polyHeader->polyBgn];

                    /* draw the current polygon */
                    FillPolygon( bgnPtAds, ptCnt, coordModeOrigin, cmplx);
                }

                polyHeader++;
            }

            /* restore default blit record */
            if( (action == ERASE) || (action == INVERT) )
            {
                grafBlit.blitRop = theGrafPort.pnMode;
            }

            if( (action == ERASE) || (action == FILL) )
            {
                grafBlit.blitPat = theGrafPort.pnPat;
            }
            status = NU_SUCCESS;
        }
    }  /* end of: else (action == FRAME) */ 

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(status);
}
