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
*  peni.c                                                       
*
* DESCRIPTION
*
*  This file contains the function RS_Pen_Setup.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Get_Pen_Setup
*  RS_Pen_Setup
*
* DEPENDENCIES
*
*  rs_base.h
*  fonti.h
*  peni.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/fonti.h"
#include "ui/peni.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    RS_Pen_Setup
*
* DESCRIPTION
*
*    Some elements of the thePenSetUp structure need to be set before calling
*    this function.
*
*    The following structure is defined in fonti.h.
*
*        typedef struct _PenSetUp
*        {
*            ShapeOfPen  penShape;
*            INT16       penWidth;
*            INT16       penHeight;
*            INT32       penDash;
*            INT32       penCap;
*            INT32       penJoin;
*            INT32       patt;
*        } PenSetUp;
*
*        PenSetUp     thePenSetUp;
*
*    a. thePenSetUp.penShape
*    
*       To set the shape of the pen, there are three types:
*           shapeRect = 0
*           shapeOval = 1
*           and Normal.
*           Default = Normal.
*
*       EX. thePenSetUp.penShape = Normal;
*
*    b. thePenSetUp.penWidth
*       thePenSetUp.penHeight
*
*       To set the pen size, set width and height in the penSetUp structure.
*       Default preset value = -1.
*
*       EX. thePenSetUp.penWidth = 2;
*           thePenSetUp.penHeight = 4;
*            
*    c. thePenSetUp.penDash
*
*       To set the penDash, a DASH has to already be defined.
*       If a dash has been defined then it can be set.
*       Default preset value = -1
*
*       EX. thePenSetUp.penDash = 2;
*
*    d. thePenSetUp.penCap
*
*       To set the penCap, it can only be set to three types: 
*           capFlat   = 0
*           capSquare = 2
*           capRound  = 3
*       Default preset value = -1
*
*       EX. thePenSetUp.penCap = capFlat;
*
*    e. thePenSetUp.penJoin
*
*       To set the penJoin, if can be set to three types:
*           joinRound = 0
*           joinBevel = 1
*           joinMiter = 2
*       Default preset value = -1
*
*       EX. thePenSetUp.penJoin = joinBevel;
*
*    f. thePenSetUp.patt
*
*       To set the patt, a fill pattern structure is set up with 32 patterns to use,
*       the value to set this to has to be 0 to 31, 
*       Default preset value = -1
*
*       EX. thePenSetUp.patt = 8;
*
* INPUTS
*
*    INT32 penColor - Pen color.
*                     EX. Black, Blue, Green, Cyan, Red, Magenta, Brown, LtGray,
*                         Gray, LtBlue, LtGreen, LtCyan, LtRed, LtMagenta, Yellow, White.
*                     Alternately, a specific numeric value for a color can be used.
*    PenSetUp *penPtr - Points to the structure containing new values for the global object.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RS_Pen_Setup(PenSetUp *penPtr, INT32 penColor)
{
    INT16  JumpPS080 = NU_FALSE;    
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* Copy the new value for the text setup to the global object. */
    if (penPtr != NU_NULL)
    {
        thePenSetUp = *penPtr;
    }

    if( thePenSetUp.penShape > 0 )
    {
        if( thePenSetUp.penShape != shapeRect )
        {
            /* clear shape flag */
            theGrafPort.pnFlags &= ~pnShapeFlg;
        }   
        else
        {
            /* mark rectangular */
            theGrafPort.pnFlags |= pnShapeFlg;
        }

        /* update user port */
        thePort->pnFlags = theGrafPort.pnFlags;

        /* smart link oval poly lines */
        lineOvPolyIDV = (SIGNED) LINE_rsOvalPolyLines;   

        /* smart link oval lines */
        lineOvalIDV = (SIGNED) PENS_rsOvalLineGeneric;     

        /* refresh current line style vector */
        SETLINESTYLE(theGrafPort.pnFlags); 
    }
    else
    {
        /* Normal pen shape, default */
        /* save current pen location */
        thePort->pnLoc.X   = theGrafPort.pnLoc.X; 
        thePort->pnLoc.Y   = theGrafPort.pnLoc.Y;
        thePort->bkPat     = 0;
       
        thePort->pnSize.X  = 0;
        thePort->pnSize.Y  = 0;
        thePort->pnLevel   = 0;
       
        thePort->pnCap     = 0;
        thePort->pnJoin    = 0;
        thePort->pnMiter   = 0;
        
#ifdef  DASHED_LINE_SUPPORT
        
        thePort->pnDash    = 0;
        thePort->pnDashNdx = 0;
        thePort->pnDashCnt = 0;

#endif  /* DASHED_LINE_SUPPORT */
        
        /* set default pen */
        PORTOPS_rsDefaultPenValues(thePort);     
    }

    /* set shadow port */
    theGrafPort.pnColor = penColor;    

    /* set default blit record */
    grafBlit.blitFore = penColor;  

    /* set user port */
    thePort->pnColor = penColor;   

    if( thePenSetUp.penMiter > 0 )
    {
        /* set user port pen miter */
        thePort->pnMiter = thePenSetUp.penMiter; 

        /* set shadow port pen miter */
        theGrafPort.pnMiter = thePenSetUp.penMiter; 
    }

#ifdef  THIN_LINE_OPTIMIZE

    if( (thePenSetUp.penHeight >= 0) && (thePenSetUp.penWidth >= 0) )
        
#endif  /* THIN_LINE_OPTIMIZE */

    {
        
#ifdef  THIN_LINE_OPTIMIZE

        /* test for thin case, either size = 0 before scaling */
        if( (thePenSetUp.penWidth == 0) || (thePenSetUp.penHeight == 0) )
        {
            /* set thin pen */
            
            /* is region open? */
            if( gFlags & gfRgnOpen)
            {
                /* region open and thin pen requested, set pen to 1,1 */
                thePenSetUp.penWidth  = 1;
                thePenSetUp.penHeight = 1;

#ifndef NO_REGION_CLIP
                
                /* make CloseRegion restore thin pen */
                regPenFlags &= ~pnSizeFlg;
                
#endif  /* NO_REGION_CLIP */
                
            }
            else
            {
                /* clear thick flag */
                thePort->pnFlags &= ~pnSizeFlg; 
                theGrafPort.pnFlags &= ~pnSizeFlg;

                /* update port pen size to 0,0 */
                thePenSetUp.penWidth  = 0;
                thePenSetUp.penHeight = 0;
                JumpPS080 = NU_TRUE;
            }
        }
        else
            
#endif  /* THIN_LINE_OPTIMIZE */
            
        {       
            /* test if virtual coordinates */
            if( theGrafPort.portFlags & pfVirtual ) 
            {
                /* Virtual To Global size */
                V2GSIZE(thePenSetUp.penWidth, 
                        thePenSetUp.penHeight, 
                       &thePenSetUp.penWidth, 
                       &thePenSetUp.penHeight);
            }
    
            /* test if height or width less than 1 */
            if( thePenSetUp.penWidth < 1 )
            {
                /* Set to default widthx value */
                /* width = 1 */
                thePenSetUp.penWidth = 1;   
            }
        
            if( thePenSetUp.penHeight < 1 )
            {
                /* Set to default heighty value */
                /* height = 1 */
                thePenSetUp.penHeight = 1;  
            }
        }

        if( !JumpPS080 )
        {
            /* set thick line flag */
            thePort->pnFlags |= pnSizeFlg;  
            theGrafPort.pnFlags |= pnSizeFlg;

#ifndef NO_REGION_CLIP
            
            /* make CloseRegion leave pen alone */
            regPenFlags |= pnSizeFlg;       

#endif  /* NO_REGION_CLIP */

        }

        /* update user port */
        thePort->pnSize.X = thePenSetUp.penWidth;
        thePort->pnSize.Y = thePenSetUp.penHeight;

        /* update shadow port */
        theGrafPort.pnSize.X = thePenSetUp.penWidth;
        theGrafPort.pnSize.Y = thePenSetUp.penHeight;

         /* smart link non-dashed square    lines */
        linePattIDV = (SIGNED) LSPC_rsSpecialLinePatternAndSquare;      

        /* refresh current line style vector */
        SETLINESTYLE(theGrafPort.pnFlags);    

        /* smart link square pen PolyLine */
        lineSqPolyIDV = (SIGNED) POLYGONS_rsSuperSetPolyLineDrawer; 
    }

#ifdef  DASHED_LINE_SUPPORT
    
    /* This has to be defined outside of this function before use */
    if( thePenSetUp.penDash >= 0 ) 
    {
        if( thePenSetUp.penDash > 7 )
        {
            thePenSetUp.penDash = 1;
        }

        if( thePenSetUp.penDash == 0 )
        {
            /* clear dashing flag */
            theGrafPort.pnFlags &= ~pnDashFlg;
        }
        else
        {
            /* set dashing flag */
            theGrafPort.pnFlags |= pnDashFlg;
        }

        /* set shadow port */
        theGrafPort.pnDash = thePenSetUp.penDash;

        /* reset the dash sequence to the start */
        theGrafPort.pnDashCnt = 0; 

        /* smart link dashed lines */
        lineDashIDV = (SIGNED) LSPC_rsDashThinLine;   

        /* refresh current line style vector */
        SETLINESTYLE(theGrafPort.pnFlags); 

        /* refresh the ports pattern and penflags */
        thePort->pnDash    = thePenSetUp.penDash;  
        thePort->pnDashCnt = 0;
        thePort->pnFlags   = theGrafPort.pnFlags;
    }

#endif  /* DASHED_LINE_SUPPORT */
    
    /* This is 0, 1 or 2 unless user defined*/
    if( thePenSetUp.penCap >= 0 ) 
    {
        if( thePenSetUp.penCap > 3 )
        {
            thePenSetUp.penCap = 0;
        }

        /* set shadow port */
        theGrafPort.pnCap = thePenSetUp.penCap;

        /* set user port */
        thePort->pnCap = thePenSetUp.penCap;
    }

    /* This is 0, 1, or 2 unless user defined */
    if( thePenSetUp.penJoin >= 0 )
    {
        if( thePenSetUp.penJoin > 2 )
        {
            /* report an error and select join #0 */
            thePenSetUp.penJoin = 0;
        }

        /* set shadow port */
        theGrafPort.pnJoin = thePenSetUp.penJoin;

        /* set user port */
        thePort->pnJoin = thePenSetUp.penJoin;
    }

    if( thePenSetUp.patt >= 0 ) 
    {
        if( thePenSetUp.patt > 32)
        {
            /* error! */
            /* make solid pen */
            thePenSetUp.patt = 1; 
        }

        /* check if pattern already set */
        if( thePenSetUp.patt != theGrafPort.pnPat ) 
        {
            /* change it if not */
            if( thePenSetUp.patt > 1 )
            {
                /* set patterned pen flag */
                theGrafPort.pnFlags |= pnPattFlg;
            }
            else
            {
                /* clear patterned pen flag */
                theGrafPort.pnFlags &= ~pnPattFlg;
            }

            /* set shadow port */
            theGrafPort.pnPat = thePenSetUp.patt;

            /* set default blitRcd */
            grafBlit.blitPat = thePenSetUp.patt;

            /* smart link patterned lines */
            linePattIDV = (SIGNED) LSPC_rsSpecialLinePatternAndSquare;      

            /* refresh current line style vector */
            SETLINESTYLE(theGrafPort.pnFlags);    

            /* refresh the ports pattern and penflags */
            thePort->pnPat = thePenSetUp.patt; 
            thePort->pnFlags = theGrafPort.pnFlags;
        }
    }

    /* SetPort will resync everything */
    SetPort(thePort);   

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    RS_Get_Pen_Setup
*
* DESCRIPTION
*
*    Function RS_Get_Pen_Setup returns back the current pen setup.
*
* INPUTS
*
*    PenSetUp *penPtr - Points to the structure where current values of the global object is tobe placed.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RS_Get_Pen_Setup(PenSetUp *penPtr)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (penPtr != NU_NULL)
    {
        *penPtr = thePenSetUp;
    }

    /* Return to user mode */
    NU_USER_MODE();
}
