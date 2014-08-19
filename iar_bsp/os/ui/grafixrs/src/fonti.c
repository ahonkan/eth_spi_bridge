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
*  fonti.c                                                      
*
* DESCRIPTION
*
*  Pen and Text setup functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Init_Text_And_Pen
*  RS_Reset_Pen
*  RS_Reset_Text
*
* DEPENDENCIES
*
*  rs_base.h
*  edges.h
*  rsfonts.h
*  fonti.h
*  texti.h
*  textd.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/edges.h"
#include "ui/rsfonts.h"
#include "ui/fonti.h"
#include "ui/texti.h"
#include "ui/textd.h"

extern VOID V2GSIZE(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);
extern VOID SetPort(rsPort *portPtr);
extern INT32 BMAPF_TextAlignHorizontalCR(signed char *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE);

/***************************************************************************
* FUNCTION
*
*    RS_Init_Text_And_Pen
*
* DESCRIPTION
*
*    Function RS_Init_Text_And_Pen is called at Initialization so that the 
*    structures can be used through out.
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
VOID RS_Init_Text_And_Pen(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* These are the defaults for the Pen */
    /*  0 global.h */
    thePenSetUp.penShape  = NORMAL;        

    /* DEFAULT_VALUE -1 global.h */
    thePenSetUp.penWidth  = DEFAULT_VALUE; 
    thePenSetUp.penHeight = DEFAULT_VALUE; 
    
#ifdef  DASHED_LINE_SUPPORT

    thePenSetUp.penDash   = DEFAULT_VALUE;
	
#endif  /* DASHED_LINE_SUPPORT */

    thePenSetUp.penCap    = DEFAULT_VALUE; 
    thePenSetUp.penJoin   = DEFAULT_VALUE; 
    thePenSetUp.patt      = DEFAULT_VALUE; 

    /*  0 global.h */
    thePenSetUp.penMiter  = NORMAL;        

    /* These are the defaults for the Text */
    theTextSetUp.boldFactor          = 1;
    theTextSetUp.face                = cNormal;

    /* DEFAULT_VALUE -1 global.h */
    theTextSetUp.mode                = DEFAULT_VALUE; 
    theTextSetUp.SPACEExtraSpace     = 0;
    theTextSetUp.charExtraSpace      = 0;
    theTextSetUp.underlineGap        = 1;
    theTextSetUp.descentUnderlineGap = 1;
    theTextSetUp.charPath            = pathRight;
    theTextSetUp.charHorizontalAlign = alignLeft;
    theTextSetUp.charVerticalAlign   = alignBaseline;
	
#ifdef      USE_STROKEDFONT

    theTextSetUp.charWidth           = 0;
    theTextSetUp.charHeight          = 0;

    /* DEFAULT_VALUE -1 global.h */
    theTextSetUp.charAngle           = DEFAULT_VALUE; 
    theTextSetUp.charSlant           = -200;
	
#endif      /* USE_STROKEDFONT */

    theTextSetUp.markerCharAngle     = 4000;
    theTextSetUp.markerCharStyle     = 0;
    theTextSetUp.markerCharWidth     = 0;
    theTextSetUp.markerCharHeight    = 0;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    RS_Reset_Pen
