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
*  coords.c                                                     
*
* DESCRIPTION
*
*  Coordinate functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  Gbl2LclPt
*  Gbl2LclRect
*  Gbl2VirPt
*  Gbl2VirRect
*  Lcl2GblPt
*  Lcl2GblRect
*  Lcl2VirPt
*  Lcl2VirRect
*  Vir2GblPt
*  Vir2GblRect
*  Vir2LclPt
*  Vir2LclRect
*  Port2Gbl
*  COORDS_rsGblPenCoord
*  COORDS_rsGblClip
*  COORDS_rsVirtCoord
*  COORDS_rsGblCoord
*  COORDS_rsV2GP
*  COORDS_rsV2GR
*  COORDS_rsG2VP
*  COORDS_rsG2VR
*  U2GP
*  G2UP
*  U2GR
*  G2UR
*  V2GSIZE
*
* DEPENDENCIES
*
*  rs_base.h
*  coords.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/coords.h"

/* Local coordinate conversion constants */
static  DEFN localXconst;       
static  DEFN localYconst;
static  DEFN localXinvert;
static  DEFN localYinvert;

/* Virtual coordinate conversion factors */
static  INT32 virtXnumer;       
static  INT32 virtYnumer;
static  INT32 virtXdenom;
static  INT32 virtYdenom;
static  INT32 virtXround;
static  INT32 virtYround;
static  DEFN virtXconst;
static  DEFN virtYconst;


