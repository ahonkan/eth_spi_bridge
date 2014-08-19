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
*  texti_rasterop.c                                                      
*
* DESCRIPTION
*
*  Contains the API function RasterOp.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RasterOp
*
* DEPENDENCIES
*
*  rs_base.h
*  edges.h
*  rsfonts.h
*  fonti.h
*  texti.h
*  globalrsv.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/edges.h"
#include "ui/rsfonts.h"
#include "ui/fonti.h"
#include "ui/texti.h"
#include "ui/globalrsv.h"

/***************************************************************************
* FUNCTION
*
*    RasterOp
*
* DESCRIPTION
*
*    Function RasterOp defines the raster write operation for subsequent
*    line drawing, text, blit, or fill functions.
*
* INPUTS
*
*    INT32 argWMODE - see description above.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RasterOp(INT32 argWMODE)
{
    INT16 grafErrValue; 


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( argWMODE > 32 )
    {
        /* error! */
        grafErrValue = c_RasterOp +  c_BadRasOp;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        /* set shadow port */
        theGrafPort.pnMode = argWMODE;
        theGrafPort.txMode = argWMODE;

        /* set default blit record */
        grafBlit.blitRop = argWMODE;

        /* set user port */
        thePort->pnMode = argWMODE;
        thePort->txMode = argWMODE;
    }

    /* Return to user mode */
    NU_USER_MODE();
}
