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
*  line_polysegment.c
*
* DESCRIPTION
*
*  This file contains API - level line drawing function, PolySegment.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PolySegment
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  line.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/line.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    PolySegment
*
* DESCRIPTION
*
*    Function PolySegment draws a series of disconnected line segments defined
*    in SEGLIST. SEGCNT specifies the number of line segments in the SEGLIST
*    array. The current port pen attributes (pen-size, pen-pattern, rasterOp,
*    etc.) are used in drawing the lines.
*
* INPUTS
*
*    INT16 SEGCNT    - Segment count.
* 
*    segPts *SEGLIST - Pointer to the segments.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PolySegment( INT16 SEGCNT, segPts *SEGLIST )
{
    /* pointer to current blitList */
    SIGNED savedList;                 


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    lineExecPntr = (VOID (*)()) lineExecIDV;

    /* pass count to blitRcd */
    grafBlit.blitCnt = SEGCNT;         

    /* save current blitList */
    savedList = grafBlit.blitList;
    
    /* load passed blitList */
    grafBlit.blitList = (SIGNED) SEGLIST;

    /* call the current line routine with the default blitRcd */
    lineExecPntr(&grafBlit);

    /* restore saved blitList */
    grafBlit.blitList = savedList;

    /* reset count to 1*/
    grafBlit.blitCnt  = 1;              

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}
