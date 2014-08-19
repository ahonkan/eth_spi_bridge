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
*  pens_penoffset.c
*
* DESCRIPTION
*
*  This file contains Pen related function, PenOffset.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PenOffset
*
* DEPENDENCIES
*
*  rs_base.h
*  pens.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/pens.h"

/***************************************************************************
* FUNCTION
*
*    PenOffset
*
* DESCRIPTION
*
*    Function PenOffset defines the starting position within a dash sequence at
*    which to begin drawing the current dashed line style.  DASHOFFSET specifies
*    the dash sequence starting position, specified in units of horizontal pixels.
*
* INPUTS
*
*    INT32 DSHOFFSET - The dash sequence starting position.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PenOffset(INT32 DSHOFFSET)
{
    
#ifdef  DASHED_LINE_SUPPORT
    
    INT16 Done      = NU_FALSE;
    INT16 JumpDP150 = NU_FALSE;
    INT16 grafErrValue; 
    INT32 dashListSize;
    INT32 currDashRcd;
    INT32 i;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (theGrafPort.pnDash < 1) || (theGrafPort.pnDash > 7) )
    {
        grafErrValue = c_PenOffse +  c_BadDash;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* current dashRcd # */
        currDashRcd = theGrafPort.pnDash;  
        dashListSize = theGrafPort.pnDashRcd[currDashRcd].dashSize;

        /* verify that the new dashRcd is okay */
        if( dashListSize == 0 )
        {
            grafErrValue = c_PenOffse +  c_BadDashCnt;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
        }
    }

    if( !Done )
    {
        /* set off because it gets flipped the first time */
        theGrafPort.pnFlags &= ~pnDashState;
                                        
        while(1) /* note: this will be an infinite loop if all dash counts are 0.
                    That is guarded against in DefineDash; any user who sets up dashing
                    in the port themselves will just have to worry about it themselves! */
        {
            for( i = 0; i < dashListSize; i++)
            {
                /* flip the dash state */
                theGrafPort.pnFlags ^= pnDashState;    
                DSHOFFSET -= theGrafPort.pnDashRcd[currDashRcd].dashList[i];
                if( DSHOFFSET < 0 )
                {
                    /* count down dash sizes */
                    JumpDP150 = NU_TRUE; 
                    break;
                }
            }
            if( JumpDP150 )
            {
                break; 
            }
        }

        JumpDP150 = NU_FALSE;

        /* To remove paradigm warning */
        (VOID)JumpDP150;

        /* store the adjusted count remaining in the shadow port */
        theGrafPort.pnDashCnt = -DSHOFFSET;         

        /* ditto for the dash # */
        theGrafPort.pnDashNdx = i;                  

        /* set the real port, too */
        thePort->pnDashCnt = theGrafPort.pnDashCnt; 
        thePort->pnDashNdx = theGrafPort.pnDashNdx;
        thePort->pnFlags   = theGrafPort.pnFlags;
    }

    /* Return to user mode */
    NU_USER_MODE();

#else

    NU_UNUSED_PARAM(DSHOFFSET);

#endif  /* DASHED_LINE_SUPPORT */
    
}