*
* DESCRIPTION
*
*    Function RS_Reset_Pen sets pen attributes to the default values.
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
VOID RS_Reset_Pen(VOID)
{
    /* These are the defaults for the Pen */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Normal pen shape, default */
    /*  NORMAL 0 global.h */
    thePenSetUp.penShape  = NORMAL;      

    /* save current pen location */
    thePort->pnLoc.X = theGrafPort.pnLoc.X; 
    thePort->pnLoc.Y = theGrafPort.pnLoc.Y;
    
    thePort->bkPat = 0;

    thePort->bkColor = 0;

    /* DEFAULT_VALUE -1 global.h */
    thePenSetUp.penWidth  = DEFAULT_VALUE; 
    thePenSetUp.penHeight = DEFAULT_VALUE; 

    /* update user port */
    thePort->pnSize.X = 0;
    thePort->pnSize.Y = 0;
    
    /* update shadow port */
    theGrafPort.pnSize.X = 0;
    theGrafPort.pnSize.Y = 0;
    
    /* clear thick flag */
    thePort->pnFlags &= ~pnSizeFlg;
    theGrafPort.pnFlags &= ~pnSizeFlg;

    thePort->pnLevel = 0;
 
    thePort->pnMode = 0;
  
    /* DEFAULT_VALUE -1 global.h */
    thePenSetUp.penCap = DEFAULT_VALUE; 
    thePort->pnCap = 0;
    theGrafPort.pnCap = 0;
 
    /* DEFAULT_VALUE -1 global.h */
    thePenSetUp.penJoin = DEFAULT_VALUE; 
    thePort->pnJoin = 0;
    theGrafPort.pnJoin = 0;

    /*  NORMAL 0 global.h */
    thePenSetUp.penMiter = NORMAL; 
    thePort->pnMiter     = 0;

    /* set shadow port pen miter */
    theGrafPort.pnMiter     = 0;      

#ifdef  DASHED_LINE_SUPPORT
    
    /* DEFAULT_VALUE -1 global.h */
    thePenSetUp.penDash = DEFAULT_VALUE; 
    thePort->pnDash = 0;
    theGrafPort.pnDash = 0;

    /* clear dashing flag */
    theGrafPort.pnFlags &= ~pnDashFlg;
    thePort->pnFlags = theGrafPort.pnFlags;

    /* reset the dash sequence to the start */
    thePort->pnDashCnt = 0;
    theGrafPort.pnDashCnt = 0;

    thePort->pnDashNdx = 0;

#endif  /* DASHED_LINE_SUPPORT */
    
     /* set shadow port */
    theGrafPort.pnColor  = -1;

    /* set default blit record */
    grafBlit.blitFore = -1; 

    /* set user port */
    thePort->pnColor  = -1; 

    /* DEFAULT_VALUE -1 global.h */
    thePenSetUp.patt = DEFAULT_VALUE; 
    thePort->pnPat   = 1;
    theGrafPort.pnPat   = 1;
    grafBlit.blitPat = 1;

    /* clear patterned pen flag */
    theGrafPort.pnFlags &= ~pnPattFlg;
    thePort->pnFlags = theGrafPort.pnFlags;

    /* SetPort will resync everything */    
    SetPort(thePort);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    RS_Reset_Text
*
* DESCRIPTION
*
*    Function RS_Reset_Text sets text attributes to the default values.
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
VOID RS_Reset_Text(VOID)
{
#ifdef      USE_STROKEDFONT
    fontRcd *theFont;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* These are the defaults for the Text */
    theTextSetUp.charPath = pathRight;

    theTextSetUp.markerCharWidth  = 0;
    theTextSetUp.markerCharHeight = 0;
    theGrafPort.mkSize.X = 0;
    theGrafPort.mkSize.Y = 0;
    thePort->mkSize.X = 0;
    thePort->mkSize.Y = 0;

    theTextSetUp.markerCharStyle = 0;
    theGrafPort.mkType = 0;
    thePort->mkType = 0;

    theTextSetUp.markerCharAngle     = 4000;
    theGrafPort.mkAngle = 4000;

#ifdef      USE_STROKEDFONT

    theFont = (fontRcd *) grafPort.txFont;

    /* smart link stroked text draw */
    txtStrokeIDV = (INT32 (*)())STRKFONT_rsStrokeFontInit;

    if( theFont->fontFlags & fontType )
    {
        txtDrwIDV = txtStrokeIDV;
        if( theGrafPort.txAlign.X == 0 )
        {
            txtAlnIDV = txtStrokeIDV;
        }
    }

    if( theGrafPort.portFlags & pfVirtual )
    {
        /* V2GSIZE Virtual To Global size */
        V2GSIZE(      theTextSetUp.charWidth, 
                      theTextSetUp.charHeight,
                    &(theTextSetUp.charWidth),
                    &(theTextSetUp.charHeight) );
    }

    theTextSetUp.charWidth  = 0;
    theTextSetUp.charHeight = 0;
    theGrafPort.txSize.X = 0;
    theGrafPort.txSize.Y = 0;
    thePort->txSize.X = 0;
    thePort->txSize.Y = 0;

#endif      /* USE_STROKEDFONT */
    
    theTextSetUp.face = cNormal;
    theGrafPort.txFace = cNormal;
    thePort->txFace = cNormal;

    theTextSetUp.boldFactor = 1;
    theGrafPort.txBold = 1;
    thePort->txBold = 1;

    theTextSetUp.underlineGap = 1;
    theGrafPort.txUnder = 1;
    thePort->txUnder = 1;

    theTextSetUp.descentUnderlineGap = 1;
    theGrafPort.txScore = 1;
    thePort->txScore = 1;

#ifdef      USE_STROKEDFONT

    theTextSetUp.charSlant = -200;
	
#endif      /* USE_STROKEDFONT */

    theGrafPort.txSlant = -200;
    thePort->txSlant = -200;  

    /* DEFAULT_VALUE -1 global.h */
    theTextSetUp.mode = DEFAULT_VALUE; 
    theGrafPort.txMode  = 0;
    grafBlit.blitRop = 0;
    thePort->txMode  = 0;

    /* DEFAULT_VALUE -1 global.h */
#ifdef      USE_STROKEDFONT

    theTextSetUp.charAngle = DEFAULT_VALUE; 
    theGrafPort.txAngle = -1;
    thePort->txAngle = -1;
	
#endif      /* USE_STROKEDFONT */

    theTextSetUp.SPACEExtraSpace = 0;
    theGrafPort.txSpace = 0;
    thePort->txSpace = 0;

    theTextSetUp.charExtraSpace = 0;
    theGrafPort.txExtra = 0;
    thePort->txExtra = 0;

    /* Text Align */
    /* 0 */
    theTextSetUp.charHorizontalAlign = alignLeft;
    theTextSetUp.charVerticalAlign   = alignBaseline;

    txtAPPIDV = (INT32 (*)())BMAPF_TextAlignHorizontalCR;
    txtAlnIDV = txtDrwIDV;

    fntVertAln = fntVertTbl[alignBaseline];

    theGrafPort.txAlign.X = alignLeft;
    theGrafPort.txAlign.Y = alignBaseline;

    thePort->txAlign.X = alignLeft;
    thePort->txAlign.Y = alignBaseline;

    /* Return to user mode */
    NU_USER_MODE();

}
