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
*  pens_definedash.c
*
* DESCRIPTION
*
*  This file contains Pen related function, DefineDash.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  DefineDash
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
*    DefineDash
*
* DESCRIPTION
*
*    Function DefineDash redefines the specified pen dashing index, in the range
*    1 to 7, to a user specified style.
*
* INPUTS
*
*    INT32 STYLE      - The pen's style.
*
*    dashRcd *LNSTYLE - Pointer to pen record.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DefineDash(INT32 argStyle, dashRcd *dashArray)
{
    
#ifdef  DASHED_LINE_SUPPORT
    
    INT16 Done = NU_FALSE;

    INT16 grafErrValue; 
    INT32 dashListSize;
    INT32 i;
    const UINT8 *dashPtr;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (argStyle < 1) || (argStyle > 7) )
    {
        grafErrValue = c_DefineDa +  c_BadDash;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        Done = NU_TRUE;
    }

    if( !Done )
    {
        if( dashArray == 0 )
        {
            /* reset to the default for this dash #? */
            dashArray = (dashRcd *)&DefDashTable[argStyle];
        }

        dashListSize = dashArray->dashSize;

        /* verify that the new dashRcd is okay */
        if( dashListSize == 0 )  
        {   
            grafErrValue = c_DefineDa +  c_BadDashCnt;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
        }
    }

    if( !Done )
    {
        dashPtr = dashArray->dashList;
        for( i = 0; i < dashListSize; i++)
        {
            if( *dashPtr != 0 )
            {
                /* We have a valid dash sequence */
                /* are we changing the current dash? */
                if( argStyle != theGrafPort.pnDashNdx )
                {
                    /* yes, reset to the start of the new dash sequence in the shadow port */
                    theGrafPort.pnDashCnt = 0;

                    /* reset in the real port too */
                    thePort->pnDashCnt = 0; 
                }

                theGrafPort.pnDashRcd[argStyle].dashSize = dashArray->dashSize;
                theGrafPort.pnDashRcd[argStyle].dashRsvd = dashArray->dashRsvd;
                theGrafPort.pnDashRcd[argStyle].dashList = dashArray->dashList;
                Done = NU_TRUE;
            }
        }
    }

    if( !Done )
    {
        /*  must be 1 non-NULL pointer */
        grafErrValue = c_DefineDa +  c_BadDashCnt;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }

    /* Return to user mode */
    NU_USER_MODE();
    
#else

    NU_UNUSED_PARAM(argStyle);
    NU_UNUSED_PARAM(dashArray);
    
#endif  /* DASHED_LINE_SUPPORT */

}
