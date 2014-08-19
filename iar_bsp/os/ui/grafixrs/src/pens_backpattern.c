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
*  pens_backpattern.c
*
* DESCRIPTION
*
*  This file contains Pen related function, BackPattern.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  BackPattern
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
*    BackPattern
*
* DESCRIPTION
*
*    Function BackPattern sets the "bkPat" field of the current viewport to
*    one of 32 predefined patterns.
*
* INPUTS
*
*    INT32 patNDX - Background pattern.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID BackPattern(INT32 patNDX)
{
    
#ifdef  FILL_PATTERNS_SUPPORT
    
    INT16 Done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( !Done )
    {
        theGrafPort.bkPat = patNDX;
        thePort->bkPat = patNDX;
    }

    /* Return to user mode */
    NU_USER_MODE();

#else

    NU_UNUSED_PARAM(patNDX);
    
#endif /* FILL_PATTERNS_SUPPORT */

}
