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
*  virtrect.c                                                   
*
* DESCRIPTION
*
*  Contains the VirtualRect function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  VirtualRect
*
* DEPENDENCIES
*
*  rs_base.h
*  virtrect.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/virtrect.h"

/***************************************************************************
* FUNCTION
*
*    VirtualRect
*
* DESCRIPTION
*
*    Function VirtualRect sets the virtual rectangle limits of the current 
*    grafPort to the rectangle specified by vRECT.
*
* INPUTS
*
*    rect *vRECT - Pointer to virtual rectangle.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID VirtualRect(rect *vRECT)
{
    INT16 grafErrValue; 

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* check for null and negative size rectangles */
    if( (vRECT->Xmin == vRECT->Xmax) || (vRECT->Ymin == vRECT->Ymax) )
    {
        grafErrValue = c_VirtualR +  c_NullRect;

        /* report error */
        nuGrafErr(grafErrValue, __LINE__, __FILE__);    
    }
    else
    if( (vRECT->Xmin > vRECT->Xmax) || (vRECT->Ymin > vRECT->Ymax) )
    {
        grafErrValue = c_VirtualR +  c_OfloRect;
        nuGrafErr(grafErrValue, __LINE__, __FILE__);  
    }
    else
    {
        /* set user port */
        /* set user port flags */
        thePort->portFlags |= pfVirtual;        

        /* set user port virtrect */
        thePort->portVirt.Xmin = vRECT->Xmin;   
        thePort->portVirt.Ymin = vRECT->Ymin;
        thePort->portVirt.Xmax = vRECT->Xmax;
        thePort->portVirt.Ymax = vRECT->Ymax;

        /* set shadow port */
        /* set shadow port flags */
        theGrafPort.portFlags |= pfVirtual;        

        /* set shadow port virtrect */
        theGrafPort.portVirt.Xmin = vRECT->Xmin;   
        theGrafPort.portVirt.Ymin = vRECT->Ymin;
        theGrafPort.portVirt.Xmax = vRECT->Xmax;
        theGrafPort.portVirt.Ymax = vRECT->Ymax;

        /* update precomputed xform constants */
        COORDS_rsGblCoord(); 
    }

    /* Return to user mode */
    NU_USER_MODE();
}
