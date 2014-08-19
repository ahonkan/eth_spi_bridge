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
*  portops.c                                                    
*
* DESCRIPTION
*
*  Port operation functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PORTOPS_rsDefaultPenValues
*  InitPort
*  PlaneMask
*  GetPort
*  PortBitmap
*  SetPort
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  portops.h
*  fonti.h
*  rs_api.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/portops.h"
#include "ui/fonti.h"
#include "ui/rs_api.h"

/***************************************************************************
* FUNCTION
*
*    PORTOPS_rsDefaultPenValues
*
* DESCRIPTION
*
*    Function PORTOPS_rsDefaultPenValues set the pen default values. Any and 
*    all changes will happen here to avoid system sync problems. Notice that
*    this module assumes the relevant pen data areas in the grafPort have been
*    zeroed.
*
* INPUTS
*
*    rsPort *penPort - Pointer to pen port.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PORTOPS_rsDefaultPenValues(rsPort *penPort)
{
    /* Manually set the non zero pen fields */
    /* solidOn */
    penPort->pnPat     = 1;             

    /* default pen to square */
    penPort->pnFlags   = pnShapeFlg;    
    penPort->pnColor   = -1;         

#ifdef  DASHED_LINE_SUPPORT
    
    /* default dash style records */
    penPort->pnDashRcd = &DashTable[0]; 

#endif  /* DASHED_LINE_SUPPORT */

}

/***************************************************************************
* FUNCTION
*
*    InitPort
*
* DESCRIPTION
*
*    Function InitPort initializes the specified rsPort, argPORT, and makes
*    it the current port.  The port fields are initialized as follows:
*
*    NAME        TYPE        INITIAL SETTING
*    -------     -------     -------------------------------------
*    portMap     farptr      default grafMap
*    portRect    rect        default grafMap limits
*    portOrgn    point       (0,0)
*    portVirt    rect        default grafMap limits
*    portClip    rect        default grafMap limits
*    portRgn     farptr      nil (0)
*    portPat     farptr      default pattern list
*    portMask    SIGNED        -1 = all enabled
*    portU2GP    VOID *()    
*    portU2GR    VOID *()    
*    portG2UP    VOID *()    
*    portG2UR    VOID *()    
*    portFlags   INT16       origin upperleft
*    bkPat       SIGNED      0 = solid back color
*    bkColor     SIGNED      0 
*    pnColor     SIGNED      -1
*    pnLoc       point       (0,0)
*    pnSize      point       (0,0) = thin
*    pnMode      INT16       0 = replace
*    pnPat       SIGNED      1 = solid pen color
*    pnCap       INT16       0 = flat
*    pnJoin      INT16       0 = round
*    pnLevel     INT16       0 = visible
*    pnFlags     INT16       pnShapeFlg = square
*    pnDash      INT16       0 = solid
*    pnDashRcd   farptr      default dash list
*    txFont      farptr      default font buffer
*    txFace      INT16       0 = normal
*    txMode      INT16       0 = replace
*    txSpace     INT16       0
*    txBold      INT16       1
*    txScore     INT16       1
*    txUnder     INT16       1
*    txAngle     INT16       340.0 (-20.0)
*
*
*
* INPUTS
*
*    rsPort *penPort - Pointer to port to be initialized.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID InitPort(rsPort *argPORT)
{
    SIGNED  *portAdrs;
    INT32   i;
    INT32   portSize;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Zero all port fields */
    portSize = sizeof( rsPort ) >> 2;
    portAdrs = (SIGNED *) argPORT;

    for( i = 0; i < portSize; i++ )
    {
        *portAdrs++ = 0;
    }

    /* Now set up the port */
    argPORT->portMap = &defGrafMap;
    
#ifdef  FILL_PATTERNS_SUPPORT

    argPORT->portPat = &patTable;

#endif /* FILL_PATTERNS_SUPPORT */
    
    /* set portRect, portClip, virtRect to grafMap limits */
    argPORT->portRect.Xmax = defGrafMap.pixWidth;
    argPORT->portClip.Xmax = defGrafMap.pixWidth;
    argPORT->portVirt.Xmax = defGrafMap.pixWidth;
    argPORT->portRect.Ymax = defGrafMap.pixHeight;
    argPORT->portClip.Ymax = defGrafMap.pixHeight;
    argPORT->portVirt.Ymax = defGrafMap.pixHeight;

    /* set upper left, rect clip on */
    argPORT->portFlags = pfUpper | pfRecClip;

    /* set default virtual coordinate routines */
    argPORT->portU2GP = COORDS_rsV2GP;
    argPORT->portU2GR = COORDS_rsV2GR;
    argPORT->portG2UP = COORDS_rsG2VP;
    argPORT->portG2UR = COORDS_rsG2VR;

    /* set default marker data */
    argPORT->mkSize.X = 1;
    argPORT->mkSize.Y = 1;
    argPORT->txBold   = 1;
    argPORT->txUnder  = 1;
    argPORT->txScore  = 1;
    argPORT->txSlant  = 3400;