/***************************************************************************
* FUNCTION
*
*    Gbl2LclPt
*
* DESCRIPTION
*
*    Function Gbl2LclPt converts the specified global bitmap point, argPT,
*    into what would be local coordinates for the current grafPort.
*
*    Note: the current port may be in virtual mode, this must be ignored.
*
* INPUTS
*
*    point *argPT - Pointer to point to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Gbl2LclPt(point *argPT)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    argPT->X += (theGrafPort.portRect.Xmin - theGrafPort.portOrgn.X);

    if( theGrafPort.portFlags & pfUpper )
    {
        /* upper left:   Yl = Yg + portYmin + -portoriginY */
        argPT->Y += (theGrafPort.portRect.Ymin - theGrafPort.portOrgn.Y);
    }
    else
    {
        /* lower left:   Yl = -Yg + portYmax + portoriginY */
        argPT->Y = -argPT->Y + theGrafPort.portRect.Ymin + theGrafPort.portOrgn.Y;
    }

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Gbl2LclRect
*
* DESCRIPTION
*
*    Function Gbl2LclRect converts the specified global bitmap rectangle,
*    argR, into what would be local coordinates for the current grafPort.
*
*    Note: the current port may be in virtual mode, this must be ignored.
*
* INPUTS
*
*    rect *argR - Pointer to rect to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Gbl2LclRect(rect *argR)
{
    point tempPt;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* convert first point */
    tempPt.X = argR->Xmin;
    tempPt.Y = argR->Ymin;
    Gbl2LclPt(&tempPt);

    argR->Xmin = tempPt.X;
    argR->Ymin = tempPt.Y;
                   
    /* convert second point */
    tempPt.X = argR->Xmax;
    tempPt.Y = argR->Ymax;
    Gbl2LclPt(&tempPt);

    /* lower origin will cause Ymax < Ymin */
    if( tempPt.Y > argR->Ymin )
    {
        argR->Ymax = tempPt.Y;
    }
    else
    {
        argR->Ymax = argR->Ymin;
        argR->Ymin = tempPt.Y;
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    Gbl2VirPt
*
* DESCRIPTION
*
*    Function Gbl2VirPt converts the specified global bitmap point, argPT,
*    into what would be virtual port coordinate value.
*
* INPUTS
*
*    point *argPT - Pointer to point to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Gbl2VirPt(point *argPT)
{
    INT32 pointX;
    INT32 pointY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    pointX = argPT->X;
    pointY = argPT->Y;

    /* convert to virtual */
    theGrafPort.portG2UP(&pointX, &pointY);

    /* save it */
    argPT->X = pointX;
    argPT->Y = pointY;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    Gbl2VirRect
*
* DESCRIPTION
*
*    Function Gbl2VirRect converts the specified rectangle, argR, from the
*    current grafPort's local coordinate system into what would be virtual port
*    coordinate values.
*
* INPUTS
*
*    rect *argR - Pointer to rect to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Gbl2VirRect(rect *argR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    theGrafPort.portG2UR(argR);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Lcl2GblPt
*
* DESCRIPTION
*
*    Function Lcl2GblPt converts the specified point, argPT, from the current
*    grafPort's local coordinate system into a global bitmap coordinate value.
*    Global bitmap coordinates are referenced with the (0,0) origin in the
*    upper-left corner of the bitmap.
*
* INPUTS
*
*    point *argPT - Pointer to point to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Lcl2GblPt(point *argPT)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* add offsets */
    argPT->X += localXconst;

    /* conditionally mirror Y */
    argPT->Y ^= localYinvert;
    argPT->Y += localYconst;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    Lcl2GblRect
*
* DESCRIPTION
*
*    Function Lcl2GblRect converts the specified rectangle, argR, from the
*    current grafPort's local coordinate system into a global bitmap coordinate
*    value.  Global bitmap coordinates are referenced with the (0,0) origin in
*    the upper-left corner of the bitmap.
*
* INPUTS
*
*    rect *argR - Pointer to rect to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Lcl2GblRect(rect *argR)
{
    SIGNED tempY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* add offsets */
    argR->Xmin += localXconst;
    argR->Xmax += localXconst;
    
    /* conditionally mirror Y */
    argR->Ymin ^= localYinvert;
    argR->Ymax ^= localYinvert;
    argR->Ymin += localYconst;
    argR->Ymax += localYconst;
                   
    /* test Y orientation */
    if( (theGrafPort.portFlags & pfUpper) )
    {
        /* local is lower corner */
        tempY = argR->Ymax;
        argR->Ymax = argR->Ymin;
        argR->Ymin = tempY;
    }

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Lcl2VirPt
*
* DESCRIPTION
*
*    Function Lcl2VirPt converts the specified point, argPT, from the current 
*    grafPort's local coordinate system into a virtual port coordinate value.
*
* INPUTS
*
*    point *argPT - Pointer to point to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Lcl2VirPt(point *argPT)
{
    INT32 pointX;
    INT32 pointY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* convert to global */
    Lcl2GblPt(argPT);

    /* pickup converted values */
    pointX = argPT->X;
    pointY = argPT->Y;

    /* convert to virtual */
    theGrafPort.portG2UP(&pointX, &pointY);

    /* save it */
    argPT->X = pointX;
    argPT->Y = pointY;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Lcl2VirRect
*
* DESCRIPTION
*
*    Function Lcl2VirRect converts the specified rectangle, argR, from the
*    current grafPort's local coordinate system into virtual port coordinate
*    values.
*
* INPUTS
*
*    rect *argR - Pointer to rect to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Lcl2VirRect(rect *argR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* convert to global */
    Lcl2GblRect(argR);

    /* convert to virtual */
    theGrafPort.portG2UR(argR);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Vir2GblPt
*
* DESCRIPTION
*
*    Function Vir2GblPt converts the specified point, argPT, from the current
*    grafPort's virtual coordinate system into a global bitmap coordinate value.
*
* INPUTS
*
*    point *argPT - Pointer to point to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Vir2GblPt(point *argPT)
{
    INT32 pointX;
    INT32 pointY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* pickup passed values */
    pointX = argPT->X;
    pointY = argPT->Y;

    /* convert to virtual */
    theGrafPort.portU2GP(&pointX, &pointY);

    /* save it */
    argPT->X = pointX;
    argPT->Y = pointY;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Vir2GblRect
*
* DESCRIPTION
*
*    Function Vir2GblRect converts the specified rectangle, argR, from the
*    current grafPort's virtual coordinate system into a global bitmap coordinate
*    value.
*
* INPUTS
*
*    rect *argR - Pointer to rect to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Vir2GblRect(rect *argR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    theGrafPort.portU2GR(argR);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Vir2LclPt
*
* DESCRIPTION
*
*    Function Vir2LclPt converts the specified point, argPT, from the current 
*    grafPort's virtual coordinate system into a local port coordinate value.
*
* INPUTS
*
*    point *argPT - Pointer to point to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Vir2LclPt(point *argPT)
{
    INT32 pointX;
    INT32 pointY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* pickup converted values */
    pointX = argPT->X;
    pointY = argPT->Y;

    /* convert to global */
    theGrafPort.portU2GP(&pointX, &pointY);

    /* save it */
    argPT->X = pointX;
    argPT->Y = pointY;

    /* convert to local */
    Gbl2LclPt(argPT);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Vir2LclRect
*
* DESCRIPTION
*
*    Function Vir2LclRect converts the specified rectangle, argR, from the
*    current grafPort's local coordinate system into virtual port coordinate
*    values.
*
* INPUTS
*
*    rect *argR - Pointer to rect to convert.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Vir2LclRect(rect *argR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* convert to global */
    theGrafPort.portU2GR(argR);

    /* convert to local */
    Gbl2LclRect(argR);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    Port2Gbl
*
* DESCRIPTION
*
*    Function Port2Gbl converts the rectangle in inpRect from the grafPort
*    pointed to by inpPort (even if its not the current port) into global
*    coordinates.
*
* INPUTS
*
*    rect *inpRect     - Pointer to input rectangle.
*
*    rsPort *inpPort - Pointer to input port.
*
*    rect *dstRect     - Pointer to destination global rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID Port2Gbl(rect *inpRect, rsPort *inpPort, rect *dstRect)
{
    INT32 saveXmin;
    INT32 saveYmin;
    INT16 saveFlags;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    *dstRect = *inpRect;

    /* virtual coord? */
    if( inpPort->portFlags & pfVirtual )
    {
        /* yes, have to pre-calculate some vars for the default routines
            including swapping portVirt.X-Ymin, flags to shadow port */
        COORDS_rsVirtCoord(inpPort);

        /* save old values */
        saveXmin  = theGrafPort.portVirt.Xmin;
        saveYmin  = theGrafPort.portVirt.Ymin;
        saveFlags = theGrafPort.portFlags;

        /* set new values */
        theGrafPort.portVirt.Xmin = inpPort->portVirt.Xmin;
        theGrafPort.portVirt.Ymin = inpPort->portVirt.Ymin;
        theGrafPort.portFlags     = inpPort->portFlags;

        /* call user routine */
        inpPort->portU2GR(dstRect);

        /* pre-calculate transformation variables back for current port */
        theGrafPort.portVirt.Xmin = saveXmin;
        theGrafPort.portVirt.Ymin = saveYmin;
        theGrafPort.portFlags     = saveFlags;
        COORDS_rsVirtCoord(&theGrafPort);
    }
    else
    {
        /* convert X */
        dstRect->Xmin = (inpRect->Xmin - inpPort->portRect.Xmin) + 
                                        inpPort->portOrgn.X;
        dstRect->Xmax = (inpRect->Xmax - inpPort->portRect.Xmin) + 
                                        inpPort->portOrgn.X;

        /* convert Y - lower left origin? */
        if( inpPort->portFlags & pfUpper )
        {
            /* upper left */
            dstRect->Ymin = (inpRect->Ymin - inpPort->portRect.Ymin) + 
                                            inpPort->portOrgn.Y;
            dstRect->Ymax = (inpRect->Ymax - inpPort->portRect.Ymin) + 
                                            inpPort->portOrgn.Y;
        }
        else
        {
            /* lower left */
            dstRect->Ymin = (-inpRect->Ymax + inpPort->portRect.Ymax) + 
                                             inpPort->portOrgn.Y;
            dstRect->Ymax = (-inpRect->Ymin + inpPort->portRect.Ymax) + 
                                             inpPort->portOrgn.Y;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsGblPenCoord
*
* DESCRIPTION
*
*    Function COORDS_rsGblPenCoord calculates the LocX and LocY variables from the current
*    ports pnLoc variables.
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
VOID COORDS_rsGblPenCoord(VOID)
{
    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GP(theGrafPort.pnLoc.X, theGrafPort.pnLoc.Y, &LocX, &LocY, 1);
    }
    else
    {
        LocX = theGrafPort.pnLoc.X;
        LocY = theGrafPort.pnLoc.Y;
    }

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsGblClip
*
* DESCRIPTION
*
*    Function COORDS_rsGblClip calculates a clip rectangle in global coordinates 
*    based on the port at ds:sn. It insures that the clip rect is within
*    the ports bitmap, and (if clipping not disabled) that it is within the
*    ports boundaries.
*
* INPUTS
*
*    rsPort *inpClpPort - Pointer to the port. 
*
*    rect *outClipR       - Pointer to the clip rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID COORDS_rsGblClip( rsPort *inpClpPort, rect *outClipR)
{
    INT32 tempXY;
    INT32 tempYmin;

    if( !(inpClpPort->portFlags & pfRecClip) )
    {
        /* rectangular clipping disabled - set clip to bitmap limits */
        outClipR->Xmin = 0;
        outClipR->Ymin = 0;
        outClipR->Xmax = inpClpPort->portMap->pixWidth;
        outClipR->Ymax = inpClpPort->portMap->pixHeight;
    }
    else
    {
        /* check clip within port limits */
        if( inpClpPort->portRect.Xmin > inpClpPort->portClip.Xmin )
        {
            outClipR->Xmin = inpClpPort->portRect.Xmin;
        }
        else
        {
            outClipR->Xmin = inpClpPort->portClip.Xmin;
        }

        if( inpClpPort->portRect.Ymin > inpClpPort->portClip.Ymin )
        {
            outClipR->Ymin = inpClpPort->portRect.Ymin;
        }
        else
        {
            outClipR->Ymin = inpClpPort->portClip.Ymin;
        }

        if( inpClpPort->portRect.Xmax < inpClpPort->portClip.Xmax )
        {
            outClipR->Xmax = inpClpPort->portRect.Xmax;
        }
        else
        {
            outClipR->Xmax = inpClpPort->portClip.Xmax;
        }

        if( inpClpPort->portRect.Ymax < inpClpPort->portClip.Ymax )
        {
            outClipR->Ymax = inpClpPort->portRect.Ymax;
        }
        else
        {
            outClipR->Ymax = inpClpPort->portClip.Ymax;
        }

        /* convert clip (always local) to global; can't call Lcl2Gbl,
           that assumes current port */
        tempXY = inpClpPort->portOrgn.X - inpClpPort->portRect.Xmin;
        outClipR->Xmin += tempXY;
        outClipR->Xmax += tempXY;

        if( inpClpPort->portFlags & pfUpper )
        {
            /* upper left */
            tempXY = inpClpPort->portOrgn.Y - inpClpPort->portRect.Ymin;
            outClipR->Ymin += tempXY;
            outClipR->Ymax += tempXY;
        }
        else
        {
            /* lower left */
            tempXY = inpClpPort->portOrgn.Y + inpClpPort->portRect.Ymax;
            tempYmin = tempXY - outClipR->Ymax;
            outClipR->Ymax = tempXY - outClipR->Ymin;
            outClipR->Ymin = tempYmin;
        }

        /* check for valid rect */
        if( outClipR->Xmin > outClipR->Xmax )
        {
            /* swap ends */
            tempXY = outClipR->Xmin;
            outClipR->Xmin = outClipR->Xmax;
            outClipR->Xmax = tempXY;
        }

        if( outClipR->Ymin > outClipR->Ymax )
        {
            /* swap ends */
            tempXY = outClipR->Ymin;
            outClipR->Ymin = outClipR->Ymax;
            outClipR->Ymax = tempXY;
        }

        /* check clip within bitmap limits */
        if( outClipR->Xmin < 0 )
        {
            outClipR->Xmin = 0;
            if( outClipR->Xmax < 0 )
            {
                outClipR->Xmax = 0;
            }
        }

        if( outClipR->Ymin < 0 )
        {
            outClipR->Ymin = 0;
            if( outClipR->Ymax < 0 )
            {
                outClipR->Ymax = 0;
            }
        }

        if( outClipR->Xmax >= inpClpPort->portMap->pixWidth )
        {
            outClipR->Xmax = inpClpPort->portMap->pixWidth;
            if( outClipR->Xmin >= inpClpPort->portMap->pixWidth)
            {
                outClipR->Xmin  = inpClpPort->portMap->pixWidth;
            }
        }

        if( outClipR->Ymax >= inpClpPort->portMap->pixHeight )
        {
            outClipR->Ymax = inpClpPort->portMap->pixHeight;
            if( outClipR->Ymin >= inpClpPort->portMap->pixHeight )
            {
                outClipR->Ymin = inpClpPort->portMap->pixHeight;
            }
        }
    } /* else */

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsVirtCoord
*
* DESCRIPTION
*
*    Function COORDS_rsVirtCoord precomputes virtual factors for standard virtual
*    call.
*
* INPUTS
*
*    rsPort *inpPort - Pointer to the port. 
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID COORDS_rsVirtCoord( rsPort *inpPort)
{
    INT16 grafErrValue;

    virtXnumer = inpPort->portRect.Xmax - inpPort->portRect.Xmin;
    if( virtXnumer == 0 )
    {
        /* error */
        virtXdenom = 1;
        grafErrValue = c_VirtualR + c_NullRect;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        virtXdenom = inpPort->portVirt.Xmax - inpPort->portVirt.Xmin;
        if( virtXdenom == 0 )
        {
            /* error */
            virtXnumer = 0;
            virtXdenom = 1;
            grafErrValue = c_VirtualR + c_NullRect;
            nuGrafErr(grafErrValue, __LINE__, __FILE__);
        }
        else
        {
            virtXround = virtXdenom >> 1;
        }
    }

    virtYnumer = inpPort->portRect.Ymax - inpPort->portRect.Ymin;
    if( virtYnumer == 0 )
    {
        /* error */
        virtYdenom = 1;
        grafErrValue = c_VirtualR + c_NullRect;
        nuGrafErr(grafErrValue, __LINE__, __FILE__);
    }
    else
    {
        virtYdenom = inpPort->portVirt.Ymax - inpPort->portVirt.Ymin;
        if( virtYdenom == 0 )
        {
            /* error */
            virtYnumer = 0;
            virtYdenom = 1;
            grafErrValue = c_VirtualR + c_NullRect;
            nuGrafErr(grafErrValue, __LINE__, __FILE__);
        }
        else
        {
            virtYround = virtYdenom >> 1;
        }
    }

    /* virt does not use port mins */
    virtXconst = inpPort->portOrgn.X;

    if( inpPort->portFlags & pfUpper )
    {
        /* upper left */
        virtYconst = inpPort->portOrgn.Y;
        localYinvert = 0;
    }
    else
    {
        virtYconst = (  inpPort->portOrgn.Y
                      + inpPort->portRect.Ymax
                      - inpPort->portRect.Ymin );

        localYinvert = -1;
    }

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsGblCoord
*
* DESCRIPTION
*
*    Function COORDS_rsGblCoord computes coordinate conversion constants.
*    Derivation of constants used for coordinate transformations:
*
*    upper left:
*       Yg = (Yl  - portYmin) + portoriginY =
*       Yg = Yl + -portYmin + portoriginY =
*       Yg = Yl + C   where C = portoriginY - portYmin
*
*    lowerleft:
*       Yg = (portYmax - Yl) + portoriginY =
*       Yg = portYmax + -Yl + portoriginY =
*
*       Yg =  -Yl + C  where C = portoriginY + portYmax 
*       Yg = (not Yl) + 1 + C   =
*       Yg = (not Yl) + (C + 1)
*
*    Let INVERT = 0, and C = (portOrignY - portYmin)  then
*       Yg = (Yl xor INVERT) + C would handle the upper left case
*
*    Let INVERT = all ones, and C = (portOrignY + portYmax + 1)  then
*       Yg = (Yl xor INVERT) + C would also handle the lower left case
*
*    The XOR would not be necessary for the X cases if no mirroring in the
*    X direction is required.
*
*    Virtual to Local
*    ----------------
*           sub  x, GDS:[grafPort.portVirt.Xmin] ;make relative to min in virtRect
*           sub  y, GDS:portVirt.Ymin
*
*           x mul virtXnumer      ( portrect X size )
*           x += .5 * virtXdenom  (pre round up)
*           x(32) div virtXdenom  ( virtual rect X size )
*
*           y mul virtYnumer 
*           y += .5 * virtYdenom  (pre round up)
*           y(32) div virtYdenom
*
*           ;at this point the x,y is relative to 0
*    upper left:
*            virtconst   = portOrign =
*                     local C + portMin
*    lower left:
*            virtYconst  = (portYmax + portOrignY + 1) - portYmin =
*                      local C - portYmin
*
*           ;convert from local 0,0 to global
*           add  x, virtXconst
*           xor  y, localInvert
*           add  y, virtYconst
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
VOID COORDS_rsGblCoord(VOID)
{
    /* local X conversion constant */
    localXconst = theGrafPort.portOrgn.X - theGrafPort.portRect.Xmin;

    /* local Y conversion constant */
    if( theGrafPort.portFlags & pfUpper )
    {
        /* upper left */
        localYconst = theGrafPort.portOrgn.Y - theGrafPort.portRect.Ymin;
    }
    else
    {
        /* lower left */
        localYconst = theGrafPort.portOrgn.Y + theGrafPort.portRect.Ymax;
    }

    /* precompute virtual factors for standard virtual call */
    COORDS_rsVirtCoord(&theGrafPort);

    /* set globalLevel indicator and flag */
    if( (theGrafPort.portFlags & pfVirtual) || (localXconst | localYconst |
        localXinvert | localYinvert) )
    {
        /* is virtual or not global */
        globalLevel = 1;
        theGrafPort.portFlags = theGrafPort.portFlags & ~pfGblCoord;
    }
    else
    {
        /* is global */
        globalLevel = -1;
        theGrafPort.portFlags = theGrafPort.portFlags | pfGblCoord;
    }

    /* update user port */
    thePort->portFlags = theGrafPort.portFlags; 

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsV2GP
*
* DESCRIPTION
*
*    Function COORDS_rsV2GP is the default virtual to global point
*    transformation routine.
*
* INPUTS
*
*    INT32 *virtX - Pointer to the virtual x point.
*
*    INT32 *virtY - Pointer to the virtual y point.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID COORDS_rsV2GP( INT32 *virtX, INT32 *virtY)
{
    SIGNED tempXY;
    INT16 GrafErrValue;

    /* convert X */
    tempXY = ((((*virtX - theGrafPort.portVirt.Xmin) * virtXnumer) +
        virtXround) / virtXdenom) + virtXconst;

    if( tempXY > 32767 )
    {
        /* overflow */
        *virtX = 32767;
        GrafErrValue = c_Vir2LclP + c_DivOflow;
        nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        /* check negative overflow */
        if( tempXY < -32767 )
        {
            /* overflow */
            *virtX = -32767;
            GrafErrValue = c_Vir2LclP + c_DivOflow;
            nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
        }
        else
        {
            /* okay */
            *virtX = (INT32) tempXY;
        }
    }

    /* convert Y */
    tempXY = (((((*virtY - theGrafPort.portVirt.Ymin) * virtYnumer) +
        virtYround) / virtYdenom) ^ localYinvert) + virtYconst;

    if( tempXY > 32767 )
    {
        /* overflow */
        *virtY = 32767;
        GrafErrValue = c_Vir2LclP + c_DivOflow;
        nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        /* check negative overflow */
        if( tempXY < -32767 )
        {
            /* overflow */
            *virtY = -32767;
            GrafErrValue = c_Vir2LclP + c_DivOflow;
            nuGrafErr(GrafErrValue, __LINE__, __FILE__);
        }
        else
        {
            /* okay */
            *virtY = (INT32) tempXY;
        }
    }

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsV2GR
*
* DESCRIPTION
*
*    Function COORDS_rsV2GR is the default virtual to global rectangle
*    transformation routine.
*
* INPUTS
*
*    rect *virRect - Pointer to the virtual rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID COORDS_rsV2GR( rect *virRect)
{
    INT32 tempX;
    INT32 tempY;

    /* convert the start point */
    tempX = virRect->Xmin;
    tempY = virRect->Ymin;
    COORDS_rsV2GP(&tempX, &tempY);
    virRect->Xmin = tempX;
    virRect->Ymin = tempY;

    /* convert the end point */
    tempX = virRect->Xmax;
    tempY = virRect->Ymax;
    COORDS_rsV2GP(&tempX, &tempY);
    virRect->Xmax = tempX;

    if( theGrafPort.portFlags & pfUpper )
    {
        /* upper origin */
        virRect->Ymax = tempY;
    }
    else
    {
        /* lower origin */
        virRect->Ymax = virRect->Ymin;
        virRect->Ymin = tempY;
    }

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsG2VP
*
* DESCRIPTION
*
*    Function COORDS_rsG2VP is the default global to virtual point
*    transformation routine.
*
* INPUTS
*
*    INT32 *virtX - Pointer to the virtual point x.
*
*    INT32 *virtY - Pointer to the virtual point y.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID COORDS_rsG2VP( INT32 *virtX, INT32 *virtY)
{
    INT16  JumpConvertY = NU_FALSE;
    SIGNED tempXY;
    INT16  GrafErrValue;

    /* convert X */
    if( virtXnumer == 0 )
    {
        /* divide by zero! */
        if( *virtX < 0 )
        {
            *virtX = -32767;
        }
        else
        {
            *virtX = 32767;
        }
        GrafErrValue = c_Gbl2VirP + c_DivOflow;
        nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
        JumpConvertY = NU_TRUE;
    }

    if( !JumpConvertY )
    {
        tempXY = ((((*virtX - theGrafPort.portOrgn.X) * virtXdenom) +
            (virtXnumer >> 1)) / virtXnumer) + theGrafPort.portVirt.Xmin;

        if( tempXY > 32767 )
        {
            /* overflow */
            *virtX = 32767;
            GrafErrValue = c_Gbl2VirP + c_DivOflow;
            nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
        }
        else
        {
            /* check negative overflow */
            if( tempXY < -32767 )
            {
                /* overflow */
                *virtX = -32767;
                GrafErrValue = c_Gbl2VirP + c_DivOflow;
                nuGrafErr(GrafErrValue, __LINE__, __FILE__);
            }
            else
            {
                /* okay */
                *virtX = (INT32) tempXY;
            }
        }
    }
    /* convert Y */
    if( virtYnumer == 0 )
    {
        /* divide by zero! */
        if( *virtY < 0 )
        {
            *virtY = -32767;
        }
        else
        {
            *virtY = 32767;
        }
        GrafErrValue = c_Gbl2VirP + c_DivOflow;
        nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        tempXY = *virtY - theGrafPort.portOrgn.Y;
        if( !(theGrafPort.portFlags & pfUpper) )
        {
            /* lower origin */
            tempXY = theGrafPort.portRect.Ymax - tempXY;
        }

        tempXY = (((tempXY * virtYdenom) + (virtYnumer >> 1)) /
            virtYnumer) + theGrafPort.portVirt.Ymin;

        if( tempXY > 32767 )
        {
            /* overflow */
            *virtY = 32767;
            GrafErrValue = c_Gbl2VirP + c_DivOflow;
            nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
        }
        else
        {
            /* check negative overflow */
            if( tempXY < -32767 )
            {
                /* overflow */
                *virtY = -32767;
                GrafErrValue = c_Gbl2VirP + c_DivOflow;
                nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
            }
            else
            {
                /* okay */
                *virtY = (INT32) tempXY;
            }
        }
    } /* else */

}

/***************************************************************************
* FUNCTION
*
*    COORDS_rsG2VR
*
* DESCRIPTION
*
*    Function COORDS_rsG2VR is the default global to virtual rectangle
*    transformation routine.
*
* INPUTS
*
*    rect *virRect - Pointer to the virtual rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID COORDS_rsG2VR( rect *virRect)
{
    INT32 tempX;
    INT32 tempY;

    /* convert the start point */
    tempX = virRect->Xmin;
    tempY = virRect->Ymin;
    COORDS_rsG2VP(&tempX, &tempY);
    virRect->Xmin = tempX;
    virRect->Ymin = tempY;

    /* convert the end point */
    tempX = virRect->Xmax;
    tempY = virRect->Ymax;
    COORDS_rsG2VP(&tempX, &tempY);
    virRect->Xmax = tempX;

    if( virRect->Ymin <= tempY )
    {
        /* okay */
        virRect->Ymax = tempY;
    }
    else
    {
        /* min and max swapped */
        virRect->Ymax = virRect->Ymin;
        virRect->Ymin = tempY;
    }

}

/***************************************************************************
* FUNCTION
*
*    U2GP
*
* DESCRIPTION
*
*    Function U2GP converts a User Point to a Global Point.
*    UserX, UserY is point in user coordinates RtnX, RtnY is a pointer to 
*    return global coordinates frame = 1 if frames, 0 if not.
*
* INPUTS
*
*    INT32 UserX - X point in user coordinates.
*
*    INT32 UserY  - Y point in user coordinates.
*
*    INT32 *RtnX - Pointer to returned X global coordinate.
*
*    INT32 *RtnY - Pointer to returned Y global coordinate.
*
*    INT16 frame  - 1 frame
*                   0 no frame.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame)
{
    INT32 TempX;
    INT32 TempY;

    if( theGrafPort.portFlags & pfVirtual )
    {
        TempX = UserX;
        TempY = UserY;

        theGrafPort.portU2GP(&TempX, &TempY);

        *RtnX = TempX;
        *RtnY = TempY;
    }
    else 
    {
        /* add offset */
        *RtnX = (UserX + localXconst); 

        /* conditionally mirror Y */
        *RtnY = ((UserY ^ localYinvert) + localYconst); 
    }

    if( frame && !(theGrafPort.portFlags & pfUpper) )
    {
        /* adjust up for frames */
        *RtnY = *RtnY - 1;
    }
}

/***************************************************************************
* FUNCTION
*
*    G2UP
*
* DESCRIPTION
*
*    Function G2UP converts a Global Point to a User Point UserX, UserY is 
*    point in user coordinates RtnX, RtnY is a pointer to return user coordinates.
*
* INPUTS
*
*    INT32 GloblX  - X point in global coordinates.
*
*    INT32 GloblY  - Y point in global coordinates.
*
*    INT32 *RtnX  - Pointer to returned X user coordinate.
*
*    INT32 *RtnY  - Pointer to returned Y user coordinate.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID G2UP(INT32 GloblX, INT32 GloblY, INT32 *RtnX, INT32 *RtnY)
{
    INT32 TempX;
    INT32 TempY;

    if( theGrafPort.portFlags & pfVirtual )
    {
        TempX = GloblX;
        TempY = GloblY;

        theGrafPort.portG2UP(&TempX, &TempY);

        *RtnX = TempX;
        *RtnY = TempY;
    }
    else 
    {
        *RtnX = GloblX + theGrafPort.portRect.Xmin - theGrafPort.portOrgn.X;
        if( theGrafPort.portFlags & pfUpper )
        {
            *RtnY = GloblY + theGrafPort.portRect.Ymin - theGrafPort.portOrgn.Y;
        }
        else
        {
            *RtnY = -GloblY + theGrafPort.portRect.Ymin + theGrafPort.portOrgn.Y - 1;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    U2GR
*
* DESCRIPTION
*
*    Function U2GR converts a User Rectangle to a Global Rectangle.
*    UserRect is rectangle in user coordinates.
*    RtnRect is pointer to return global rectangle.
*    frame = 1 if frames, 0 if not.
*
* INPUTS
*
*    rect UserRect - Rectangle in user coordinates.
*
*    rect *RtnRect - Pointer to rectangle returned in global coordinates.
*
*    INT16 frame   - 1 frame.
*                  - 0 no frame.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame)
{
    rect TempRect;

    if( theGrafPort.portFlags & pfVirtual )
    {
        TempRect = UserRect;
        theGrafPort.portU2GR(&TempRect);
        *RtnRect = TempRect;
    }
    else 
    {
        /* add offset */
        RtnRect->Xmin = (UserRect.Xmin + localXconst); 
        RtnRect->Xmax = (UserRect.Xmax + localXconst);

        if( theGrafPort.portFlags & pfUpper )
        {
            /* conditionally mirror Y */
            RtnRect->Ymin = ((UserRect.Ymin ^ localYinvert) + localYconst);
            RtnRect->Ymax = ((UserRect.Ymax ^ localYinvert) + localYconst);
        }

        /* swap Ymin with Ymax */
        else    
        {
            /* conditionally mirror Y */
            RtnRect->Ymax = ((UserRect.Ymin ^ localYinvert) + localYconst);
            RtnRect->Ymin = ((UserRect.Ymax ^ localYinvert) + localYconst);
        }
    }

    /* adjust up for frames */
    if( frame && !(theGrafPort.portFlags & pfUpper) )
    {
        RtnRect->Ymin--;
        RtnRect->Ymax--;
    }

}

/***************************************************************************
* FUNCTION
*
*    G2UR
*
* DESCRIPTION
*
*    Function G2UR converts a Global Rectangle to a User Rectangle.
*    UserRect is rectangle in user coordinates.
*    RtnRect is pointer to return global rectangle.
*
* INPUTS
*
*    rect UserRect - Rectangle in global coordinates.
*
*    rect *RtnRect - Pointer to rectangle returned in user coordinates.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID G2UR(rect UserRect, rect *RtnRect)
{
    rect TempRect;

    if( theGrafPort.portFlags & pfVirtual )
    {
        TempRect = UserRect;
        theGrafPort.portG2UR(&TempRect);
        *RtnRect = TempRect;
    }
    else 
    {
        RtnRect->Xmin = UserRect.Xmin + theGrafPort.portRect.Xmin - theGrafPort.portOrgn.X;
        RtnRect->Xmax = UserRect.Xmax + theGrafPort.portRect.Xmin - theGrafPort.portOrgn.X;

        if( theGrafPort.portFlags & pfUpper )
        {
            RtnRect->Ymin = UserRect.Ymin + theGrafPort.portRect.Ymin - theGrafPort.portOrgn.Y;
            RtnRect->Ymax = UserRect.Ymax + theGrafPort.portRect.Ymin - theGrafPort.portOrgn.Y;
        }
        else
        {
            RtnRect->Ymin = -UserRect.Ymin + theGrafPort.portRect.Ymax + theGrafPort.portOrgn.Y;
            RtnRect->Ymax = -UserRect.Ymax + theGrafPort.portRect.Ymax + theGrafPort.portOrgn.Y;
        }
    }
}

/***************************************************************************
* FUNCTION
*
*    V2GSIZE
*
* DESCRIPTION
*
*    Function V2GSIZE converts virtual xy to a global xy.
*    UserRect is rectangle in virtual coordinates.
*    RtnRect is pointer to return global rectangle.
*
* INPUTS
*
*    INT32 UserX   - X point in virtual coordinates.
* 
*    INT32 UserY   - Y point in virtual coordinates.
*
*    INT32 *RtnX  - Pointer to returned X global coordinate.
*
*    INT32 *RtnY* - Pointer to returned Y global coordinate.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID V2GSIZE(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY)
{
    INT32 TempX;
    INT32 TempY;

    /* make relative to virtual rect min */
    TempX = UserX + theGrafPort.portVirt.Xmin;
    TempY = UserY + theGrafPort.portVirt.Ymin;

    theGrafPort.portU2GP(&TempX, &TempY);

    /* make relative to 0 in port */
    *RtnX = TempX - theGrafPort.portOrgn.X;
    *RtnY = TempY - theGrafPort.portOrgn.Y;

    /* adjust up for frames */
    if( !(theGrafPort.portFlags & pfUpper) )
    {
        /* find magnitude from bottom of port to Y */
        *RtnY = (theGrafPort.portRect.Ymax - theGrafPort.portRect.Ymin) - *RtnY;
    }

}