#ifdef  INCLUDE_DEFAULT_FONT
    
    /* set font to default font buffer */
    argPORT->txFont = (fontRcd *) defFont;

#endif  /* INCLUDE_DEFAULT_FONT */
    
    argPORT->portMask = 0xFFFFFFFF;

    /* Set the pen default values */
    PORTOPS_rsDefaultPenValues(argPORT);

    /* Update system */
    SetPort(argPORT);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    PlaneMask
*
* DESCRIPTION
*
*    Function PlaneMask sets the "portMask" field in the current bitmap.
*
* INPUTS
*
*    SIGNED PLNMSK - Port mask.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PlaneMask(SIGNED PLNMSK)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set shadow port */
    theGrafPort.portMask = PLNMSK; 

    /* set default blit record */
    grafBlit.blitMask = PLNMSK; 

    /* set user port */
    thePort->portMask = PLNMSK; 

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    GetPort
*
* DESCRIPTION
*
*    Function GetPort stores the current rsPort address to the pointer
*    specified by gpPORT.  Note that a pointer to the pointer is passed on
*    the stack.
*
* INPUTS
*
*    rsPort **gpPORT - Port address.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetPort(rsPort **gpPORT)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    *gpPORT = thePort;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    PortBitmap
*
* DESCRIPTION
*
*    Function PortBitmap sets the current port to reference the specified
*    grafMap for subsequent writing operations.
*
* INPUTS
*
*    grafMap *ptrBMAP - grafMap for the port..
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PortBitmap(grafMap *ptrBMAP)
{
    rect tmpRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set default blitrcd grafMap */
    grafBlit.blitDmap = ptrBMAP;

#ifdef  THIN_LINE_OPTIMIZE
    
    /* set thin line indirect vector */
    lineThinIDV = (SIGNED) ptrBMAP->prLine;

    /* refresh current line style vector */
    SETLINESTYLE(theGrafPort.pnFlags);

#endif  /* THIN_LINE_OPTIMIZE */
    
    /* set shadow copy of the port */
    theGrafPort.portMap = ptrBMAP;

    if( ptrBMAP->mapFlags & mfResetBM )
    {
        /* reset bank manager */
        ptrBMAP->mapWinYmin[0] = -1;
        ptrBMAP->mapWinYmin[1] = -1;
        ptrBMAP->mapWinYmax[0] = -1;
        ptrBMAP->mapWinYmax[1] = -1;
    }

    /* set the current ports portMap pointer */
    thePort->portMap  = ptrBMAP;

    /* re-compute global clip data in case bitmap is smaller than port */
    COORDS_rsGblClip(thePort, &tmpRect);
    ViewClip = tmpRect;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SetPort
*
* DESCRIPTION
*
*    Function SetPort sets the specified GrafPort to be the current port.
*    Also calls PortBitmap to reset the current grafMap.
*
* INPUTS
*
*    rsPort *portPtr - Pointer to the specified GrafPort.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetPort(rsPort *portPtr)
{
    INT16 grafErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( gFlags & gfRgnOpen )
    {
        /* region open */
        grafErrValue = c_SetPort + c_RgnOflow;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        if( portPtr == NU_NULL )
        {
            /* use default port if null */
            portPtr = &defPort; 
        }

        /* Place in GRAFIX_DATA:thePort */
        thePort  = portPtr;

        /* Copy thePort to grafPort shadow */
        theGrafPort = *portPtr;

        /* reset bitmap */
        PortBitmap(portPtr->portMap);

        /* update  coordinate xform data */
        COORDS_rsGblCoord();

        /* update LocX, LocY in global coords */
        COORDS_rsGblPenCoord();

        /* reset pen mode */
        PenMode((INT32)(theGrafPort.pnMode));

        /* refresh current line style vector */
        SETLINESTYLE(theGrafPort.pnFlags);

        /* reset plane mask */
        PlaneMask(theGrafPort.portMask);

        /* set blit record */
        grafBlit.blitBack = theGrafPort.bkColor;
        grafBlit.blitFore = theGrafPort.pnColor;
        grafBlit.blitPat  = theGrafPort.pnPat;
        grafBlit.blitSmap = theGrafPort.portMap;
        grafBlit.blitDmap = theGrafPort.portMap;
        
#ifdef  FILL_PATTERNS_SUPPORT
        
        grafBlit.blitPatl = theGrafPort.portPat;

#endif /* FILL_PATTERNS_SUPPORT */
        
#ifndef NO_REGION_CLIP

        /* Set the region clipping info */
        grafBlit.blitRegn = theGrafPort.portRegion;
        if( theGrafPort.portFlags & pfRgnClip )
        {
            /* regions enabled - set flag for regions */
            grafBlit.blitFlags = grafBlit.blitFlags | bfClipRegn;
        }
        else
            
#endif  /* NO_REGION_CLIP */
            
        {
            /* regions not enabled - clear region flag */
            grafBlit.blitFlags = grafBlit.blitFlags & ~bfClipRegn;
        }

        /* set font */
        SetFont( (fontRcd *)theGrafPort.txFont );

    } /* else */

    /* Return to user mode */
    NU_USER_MODE();
}
